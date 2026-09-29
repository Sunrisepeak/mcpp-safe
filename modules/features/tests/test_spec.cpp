// MC1 A0.3.2: the configuration read from the specification's example manifest is, setting by
// setting, what specs/examples/mc1-config.json says it is (tools/checks/specs.py checks that JSON
// against the TOML and the schema).
import std;
import mcxx.testing;
import mcxx.plugin;
import mcxx.features;

namespace features = mcxx::features;
using mcxx::plugin::Level;

namespace {

std::filesystem::path repository() {
    // tests/test_spec.cpp -> modules/features -> modules -> the repository
    return std::filesystem::path { std::source_location::current().file_name() }.parent_path().parent_path().parent_path().parent_path();
}

} // namespace

int main() {
    using namespace mcxx::testing;
    const auto manifest = repository() / "specs/examples/mc1-config.toml";

    "the example manifest is found and read without problems"_test = [&] {
        expect(fatal(std::filesystem::exists(manifest))) << manifest.generic_string();
        const auto config = features::read_config(manifest.generic_string());
        expect(config.problems.empty()) << (config.problems.empty() ? "" : config.problems.front());
    };

    "every setting of the example, as the specification's JSON form states it"_test = [&] {
        const auto c = features::read_config(manifest.generic_string());
        expect(c.profiles == std::vector<std::string> { "safe", "modules" });
        const std::map<std::string, Level, std::less<>> package { { "json-brace-init", Level::deny }, { "exceptions", Level::deny }, { "rtti", Level::warn },
                                                                  { "raw-pointers", Level::warn }, { "lib:std.vector", Level::allow } };
        expect(c.package == package);
        expect(c.modules.size() == 2 && c.modules.at("app.legacy") == features::LevelMap { { "goto", Level::allow }, { "c-array", Level::warn } });
        expect(c.modules.at("app.legacy:io") == features::LevelMap { { "c-array", Level::allow } });
        expect(c.namespaces.size() == 1 && c.namespaces.at("app::ffi") == features::LevelMap { { "raw-pointers", Level::allow }, { "c-varargs", Level::allow } });
        expect(c.imports.size() == 1 && c.imports.at("vendor.zlib").ids == std::vector<std::string> { "c-array", "raw-pointers" }
               && c.imports.at("vendor.zlib").reason.contains("zlib"));
        expect(c.problems.empty()) << std::format("{}", c.problems);
    };

    "keys this version does not define are ignored, not errors"_test = [] {
        const auto c = features::parse_config("[package]\nname = \"p\"\n[package.metadata.mcxx]\nprofile = \"safe\"\nfuture-key = 1\n"
                                              "[package.metadata.mcxx.plugins]\nacme = { path = \"x\" }\n");
        expect(c.problems.empty() && c.profiles == std::vector<std::string> { "safe" });
    };

    "a plugin entry is static or out of process, exactly one; the specification's example reads"_test = [&] {
        const auto example = features::read_config((repository() / "specs/examples/mc4-plugins.toml").generic_string());
        expect(example.problems.empty() && example.plugins.size() == 3) << (example.problems.empty() ? "" : example.problems.front());
        const auto lint = std::ranges::find(example.plugins, "gcc-lint", &features::PluginEntry::name);
        expect(lint != example.plugins.end() && !lint->is_static() && lint->timeout == std::chrono::milliseconds { 5000 });
        const auto both = features::parse_config("[package]\nname = \"p\"\n[package.metadata.mcxx.plugins]\nx = { path = \"a\", command = [\"b\"] }\n");
        const auto neither = features::parse_config("[package]\nname = \"p\"\n[package.metadata.mcxx.plugins]\nx = { timeout-ms = 5 }\n");
        expect(both.plugins.empty() && both.problems.size() == 1 && both.problems[0].contains("MC4-3-2"));
        expect(neither.plugins.empty() && neither.problems.size() == 1);
    };

    "the example's precedence: partition over module over profiles; a namespace over the package"_test = [&] {
        const auto plan = features::make_plan(features::read_config(manifest.generic_string()));
        const auto* array = plan.gate("c-array");
        expect(fatal(array != nullptr));
        expect(plan.level(*array, "", "") == Level::deny) << "safe";
        expect(plan.level(*array, "app.legacy", "") == Level::warn);
        expect(plan.level(*array, "app.legacy:io", "") == Level::allow);
        const auto* include = plan.gate("include");
        expect(fatal(include != nullptr));
        expect(plan.level(*include, "", "") == Level::deny) << "modules";
        const auto* varargs = plan.gate("c-varargs");
        expect(fatal(varargs != nullptr));
        expect(plan.level(*varargs, "", "app::ffi::detail") == Level::allow && plan.level(*varargs, "", "app") == Level::deny);
    };

    return report();
}
