// Out-of-process plugins, end to end: this test program is its own plugin (`--serve`), so the same
// rule runs in process and out of process on the same facts (MC4-6.3-3); shell scripts inject the
// failures (a crash, a hang, garbage, another protocol) that must be reported, not crash or hang.
import std;
import nlohmann.json;
import mcxx.testing;
import mcxx.msa;
import mcxx.plugin;
import mcxx.plugin.wire;
import mcxx.plugin.remote;
import mcxx.plugin.host;
import mcxx.features;

namespace msa = mcxx::msa;
namespace fact = mcxx::msa::fact;
namespace plugin = mcxx::plugin;
namespace host = mcxx::plugin::host;
namespace features = mcxx::features;
namespace fs = std::filesystem;

namespace {

struct Naming final : plugin::Rule {
    std::vector<plugin::Feature> fs_ { plugin::Feature { .id = "acme-snake-case", .category = plugin::Category::policy, .summary = "not snake_case",
                                                        .default_level = plugin::Level::warn, .needs = fact::Kinds::declarations } };
    std::string_view name() const override { return "acme.naming"; }
    std::span<const plugin::Feature> features() const override { return fs_; }
    void check(const plugin::Context& c, std::vector<plugin::Finding>& out) const override {
        for (const auto& d : c.facts.declarations)
            if (std::ranges::any_of(d.qualified_name, [](char ch) { return ch >= 'A' && ch <= 'Z'; }))
                out.push_back({ "acme-snake-case", d.name, std::format("`{}` is not snake_case", d.qualified_name), d.container });
    }
};

fact::Facts facts() {
    fact::Facts f;
    for (std::uint32_t line { 0 }; const char* name : { "app::good_name", "app::BadName", "app::other", "app::AlsoBad" }) {
        fact::Declaration d { { { { line, 0 }, { line, 20 } }, "app" } };
        d.name = { { line, 4 }, { line, 12 } };
        d.qualified_name = name;
        f.declarations.push_back(d);
        ++line;
    }
    return f;
}

fs::path scratch() {
    static const fs::path dir { fs::temp_directory_path() / std::format("mcxx-host-{}", std::random_device {}()) };
    fs::create_directories(dir);
    return dir;
}

// A plugin in shell: `lines` are answered in order, one per request read.
std::vector<std::string> script(std::string_view name, std::string_view body) {
    const fs::path p { scratch() / name };
    std::ofstream { p } << "#!/bin/sh\n" << body;
    return { "/bin/sh", p.generic_string() };
}

std::string welcome(std::string_view provider, std::string_view feature, std::string_view level) {
    return std::format(R"({{"type":"welcome","id":0,"protocol":1,"providers":[{{"name":"{}","extension-points":["rule"],"features":[{{"id":"{}","category":"policy","summary":"s","default":"{}","waivable":true,"needs":["declarations"]}}],"profiles":[],"replaces":[]}}]}})",
                       provider, feature, level);
}

features::Config with(std::string name, std::vector<std::string> command, std::chrono::milliseconds timeout = std::chrono::milliseconds { 3000 }) {
    features::Config c;
    c.plugins.push_back({ .name = std::move(name), .command = std::move(command), .timeout = timeout });
    return c;
}

std::vector<msa::Diagnostic> gate(std::string_view feature_level_toml = "") {
    const auto config = features::parse_config(std::format("[package]\nname = \"p\"\n{}", feature_level_toml));
    return features::evaluate({ "/nowhere/src/a.cpp", "", facts() }, config).diagnostics;
}

const msa::Diagnostic* coded(const std::vector<msa::Diagnostic>& ds, std::string_view code, std::string_view contains = "") {
    for (const auto& d : ds)
        if (d.code == code && d.message.contains(contains)) return &d;
    return nullptr;
}

} // namespace

