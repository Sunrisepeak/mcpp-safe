// MC++ is a plugin system of its own: a plugin replaces a built-in feature, stands in for a whole
// provider, redefines a profile; a second provider of an id that does not say so is a conflict.
import std;
import mcxx.testing;
import mcxx.msa;
import mcxx.plugin;
import mcxx.features;

namespace msa = mcxx::msa;
namespace fact = mcxx::msa::fact;
namespace plugin = mcxx::plugin;

namespace {

// A better `uninitialized`: say, one that knows which locals are written before they are read.
struct FlowUninitialized final : plugin::Rule {
    std::vector<plugin::Feature> features_ { plugin::Feature { .id = "uninitialized", .category = plugin::Category::iso,
                                                               .standard = "[basic.indet]", .summary = "read before written",
                                                               .profiles = { { "safe", plugin::Level::deny } },
                                                               .needs = fact::Kinds::initializations, .replaces = true } };
    std::vector<plugin::Profile> profiles_ { plugin::Profile { .name = "modules", .summary = "and no macros either",
                                                               .categories = {}, .replaces = true } };
    std::string_view name() const override { return "flow.uninitialized"; }
    std::span<const plugin::Feature> features() const override { return features_; }
    std::span<const plugin::Profile> profiles() const override { return profiles_; }
    void check(const plugin::Context& c, std::vector<plugin::Finding>& out) const override {
        for (const auto& i : c.facts.initializations)
            if (i.indeterminate && i.variable != "written_first") out.push_back({ "uninitialized", i.name, "read before it is written", i.container });
    }
};

// Claims `goto` without saying it replaces it: a conflict, and not used.
struct Squatter final : plugin::Rule {
    std::vector<plugin::Feature> features_ { plugin::Feature { .id = "goto", .category = plugin::Category::policy } };
    std::string_view name() const override { return "squatter"; }
    std::span<const plugin::Feature> features() const override { return features_; }
    void check(const plugin::Context&, std::vector<plugin::Finding>& out) const override { out.push_back({ "goto", {}, "squatted", "" }); }
};

// Registered late (in the test), standing in for the squatter entirely.
struct Late final : plugin::Rule {
    static constexpr std::string_view replaced[] { "squatter" };
    std::string_view name() const override { return "late"; }
    std::span<const std::string_view> replaces() const override { return replaced; }
    void check(const plugin::Context&, std::vector<plugin::Finding>&) const override {}
};

plugin::Registration<FlowUninitialized> flow;
plugin::Registration<Squatter> squatter;

fact::Initialization local(std::uint32_t line, std::string name) {
    fact::Initialization i;
    i.range = i.name = { { line, 0 }, { line, 3 } };
    i.variable = std::move(name);
    i.type = "int";
    i.indeterminate = true;
    return i;
}

} // namespace

int main() {
    using namespace mcxx::testing;

    "a plugin's `replaces` takes a built-in feature over; the built-in one is not asked"_test = [] {
        const auto catalog = plugin::catalog();
        const auto* e = catalog->find("uninitialized");
        expect(fatal(e != nullptr));
        expect(e->provider->name() == "flow.uninitialized" && e->origin == plugin::Origin::plugin);
        expect(e->shadowed == std::vector<std::string> { "mc++.iso" });
        fact::Facts facts;
        facts.initializations.push_back(local(1, "written_first"));
        facts.initializations.push_back(local(2, "read_first"));
        const auto config = mcxx::features::parse_config("[package]\nname = \"p\"\n[package.metadata.mcxx]\nprofile = \"safe\"\n");
        const auto result = mcxx::features::evaluate({ "/p/a.cpp", "", facts }, config);
        std::vector<std::string> messages;
        for (const auto& d : result.diagnostics)
            if (d.code == "uninitialized") messages.push_back(d.message);
        expect(messages.size() == 1 && messages[0].starts_with("read before it is written")) << messages.size();
    };

    "a second provider that does not replace is a conflict: reported, the built-in one used"_test = [] {
        const auto catalog = plugin::catalog();
        expect(catalog->find("goto")->provider->name() == "mc++.iso");
        expect(std::ranges::any_of(catalog->problems, [](const std::string& p) { return p.contains("`goto`") && p.contains("squatter"); }));
        fact::Facts facts;
        facts.gotos.push_back({ { { { 3, 0 }, { 3, 4 } }, "" }, "out" });
        const auto config = mcxx::features::parse_config("[package]\nname = \"p\"\n[package.metadata.mcxx.features]\ngoto = \"deny\"\n");
        const auto result = mcxx::features::evaluate({ "/p/a.cpp", "", facts }, config);
        std::size_t gotos { 0 }, conflicts { 0 };
        for (const auto& d : result.diagnostics) {
            if (d.code == "goto") {
                ++gotos;
                expect(!d.message.contains("squatted"));
            }
            if (d.code == "mcxx-config" && d.message.contains("squatter")) ++conflicts;
        }
        expect(gotos == 1 && conflicts == 1);
    };

    "a profile redefined: the plugin's definition is used; a feature that joins it by name still does"_test = [] {
        fact::Facts facts;
        facts.includes.push_back({ { { { 1, 0 }, { 1, 9 } }, "" }, "<vector>", false });
        const auto config = mcxx::features::parse_config("[package]\nname = \"p\"\n[package.metadata.mcxx]\nprofile = \"modules\"\n");
        expect(plugin::catalog()->profile("modules")->summary == "and no macros either");
        // include joins `modules` by its own Feature::profiles, which a profile's redefinition does not undo.
        const auto result = mcxx::features::evaluate({ "/p/a.cpp", "", facts }, config);
        expect(std::ranges::count_if(result.diagnostics, [](const msa::Diagnostic& d) { return d.code == "include"; }) == 1);
    };

    "a provider that stands in for another removes it, and a catalog is rebuilt after a registration"_test = [] {
        const auto before = plugin::catalog();
        plugin::register_rule(std::make_unique<Late>());
        const auto after = plugin::catalog();
        expect(after->generation > before->generation);
        expect(std::ranges::find(after->replaced, "squatter") != after->replaced.end());
        expect(!std::ranges::any_of(after->problems, [](const std::string& p) { return p.contains("squatter"); }));
        expect(std::ranges::none_of(after->rules, [](const plugin::Rule* r) { return r->name() == "squatter"; }));
    };

    return report();
}
