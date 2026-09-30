// mcxx.backend.clang against a small modules program written to a scratch directory: no standard library,
// so it needs no toolchain beyond libmc++ itself and runs in seconds. What each case pins down is a
// behaviour mcppls depends on.
import std;
import mcxx.testing;
import mcxx.msa;
import mcxx.backend.clang;
import mcxx.os;
import mcxx.base;
import mcxx.plugins.json;
import mcxx.plugins.std;
import example.device;

namespace msa = mcxx::msa;

namespace {

struct Program {
    std::filesystem::path root;
    std::vector<msa::Command> commands;

    explicit Program(std::string_view name) {
        root = std::filesystem::temp_directory_path() / std::format("mcxx-backend-{}-{}", name, std::random_device {}());
        std::filesystem::create_directories(root / "src");
    }
    ~Program() {
        std::error_code ec;
        std::filesystem::remove_all(root, ec);
    }
    void manifest(std::string_view text) { std::ofstream { root / "mcpp.toml" } << text; }
    std::string file(std::string_view relative, std::string_view text) {
        const auto path = root / relative;
        std::filesystem::create_directories(path.parent_path());
        std::ofstream { path } << text;
        // As libmc++ spells paths (mcxx.base): '/' separators, a Windows path's drive kept.
        const std::string p { mcxx::base::normalize_path(path.generic_string()) };
        commands.push_back({ mcxx::base::normalize_path(root.generic_string()), p, { "clang++", "-std=c++23", "-c", p } });
        return p;
    }
};

std::unique_ptr<msa::Workspace> workspace_for(const Program& program) {
    msa::Workspace::Options options;
    options.cache_directory = (program.root / ".cache").generic_string();
    options.workers = 2;
    options.background_index = true;
    auto w = mcxx::backend::clang::make_workspace(std::move(options));
    w->set_commands(program.commands);
    return w;
}

std::optional<msa::Position> find(std::string_view text, std::string_view needle, std::uint32_t skip = 0) {
    std::uint32_t line { 0 };
    std::size_t start { 0 };
    while (start <= text.size()) {
        const std::size_t end { std::min(text.find('\n', start), text.size()) };
        const std::string_view l { text.substr(start, end - start) };
        if (const auto at = l.find(needle); at != std::string_view::npos) return msa::Position { line, static_cast<std::uint32_t>(at + skip) };
        start = end + 1;
        ++line;
    }
    return std::nullopt;
}

} // namespace

