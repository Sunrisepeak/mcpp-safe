// mc++.policy over facts: raw-pointers as a package's choice, lib:std.vector as library control,
// and ext:cfg (an extension) denied by profile portable.
import std;
import mcxx.testing;
import mcxx.msa;
import mcxx.plugin;
import mcxx.features;
import mcxx.plugins.std;

namespace msa = mcxx::msa;
namespace fact = mcxx::msa::fact;

namespace {

msa::Range at(std::uint32_t line) { return { { line, 0 }, { line, 5 } }; }

fact::Declaration declaration(std::uint32_t line, std::string name, std::string type, std::string container = "") {
    fact::Declaration d { { at(line), std::move(container) } };
    d.name = at(line);
    d.qualified_name = std::move(name);
    d.type = std::move(type);
    return d;
}

fact::Facts facts() {
    fact::Facts f;
    auto p = declaration(1, "p", "int *");
    p.pointer = true;
    f.declarations.push_back(p);
    auto r = declaration(2, "r", "int &");
    f.declarations.push_back(r);
    auto v = declaration(3, "items", "std::vector<int>", "app");
    v.templates = { "std::vector" };
    f.declarations.push_back(v);
    auto detail = declaration(4, "app::detail::q", "char *", "app::detail");
    detail.pointer = true;
    f.declarations.push_back(detail);
    return f;
}

std::vector<std::string> codes(const mcxx::features::Result& r) {
    std::vector<std::string> out;
    for (const auto& d : r.diagnostics) out.push_back(d.code);
    return out;
}

mcxx::features::Config config(std::string_view metadata) {
    return mcxx::features::parse_config(std::format("[package]\nname = \"p\"\n{}", metadata));
}

} // namespace

int main() {
    using namespace mcxx::testing;
    const fact::Facts f { facts() };

    "allowed by default, and in no built-in profile: a package decides"_test = [&] {
        expect(mcxx::features::evaluate({ "/p/a.cpp", "", f }, mcxx::features::Config {}).diagnostics.empty());
        expect(mcxx::features::evaluate({ "/p/a.cpp", "", f }, config("[package.metadata.mcxx]\nprofile = \"strict\"\n")).diagnostics.empty());
    };

    "raw-pointers = deny: every declaration holding a pointer, a reference is not one; a namespace may allow it"_test = [&] {
        const auto c = config("[package.metadata.mcxx.features]\nraw-pointers = \"deny\"\n[package.metadata.mcxx.namespaces.\"app::detail\"]\nraw-pointers = \"allow\"\n");
        const auto result = mcxx::features::evaluate({ "/p/a.cpp", "", f }, c);
        expect(codes(result) == std::vector<std::string> { "raw-pointers" }) << result.diagnostics.size();
        expect(result.diagnostics.size() == 1 && result.diagnostics[0].range == at(1) && result.diagnostics[0].severity == msa::Severity::error);
    };

    "lib:std.vector is library control, asked for by name"_test = [&] {
        const auto result = mcxx::features::evaluate({ "/p/a.cpp", "", f }, config("[package.metadata.mcxx.features]\n\"lib:std.vector\" = \"warn\"\n"));
        expect(codes(result) == std::vector<std::string> { "lib:std.vector" });
    };

    "the catalog: categories, providers, the cfg filter's extension feature"_test = [] {
        const auto catalog = mcxx::plugin::catalog();
        expect(catalog->problems.empty()) << (catalog->problems.empty() ? "" : catalog->problems.front());
        const auto* pointers = catalog->find("raw-pointers");
        const auto* cfg = catalog->find("ext:cfg");
        const auto* goto_ = catalog->find("goto");
        expect(fatal(pointers != nullptr && cfg != nullptr && goto_ != nullptr));
        expect(pointers->feature->category == mcxx::plugin::Category::policy && pointers->provider->name() == "mc++.policy");
        expect(cfg->feature->category == mcxx::plugin::Category::extension);
        expect(goto_->origin == mcxx::plugin::Origin::builtin && goto_->feature->standard == "[stmt.goto]");
    };

    "profile portable denies the extension's uses, which a filter reports"_test = [] {
        const mcxx::plugin::Target target { .triple = "x86_64-unknown-linux-gnu", .os = "linux", .family = "unix", .arch = "x86_64" };
        const auto filtered = mcxx::plugin::apply_source_filters({ "/p/a.cpp", target }, "[[mcpp::cfg(unix)]] int f();\n");
        expect(fatal(filtered.findings.size() == 1));
        const fact::Facts none;
        const auto plan = mcxx::features::make_plan(config("[package.metadata.mcxx]\nprofile = \"portable\"\n"));
        const auto result = mcxx::features::evaluate({ "/p/a.cpp", "", none }, plan, filtered.findings);
        expect(codes(result) == std::vector<std::string> { "ext:cfg" });
        expect(mcxx::features::evaluate({ "/p/a.cpp", "", none }, mcxx::features::make_plan({}), filtered.findings).diagnostics.empty());
    };

    return report();
}
