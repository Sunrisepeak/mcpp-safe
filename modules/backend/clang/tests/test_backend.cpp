// mcxx.backend.clang against a small modules program written to a scratch directory: no standard library,
// so it needs no toolchain beyond libmc++ itself and runs in seconds. What each case pins down is a
// behaviour mcppls depends on.
import std;
import mcxx.testing;
import mcxx.msa;
import mcxx.backend.clang;
import mcxx.plugins.json;
import mcxx.plugins.std;

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
        const std::string p { path.generic_string() };
        commands.push_back({ root.generic_string(), p, { "clang++", "-std=c++23", "-c", p } });
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
        "    const E* b; unsigned long n;\n"
        "    constexpr initializer_list(const E* b, unsigned long n) : b(b), n(n) {}\n"
        "public:\n"
        "    constexpr initializer_list() : b(nullptr), n(0) {}\n"
        "    constexpr unsigned long size() const { return n; }\n"
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

    "mc++.safe's facts, and its profile turning them into errors"_test = [] {
        Program p { "safe" };
        p.manifest("[package]\nname = \"t\"\nversion = \"0.1.0\"\n[package.metadata.mcxx]\nprofile = \"safe\"\n");
        const std::string text { std::string { JSON_PRELUDE } +
                                 "#define LIMIT 4\n"
                                 "union U { int i; float f; };\n"
                                 "int sum(int* p, int n) {\n"
                                 "    int arr[LIMIT] = {};\n"
                                 "    std::vector<int> v;\n"
                                 "    int* q = new int(1);\n"
                                 "    long bits = reinterpret_cast<long>(q);\n"
                                 "    void* untyped = q;\n"
                                 "    int* back = static_cast<int*>(untyped);\n"
                                 "    delete q;\n"
                                 "    if (n == 0) goto done;\n"
                                 "    return *(p + 1) + p[2] + arr[0] + (int)bits + *back;\n"
                                 "done:\n"
                                 "    return 0;\n"
                                 "}\n" };
        const std::string file { p.file("src/safe.cpp", text) };
        auto w = workspace_for(p);
        auto unit = w->parse(file, text, 1);
        expect(fatal(unit != nullptr));
        const auto& f = unit->facts();
        expect(f.macros.size() == 1 && f.macros[0].name == "LIMIT");
        expect(f.gotos.size() == 1 && f.gotos[0].label == "done");
        expect(f.allocations.size() == 2) << f.allocations.size();
        expect(f.pointer_arithmetic.size() == 2) << "p + 1 and p[2]; arr[0] is a C array's own";
        expect(std::ranges::count_if(f.casts, [](const auto& c) { return c.reinterprets; }) == 1) << "static_cast from void* is not a reinterpretation";
        std::vector<std::string> codes;
        for (const auto& d : unit->diagnostics())
            if (d.severity == msa::Severity::error) codes.push_back(d.code);
        for (const auto* want : { "macros", "union", "c-array", "lib:std.vector", "new-delete", "reinterpret-cast", "goto", "raw-pointer-arithmetic" })
            expect(std::ranges::find(codes, want) != codes.end()) << want;
    };


    "[[mcpp::cfg]]: the editor sees the target's declarations only, at their own positions"_test = [] {
        Program p { "cfg" };
        const std::string text {
            "namespace plat {\n"
            "[[mcpp::cfg(windows)]] int console() { return AllocConsole(); }\n"
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
        expect(console->definition && console->definition->range.begin.line == 2) << "the non-Windows one, where it is written";
    };

    return report();
}