int main(int argc, char** argv) {
    if (argc > 1 && std::string_view { argv[1] } == "--serve") {
        plugin::register_rule(std::make_unique<Naming>());
        return mcxx::plugin::remote::serve();
    }
    using namespace mcxx::testing;
    const std::string self { fs::read_symlink("/proc/self/exe").generic_string() };

    "a plugin out of process finds what the same rule finds in process"_test = [&] {
        std::vector<plugin::Finding> direct;
        Naming {}.check({ "/nowhere/src/a.cpp", "", facts() }, direct);
        expect(fatal(direct.size() == 2));
        const auto problems = host::load(with("naming", { self, "--serve" }));
        expect(fatal(problems.empty())) << (problems.empty() ? "" : problems.front());
        const auto* e = plugin::catalog()->find("acme-snake-case");
        expect(fatal(e != nullptr)) << "the plugin's feature is in the catalog";
        expect(e->provider->name() == "acme.naming" && e->origin == plugin::Origin::plugin);
        const auto ds = gate("[package.metadata.mcxx.features]\nacme-snake-case = \"deny\"\n");
        std::vector<std::string> remote, local;
        for (const auto& d : ds)
            if (d.code == "acme-snake-case") remote.push_back(std::format("{}:{}", d.range.begin.line, d.message.substr(0, d.message.find(" ["))));
        for (const auto& f : direct) local.push_back(std::format("{}:{}", f.range.begin.line, f.message));
        expect(remote == local) << std::format("{} out of process, {} in process", remote.size(), local.size());
        expect(host::load(with("naming", { self, "--serve" })).empty()) << "loaded once per package and name";
    };

    "a plugin that crashes on a request is reported at the file, an error where its feature is denied"_test = [] {
        const auto problems = host::load(with("crashy", script("crash.sh", "read l\necho '" + welcome("crashy", "acme-crash", "deny") + "'\nread l\nexit 3\n")));
        expect(fatal(problems.empty())) << (problems.empty() ? "" : problems.front());
        const auto ds = gate();
        const auto* d = coded(ds, "mcxx-plugin", "crashy");
        expect(fatal(d != nullptr));
        expect(d->severity == msa::Severity::error && d->message.contains("exited with status 3") && d->message.contains("acme-crash")) << d->message;
        const auto again = gate();
        const auto* d2 = coded(again, "mcxx-plugin", "crashy");
        expect(d2 != nullptr && d2->severity == msa::Severity::error) << "every later file too";
    };

    "a plugin that does not answer in time is killed and reported; a warn feature's failure is a warning"_test = [] {
        const auto problems = host::load(with("hangs", script("hang.sh", "read l\necho '" + welcome("hangs", "acme-hang", "warn") + "'\nread l\nsleep 30\n"),
                                              std::chrono::milliseconds { 300 }));
        expect(fatal(problems.empty()));
        const auto start = std::chrono::steady_clock::now();
        const auto ds = gate();
        const auto took = std::chrono::steady_clock::now() - start;
        const auto* d = coded(ds, "mcxx-plugin", "hangs");
        expect(fatal(d != nullptr));
        expect(d->severity == msa::Severity::warning && d->message.contains("did not answer in 300 ms")) << d->message;
        expect(took < std::chrono::seconds { 3 }) << "the host waited for the time limit only";
    };

    "a plugin that cannot shake hands is a problem of its package, and nothing it gates is registered"_test = [] {
        const auto garbage = host::load(with("garbage", script("garbage.sh", "read l\necho 'hello, world'\n")));
        expect(garbage.size() == 1 && garbage[0].contains("garbage") && garbage[0].contains("not an MC4 message")) << (garbage.empty() ? "" : garbage[0]);
        const auto future = host::load(with("future", script("future.sh",
            "read l\necho '{\"type\":\"error\",\"id\":0,\"code\":\"protocol\",\"message\":\"MC4 protocol 2 only\",\"protocols\":[2]}'\n")));
        expect(future.size() == 1 && future[0].contains("protocol 2 only")) << (future.empty() ? "" : future[0]);
        const auto missing = host::load(with("missing", { "/nonexistent/plugin" }));
        expect(missing.size() == 1 && missing[0].contains("could not be started"));
        expect(host::load(with("garbage", { "/bin/true" })).size() == 1) << "the problem is remembered, not retried per file";
    };

    "a proxy answers only for its own package's files"_test = [&] {
        const fs::path owner { scratch() / "owner" };
        fs::create_directories(owner / "src");
        std::ofstream { owner / "mcpp.toml" } << "[package]\nname = \"owner\"\n";
        features::Config c { with("scoped", script("scoped.sh", "read l\necho '" + welcome("scoped", "acme-scoped", "deny") + "'\nread l\nexit 1\n")) };
        c.manifest = (owner / "mcpp.toml").generic_string();
        expect(fatal(host::load(c).empty()));
        const auto ds = gate();
        expect(coded(ds, "mcxx-plugin", "scoped") == nullptr) << "a file of no package is not the plugin's";
        const auto mine = features::evaluate({ (owner / "src/a.cpp").generic_string(), "", facts() }, features::Config {}).diagnostics;
        expect(coded(mine, "mcxx-plugin", "scoped") != nullptr) << "its package's file is";
    };

    std::error_code ec;
    fs::remove_all(scratch(), ec);
    return report();
}
