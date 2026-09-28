// mcxx.clang against a small modules program written to a scratch directory: no standard library,
// so it needs no toolchain beyond libmc++ itself and runs in seconds. What each case pins down is a
// behaviour mcppls depends on.
import std;
import mcxx.testing;
import mcxx.msa;
import mcxx.clang;

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
    auto w = mcxx::clang::make_workspace(std::move(options));
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

    return report();
}
