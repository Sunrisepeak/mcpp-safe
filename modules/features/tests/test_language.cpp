// MC4 0.4.0: language providers -- the compiler arguments a file's enabled language features need,
// asked with the file's levels (profiles, the package, file globs).
import std;
import mcxx.testing;
import mcxx.plugin;
import mcxx.features;

namespace plugin = mcxx::plugin;

namespace {

// A language feature off unless a configuration turns it on (as ML's C++26 features are), and the
// argument that turns it on in the compiler.
struct Lang final : plugin::LanguageProvider {
    std::vector<plugin::Feature> features_ { plugin::Feature { .id = "ext:test-lang", .category = plugin::Category::extension,
                                                               .summary = "a language feature for the test", .default_level = plugin::Level::deny } };
    std::string_view name() const override { return "test.lang"; }
    std::span<const plugin::Feature> features() const override { return features_; }
    std::vector<std::string> arguments(const plugin::LanguageContext& context) const override {
        if (!context.enabled("ext:test-lang")) return {};
        return { "-DTEST_LANG=1", "--target-was=" + context.target.os };
    }
};
plugin::Registration<Lang> registration;

struct Scratch {
    std::filesystem::path root;
    Scratch() {
        root = std::filesystem::temp_directory_path() / std::format("mcxx-language-{}", std::random_device {}());
        std::filesystem::create_directories(root / "src/legacy");
        std::ofstream { root / "mcpp.toml" } << "[package]\nname = \"p\"\nversion = \"0.1.0\"\n"
                                                 "[package.metadata.mcxx.features]\n\"ext:test-lang\" = \"allow\"\n"
                                                 "[package.metadata.mcxx.files.\"src/legacy/**\"]\n\"ext:test-lang\" = \"deny\"\n";
        std::ofstream { root / "src/a.cpp" } << "int a;\n";
        std::ofstream { root / "src/legacy/b.cpp" } << "int b;\n";
    }
    ~Scratch() {
        std::error_code ec;
        std::filesystem::remove_all(root, ec);
    }
};

} // namespace

int main() {
    using namespace mcxx::testing;

    "a language provider's arguments are added where its feature is enabled, and nowhere else"_test = [] {
        const Scratch s;
        const plugin::Target linux { .triple = "x86_64-unknown-linux-gnu", .os = "linux" };
        const auto enabled = mcxx::features::language_arguments((s.root / "src/a.cpp").generic_string(), linux);
        expect(enabled == std::vector<std::string> { "-DTEST_LANG=1", "--target-was=linux" })
            << std::format("{} arguments", enabled.size());
        // A file glob denies it for src/legacy/**: no arguments there.
        const auto denied = mcxx::features::language_arguments((s.root / "src/legacy/b.cpp").generic_string(), linux);
        expect(denied.empty()) << std::format("{} arguments under src/legacy", denied.size());
    };

    "outside any package the feature's default level decides: deny, so no arguments"_test = [] {
        const plugin::Target linux { .triple = "x86_64-unknown-linux-gnu", .os = "linux" };
        const auto none = mcxx::features::language_arguments("/nonexistent-mcxx-dir/x.cpp", linux);
        expect(none.empty()) << std::format("{} arguments", none.size());
    };

    "the catalog lists the language provider"_test = [] {
        const auto catalog = plugin::catalog();
        expect(catalog->languages.size() == 1 && catalog->languages[0]->name() == "test.lang");
    };

    return report();
}