int main() {
    using namespace mcxx::testing;

    "a module program parses clean; hover facts and cross-module definitions"_test = [] {
        Program p { "basic" };
        const std::string lib { p.file("src/greet.cppm", "export module hello.greet;\nexport namespace hello {\n/// Twice what it is given.\nconstexpr int twice(int v) { return v + v; }\nconstexpr int answer = twice(21);\n}\n") };
        const std::string mainText { "import hello.greet;\nint main() { return hello::answer - hello::twice(21); }\n" };
        const std::string main { p.file("src/main.cpp", mainText) };
        auto w = workspace_for(p);
        auto unit = w->parse(main, mainText, 1);
        expect(fatal(unit != nullptr));
        expect(unit->diagnostics().empty()) << (unit->diagnostics().empty() ? std::string {} : unit->diagnostics().front().message);
        const auto answer = unit->entity_at(*find(mainText, "answer", 1));
        expect(fatal(answer.has_value()));
        expect(answer->name == "answer" && answer->container == "hello" && answer->value == "42") << answer->value;
        expect(answer->module == "hello.greet");
        expect(answer->declaration && answer->declaration->path == lib) << "definition lives in the interface";
        const auto twice = unit->entity_at(*find(mainText, "twice", 1));
        expect(fatal(twice.has_value()));
        expect(twice->kind == msa::Kind::function && twice->documentation.contains("Twice what it is given"));
        expect(twice->parameters.size() == 1 && twice->parameters[0].name == "v");
    };

    "a failed module is named on the import, with its cause, and nothing else is said about it"_test = [] {
        Program p { "failure" };
        p.file("src/base.cppm", "export module chain.base;\nexport int base() { return undeclared_symbol; }\n");
        p.file("src/mid.cppm", "export module chain.mid;\nimport chain.base;\nexport int mid() { return 1; }\n");
        const std::string text { "import chain.mid;\nint main() { return 0; }\n" };
        const std::string main { p.file("src/main.cpp", text) };
        auto w = workspace_for(p);
        auto unit = w->parse(main, text, 1);
        expect(fatal(unit != nullptr));
        const auto diagnostics = unit->diagnostics();
        expect(fatal(diagnostics.size() == 1)) << std::format("{} diagnostics", diagnostics.size());
        expect(diagnostics[0].code == "module-failed");
        expect(diagnostics[0].message.contains("chain.mid cannot be built because chain.base did not compile")) << diagnostics[0].message;
        expect(diagnostics[0].range.begin.line == 0 && diagnostics[0].range.begin.column == 7) << "on the import's name";
        const auto status = w->status();
        expect(std::ranges::any_of(status.failures, [](const msa::ModuleFailure& f) { return f.module == "chain.mid" && f.cause == "chain.base"; }));
    };

    "a module dispatched before the program changed waits for its dependency to be built again, not failed on it"_test = [] {
        Program p { "reprogram" };
        p.file("src/a.cppm", "export module a;\n#ifdef FIXED\nexport int a() { return 1; }\n#else\nexport int a() { return undeclared_symbol; }\n#endif\n");
        p.file("src/b.cppm", "export module b;\nimport a;\nexport int b() { return a(); }\n");
        const std::string text { "import b;\nint main() { return b(); }\n" };
        const std::string main { p.file("src/main.cpp", text) };
        std::vector<msa::Command> fixed { p.commands };
        for (auto& c : fixed) c.arguments.push_back("-DFIXED");
        // One worker, and the program described again from its notice that `a` failed: `b`, dispatched
        // on that failure, is still queued when `a` goes stale (mcppls: the inferred plan, then mcpp's).
        std::atomic<msa::Workspace*> self { nullptr };
        std::atomic_bool reprogrammed { false };
        msa::Workspace::Options options;
        options.cache_directory = (p.root / ".cache").generic_string();
        options.workers = 1;
        options.background_index = false;
        options.changed = [&] {
            msa::Workspace* w { self.load() };
            if (w == nullptr || reprogrammed.load()) return;
            const auto s = w->status();
            if (std::ranges::none_of(s.failures, [](const msa::ModuleFailure& f) { return f.module == "a"; })) return;
            if (!reprogrammed.exchange(true)) w->set_commands(fixed);
        };
        auto w = mcxx::backend::clang::make_workspace(std::move(options));
        self = w.get();
        w->set_commands(p.commands);
        for (int round { 0 }; round < 600 && (!reprogrammed.load() || w->status().busy); ++round)
            std::this_thread::sleep_for(std::chrono::milliseconds { 100 });
        expect(fatal(reprogrammed.load())) << "a failed once";
        auto unit = w->parse(main, text, 1);
        expect(fatal(unit != nullptr));
        const auto diagnostics = unit->diagnostics();
        expect(diagnostics.empty()) << std::format("{} diagnostics: {}", diagnostics.size(), diagnostics.empty() ? std::string {} : diagnostics[0].message);
        expect(w->status().failures.empty()) << std::format("{} modules failed", w->status().failures.size());
        self = nullptr;
    };

    "a failed module, and what imports it, are built again when a header it read is mended"_test = [] {
        Program p { "header" };
        // Not a unit of the program (no command): only the module that includes it knows it.
        const auto header = p.root / "src/base.h";
        std::ofstream { header } << "inline int base_value() { return undeclared_symbol; }\n";
        p.file("src/base.cppm", "module;\n#include \"base.h\"\nexport module chain.base;\nexport int base() { return base_value(); }\n");
        p.file("src/mid.cppm", "export module chain.mid;\nimport chain.base;\nexport int mid() { return base(); }\n");
        const std::string text { "import chain.mid;\nint main() { return mid(); }\n" };
        const std::string main { p.file("src/main.cpp", text) };
        auto w = workspace_for(p);
        auto unit = w->parse(main, text, 1);
        expect(fatal(unit != nullptr));
        expect(unit->diagnostics().size() == 1) << std::format("{} diagnostics", unit->diagnostics().size());
        std::ofstream { header } << "inline int base_value() { return 1; }\n";
        w->file_changed(mcxx::base::normalize_path(header.generic_string()));
        auto again = w->parse(main, text, 2);
        expect(fatal(again != nullptr));
        const auto after = again->diagnostics();
        expect(after.empty()) << std::format("{} diagnostics: {}", after.size(), after.empty() ? std::string {} : after[0].message);
        expect(w->status().failures.empty()) << std::format("{} modules still failed", w->status().failures.size());
    };

    "an interface edited in the editor is what its importers see"_test = [] {
        Program p { "overlay" };
        const std::string libText { "export module lib;\nexport int one() { return 1; }\n" };
        const std::string lib { p.file("src/lib.cppm", libText) };
        const std::string text { "import lib;\nint main() { return two(); }\n" };
        const std::string main { p.file("src/main.cpp", text) };
        auto w = workspace_for(p);
        auto before = w->parse(main, text, 1);
        expect(fatal(before != nullptr));
        expect(!before->diagnostics().empty()) << "two() is not there yet";
        (void)w->parse(lib, "export module lib;\nexport int one() { return 1; }\nexport int two() { return 2; }\n", 2);
        auto after = w->parse(main, text, 2);
        expect(fatal(after != nullptr));
        expect(after->diagnostics().empty()) << (after->diagnostics().empty() ? std::string {} : after->diagnostics().front().message);
        w->close(lib);
        auto closed = w->parse(main, text, 3);
        expect(!closed->diagnostics().empty()) << "closed: the disk's text again";
    };

    "diagnostic codes are clang's names without their kind"_test = [] {
        Program p { "codes" };
        const std::string text { "int main() { return missing; }\n" };
        const std::string main { p.file("src/main.cpp", text) };
        auto w = workspace_for(p);
        auto unit = w->parse(main, text, 1);
        expect(fatal(unit != nullptr && !unit->diagnostics().empty()));
        expect(unit->diagnostics()[0].code == "undeclared_var_use") << unit->diagnostics()[0].code;
    };


    // No standard library here: the smallest std::initializer_list Clang accepts, and a stand-in for
    // nlohmann::basic_json with its ABI inline namespace and its initializer_list constructor.
    static constexpr std::string_view JSON_PRELUDE {
        "namespace std {\n"
        "template <class E> class initializer_list {\n"
        "    const E* b; decltype(sizeof(0)) n;\n"   // size_t, whatever the target's
        "    constexpr initializer_list(const E* b, decltype(sizeof(0)) n) : b(b), n(n) {}\n"
        "public:\n"
        "    constexpr initializer_list() : b(nullptr), n(0) {}\n"
        "    constexpr decltype(sizeof(0)) size() const { return n; }\n"
        "};\n"
        "template <class T> struct vector { T* data; };\n"
        "}\n"
        "namespace nlohmann { inline namespace json_abi_v3_12_0 {\n"
        "template <class T = int> class basic_json {\n"
        "public:\n"
        "    basic_json(decltype(nullptr) = nullptr) {}\n"
        "    basic_json(int) {}\n"
        "    basic_json(std::initializer_list<basic_json>) {}\n"
        "    basic_json(const basic_json&) = default;\n"
        "};\n"
        "} using json = basic_json<>; }\n" };

    "json-brace-init is caught when the file is parsed: one value in braces, and nothing else"_test = [] {
        Program p { "json" };
        const std::string text { std::string { JSON_PRELUDE } +
                                 "namespace app {\n"
                                 "nlohmann::json one { 1 };\n"                                        // the pitfall
                                 "nlohmann::json copy = 1;\n"
                                 "nlohmann::json literal { { 1, 2 } };\n"
                                 "nlohmann::json two { 1, 2 };\n"
                                 "struct S { nlohmann::json member { nullptr }; };\n"               // the pitfall, as a member
                                 "[[mcpp::allow(\"json-brace-init\", \"an array is meant\")]] nlohmann::json waived { 3 };\n"
                                 "}\n" };
        const std::string file { p.file("src/app.cpp", text) };
        auto w = workspace_for(p);
        auto unit = w->parse(file, text, 1);
        expect(fatal(unit != nullptr));
        std::vector<std::string> gates;
        for (const auto& d : unit->diagnostics()) {
            if (d.code == mcxx::plugins::json::BRACE_INIT) gates.push_back(std::format("{}:{}", d.range.begin.line, d.severity == msa::Severity::error ? "error" : "?"));
            else expect(d.severity != msa::Severity::error) << d.message;
        }
        const std::uint32_t base { static_cast<std::uint32_t>(std::ranges::count(JSON_PRELUDE, '\n')) };
        expect(gates == std::vector<std::string> { std::format("{}:error", base + 1), std::format("{}:error", base + 5) }) << gates.size();
        const auto& facts = unit->facts();
        const auto one = std::ranges::find_if(facts.initializations, [](const auto& i) { return i.variable == "app::one"; });
        expect(fatal(one != facts.initializations.end()));
        expect(one->type_template == "nlohmann::basic_json" && one->initializer_list_constructor && one->elements == 1 && !one->element_braced);
        expect(one->form == msa::fact::InitForm::direct_list && one->container == "app");
        const auto literal = std::ranges::find_if(facts.initializations, [](const auto& i) { return i.variable == "app::literal"; });
        expect(literal != facts.initializations.end() && literal->element_braced);
        expect(facts.suppressions.size() == 1 && facts.suppressions[0].reason == "an array is meant");
    };

    "mc++.iso's facts, and profile strict turning them into errors; a plugin's policy at its own level"_test = [] {
        Program p { "strict" };
        p.manifest("[package]\nname = \"t\"\nversion = \"0.1.0\"\n[package.metadata.mcxx]\nprofile = \"strict\"\n"
                   "[package.metadata.mcxx.features]\n\"lib:std.vector\" = \"deny\"\nraw-pointers = \"warn\"\n");
        std::ofstream { p.root / "src/local.h" } << "inline int local_value() { return 1; }\n";
        const std::string text { std::string { JSON_PRELUDE } +
                                 "#include \"local.h\"\n"
                                 "#define LIMIT 4\n"
                                 "union U { int i; float f; };\n"
                                 "struct P { int a; };\n"
                                 "int trace(const char* format, ...);\n"
                                 "using Callback = void (*)(int level);\n"
                                 "int sum(int* p, int n) {\n"
                                 "    int arr[LIMIT] = {};\n"
                                 "    std::vector<int> v;\n"
                                 "    int* q = new int(1);\n"
                                 "    long long bits = reinterpret_cast<long long>(q);\n"
                                 "    void* untyped = q;\n"
                                 "    int* back = static_cast<int*>(untyped);\n"
                                 "    const int* c = back;\n"
                                 "    int* w = const_cast<int*>(c);\n"
                                 "    int x;\n"
                                 "    P pt;\n"
                                 "    P zero {};\n"
                                 "    static int counted;\n"
                                 "    delete q;\n"
                                 "    if (n == 0) goto done;\n"
                                 "    return *(p + 1) + p[2] + arr[0] + (int)bits + *back + *w + x + pt.a + zero.a + counted + local_value();\n"
                                 "done:\n"
                                 "    asm volatile(\"nop\");\n"
                                 "    return 0;\n"
                                 "}\n" };
        const std::string file { p.file("src/strict.cpp", text) };
        auto w = workspace_for(p);
        auto unit = w->parse(file, text, 1);
        expect(fatal(unit != nullptr));
        const auto& f = unit->facts();
        expect(f.macros.size() == 1 && f.macros[0].name == "LIMIT");
        expect(f.gotos.size() == 1 && f.gotos[0].label == "done");
        expect(f.allocations.size() == 2) << f.allocations.size();
        expect(f.pointer_arithmetic.size() == 2) << "p + 1 and p[2]; arr[0] is a C array's own";
        expect(std::ranges::count_if(f.casts, [](const auto& c) { return c.reinterprets; }) == 1) << "static_cast from void* is not a reinterpretation";
        expect(f.includes.size() == 1 && f.includes[0].header == "\"local.h\"" && !f.includes[0].global_module_fragment);
        std::vector<std::string> indeterminate;
        for (const auto& i : f.initializations)
            if (i.indeterminate) indeterminate.push_back(i.variable);
        // The stand-in vector has a trivial constructor: `v` is as indeterminate as `pt`.
        expect(indeterminate == std::vector<std::string> { "v", "x", "pt" }) << std::format("{}: `P zero {{}}` is value-initialized, a static is zero-initialized", indeterminate);
        expect(std::ranges::count_if(f.declarations, [](const auto& d) { return d.c_variadic; }) == 1);
        expect(std::ranges::none_of(f.declarations, [](const auto& d) { return d.qualified_name == "level"; }))
            << "a parameter written in a function type is the type's, not a declaration of the file (MC3 §4.2)";
        expect(std::ranges::count_if(f.uses, [](const auto& u) { return u.construct == "asm"; }) == 1);
        std::vector<std::string> errors, warnings;
        for (const auto& d : unit->diagnostics()) (d.severity == msa::Severity::error ? errors : warnings).push_back(d.code);
        for (const auto* want : { "macros", "union", "c-array", "lib:std.vector", "new-delete", "reinterpret-cast", "goto", "raw-pointer-arithmetic",
                                  "include", "const-cast", "c-style-cast", "uninitialized", "c-varargs", "asm" })
            expect(std::ranges::find(errors, want) != errors.end()) << want;
        expect(std::ranges::find(warnings, "raw-pointers") != warnings.end());
    };


    "a safe module importing what its dialect denies is told so at the import, or waives it there (M1.2)"_test = [] {
        Program p { "boundary" };
        std::filesystem::create_directories(p.root / "legacy");
        std::filesystem::create_directories(p.root / "app");
        std::ofstream { p.root / "legacy/mcpp.toml" } << "[package]\nname = \"legacy\"\nversion = \"0.1.0\"\n";
        std::ofstream { p.root / "app/mcpp.toml" } << "[package]\nname = \"app\"\nversion = \"0.1.0\"\n[package.metadata.mcxx]\nprofile = \"safe\"\n"
                                                      "[package.metadata.mcxx.features]\nraw-pointers = \"deny\"\n";
        p.file("legacy/legacy.cppm", "export module legacy;\nexport int* address();\nexport using Table = int[3];\nexport int sum(int n, ...);\n"
                                     "int hidden[4];\n");
        const std::string plain_text { "import legacy;\nint use() { return 0; }\n" };
        const std::string plain { p.file("app/plain.cpp", plain_text) };
        const std::string waived_text { "import legacy [[mcpp::allow(\"c-array, raw-pointers, c-varargs\", \"a C library\")]];\nint use2() { return 0; }\n" };
        const std::string waived { p.file("app/waived.cpp", waived_text) };
        auto w = workspace_for(p);
        auto u = w->parse(plain, plain_text, 1);
        expect(fatal(u != nullptr));
        std::vector<std::string> errors;
        for (const auto& d : u->diagnostics())
            if (d.severity == msa::Severity::error) {
                errors.push_back(d.code);
                expect(d.range.begin.line == 0 && d.message.contains("import of legacy brings in")) << d.message;
            }
        std::ranges::sort(errors);
        expect(errors == std::vector<std::string> { "c-array", "c-varargs", "raw-pointers" }) << std::format("{}", errors);
        const auto& imports = u->facts().imports;
        expect(imports.size() == 1 && imports[0].module == "legacy" && imports[0].interfaces.size() == 1 && imports[0].interfaces[0].found);
        expect(imports.size() == 1 && std::ranges::none_of(imports[0].interfaces[0].exported, [](const auto& d) { return d.qualified_name == "hidden"; }))
            << "what the module does not export does not cross";
        auto v = w->parse(waived, waived_text, 1);
        expect(fatal(v != nullptr));
        std::vector<std::string> left;
        for (const auto& d : v->diagnostics()) left.push_back(std::format("{} {}", d.code, d.message));
        expect(left.empty()) << std::format("waived on the import, and Clang never sees the attribute: {}", left);
    };

    "an editor's parse of an interface writes no .ifc: only a compile that writes a BMI does (MC2-2-1)"_test = [] {
        Program p { "noifc" };
        const std::string text { "export module lonely;\nexport int lonely_value() { return 1; }\n" };
        const std::string file { p.file("src/lonely.cppm", text) };
        auto w = workspace_for(p);
        auto unit = w->parse(file, text, 1);
        expect(fatal(unit != nullptr));
        expect(!std::filesystem::exists("lonely.ifc") && !std::filesystem::exists(p.root / "lonely.ifc") && !std::filesystem::exists(p.root / "src/lonely.ifc"))
            << "Clang derives a module output path for a .cppm given with -c; the editor writes nothing there";
    };

    "an unnamed class's name and type do not say where the file is (MC3-4-4)"_test = [] {
        Program p { "unnamed" };
        const std::string text { "struct Box { union { int i; char c[4]; }; };\nstruct { int a; } single;\nenum { red } color;\n"
                                 "namespace outer { namespace { int hidden; } }\n" };
        const std::string file { p.file("src/unnamed.cpp", text) };
        auto w = workspace_for(p);
        auto unit = w->parse(file, text, 1);
        expect(fatal(unit != nullptr));
        std::vector<std::string> names, types;
        for (const auto& d : unit->facts().declarations) {
            names.push_back(d.qualified_name);
            if (!d.type.empty()) types.push_back(d.type);
        }
        expect(std::ranges::find(names, "Box::(anonymous union)") != names.end()) << std::format("{}", names);
        expect(std::ranges::find(names, "outer::(anonymous namespace)") != names.end() && std::ranges::find(names, "outer::(anonymous namespace)::hidden") != names.end())
            << "an unnamed namespace is named as in its members' names";
        for (const auto& d : unit->facts().declarations)
            if (d.qualified_name == "outer::(anonymous namespace)::hidden") expect(d.container == "outer::(anonymous namespace)") << d.container;
        for (const auto& t : types) expect(!t.contains(" at ") && !t.contains("unnamed.cpp")) << t;
        for (const auto& n : names) expect(!n.contains(" at ") && !n.contains("unnamed.cpp")) << n;
    };

    "[[mcpp::cfg]]: the editor sees the target's declarations only, at their own positions"_test = [] {
        Program p { "cfg" };
        const std::string text {
            "namespace plat {\n"
            "[[mcpp::cfg(windows)]] int console() { return 1; }\n"
            "[[mcpp::cfg(not(windows))]] int console() { return 0; }\n"
            "[[mcpp::cfg(colour = \"red\")]] int broken;\n"
            "}\n"
            "int main() { return plat::console(); }\n" };
        const std::string file { p.file("src/plat.cpp", text) };
        auto w = workspace_for(p);
        auto unit = w->parse(file, text, 1);
        expect(fatal(unit != nullptr));
        std::vector<std::string> errors;
        for (const auto& d : unit->diagnostics())
            if (d.severity == msa::Severity::error) errors.push_back(std::format("{}: {}", d.range.begin.line, d.message));
        expect(errors.size() == 1 && errors[0].starts_with("3: ") && errors[0].contains("unknown key `colour`")) << (errors.empty() ? "none" : errors[0]);
        const auto console = unit->entity_at(*find(text, "console();", 0));
        expect(fatal(console.has_value()));
        // The target's own one, where it is written: this program's target is the editor's.
        constexpr bool windows { mcxx::os::FAMILY == mcxx::os::Family::windows };
        expect(console->definition && console->definition->range.begin.line == (windows ? 1u : 2u)) << "the target's one, where it is written";
    };


    "a package's out-of-process plugin gates its files; a static plugin not composed in is said"_test = [] {
        Program p { "plugins" };
        // A plugin in shell (MC4 protocol 1): it welcomes, then answers each check with one finding.
        std::ofstream { p.root / "lint.sh" }
            << "#!/bin/sh\nread l\n"
               "echo '{\"type\":\"welcome\",\"id\":0,\"protocol\":1,\"providers\":[{\"name\":\"acme.lint\",\"extension-points\":[\"rule\"],"
               "\"features\":[{\"id\":\"acme-lint\",\"category\":\"policy\",\"summary\":\"s\",\"default\":\"deny\",\"waivable\":true,"
               "\"needs\":[\"declarations\"]}],\"profiles\":[],\"replaces\":[]}]}'\n"
               "n=1\nwhile read l; do echo \"{\\\"type\\\":\\\"findings\\\",\\\"id\\\":$n,\\\"findings\\\":[{\\\"feature\\\":\\\"acme-lint\\\","
               "\\\"range\\\":{\\\"begin\\\":{\\\"line\\\":0,\\\"column\\\":4},\\\"end\\\":{\\\"line\\\":0,\\\"column\\\":9}},"
               "\\\"message\\\":\\\"linted\\\",\\\"container\\\":\\\"\\\"}]}\"; n=$((n+1)); done\n";
        p.manifest("[package]\nname = \"t\"\nversion = \"0.1.0\"\n[package.metadata.mcxx.plugins]\n"
                   "lint = { command = [\"/bin/sh\", \"lint.sh\"] }\nacme-rules = { path = \"tools/acme-rules\" }\n");
        const std::string text { "int value = 1;\n" };
        const std::string file { p.file("src/a.cpp", text) };
        auto w = workspace_for(p);
        auto unit = w->parse(file, text, 1);
        expect(fatal(unit != nullptr));
        const msa::Diagnostic* lint { nullptr };
        const msa::Diagnostic* composed { nullptr };
        for (const auto& d : unit->diagnostics()) {
            if (d.code == "acme-lint") lint = &d;
            if (d.code == "mcxx-plugin" && d.message.contains("acme-rules")) composed = &d;
        }
        std::string all;
        for (const auto& d : unit->diagnostics()) all += std::format("[{}] {}\n", d.code, d.message);
        // The out-of-process plugin is a shell script: a POSIX system's.
        if constexpr (mcxx::os::FAMILY != mcxx::os::Family::windows)
            expect(lint != nullptr && lint->severity == msa::Severity::error && lint->range.begin.column == 4) << "the plugin's finding, at its level: " << all;
        expect(composed != nullptr && composed->severity == msa::Severity::warning && composed->message.contains("mcxx compose"))
            << "the editor cannot compose: a warning: " << all;
    };


    "claimed attributes: [[acme::hot]]'s rule reads its declaration, [[acme::device]] is a region"_test = [] {
        Program p { "attributes" };
        p.manifest("[package]\nname = \"t\"\nversion = \"0.1.0\"\n");
        const std::string text { "[[acme::hot]] int on_frame() { int* p = new int(1); int v = *p; delete p; return v; }\n"
                                 "[[acme::device]] int kernel(int n) {\n"
                                 "    if (n < 0) throw n;\n"
                                 "    return n;\n"
                                 "}\n"
                                 "int host(int n) { if (n < 0) throw n; return n; }\n"
                                 "[[acme::hot(\"frame\", 3)]] int quiet() { return 0; }\n" };
        const std::string file { p.file("src/a.cpp", text) };
        auto w = workspace_for(p);
        auto unit = w->parse(file, text, 1);
        expect(fatal(unit != nullptr));
        const auto& f = unit->facts();
        expect(fatal(f.attributes.size() == 3)) << f.attributes.size();
        expect(f.attributes[0].name == "acme::hot" && f.attributes[0].declaration == "on_frame");
        expect(f.attributes[1].name == "acme::device" && f.attributes[1].range.begin.line == 1 && f.attributes[1].range.end.line == 4);
        expect(f.attributes[2].arguments == std::vector<std::string> { "frame", "3" });
        std::vector<std::pair<std::string, std::uint32_t>> found;
        std::string all;
        for (const auto& d : unit->diagnostics()) {
            all += std::format("[{}] {}:{}\n", d.code, d.range.begin.line, d.message.substr(0, 80));
            if (d.severity == msa::Severity::error) found.emplace_back(d.code, d.range.begin.line);
            expect(!d.message.contains("unknown attribute")) << "a claimed attribute is known";
        }
        expect(std::ranges::count(found, std::pair<std::string, std::uint32_t> { "acme-hot-alloc", 0 }) == 1) << all;
        expect(std::ranges::count(found, std::pair<std::string, std::uint32_t> { "exceptions", 2 }) == 1) << "the region denies throw: " << all;
        expect(std::ranges::none_of(found, [](const auto& x) { return x.second == 5; })) << "outside the region, the package's levels: " << all;
    };


    "new commands while an interface is being built: the build is redone, never taken as ready"_test = [] {
        // The shape of mcppls's plan changing under a build: a provisional plan (-std=c++26), then the
        // build tool's (-std=c++23). An interface built for the first must not reach the second's importers.
        for (int round { 0 }; round < 3; ++round) {
            Program p { "replan" };
            std::string body;
            for (int i { 0 }; i < 400; ++i) body += std::format("export constexpr int v{} = {};\n", i, i);
            const std::string lib { p.file("src/lib.cppm", "export module lib;\n" + body) };
            const std::string text { "import lib;\nint main() { return v1; }\n" };
            const std::string main { p.file("src/main.cpp", text) };
            auto first = p.commands;
            for (auto& c : first) c.arguments[1] = "-std=c++26";
            msa::Workspace::Options options;
            options.cache_directory = (p.root / ".cache").generic_string();
            options.workers = 2;
            options.background_index = true;   // the index starts building lib under the first plan
            auto w = mcxx::backend::clang::make_workspace(std::move(options));
            w->set_commands(first);
            std::this_thread::sleep_for(std::chrono::milliseconds { 5 * round });
            w->set_commands(p.commands);
            auto unit = w->parse(main, text, 1);
            expect(fatal(unit != nullptr));
            expect(unit->diagnostics().empty()) << (unit->diagnostics().empty() ? std::string {} : unit->diagnostics().front().message);
        }
    };

    return report();
}
