// MC1 0.5.0, category `standard`: a core-language feature of a newer C++ standard -- named for its
// standard (MC1-2.1-5), off unless enabled (MC1-2.1-6), naming its paper (MC1-2-3).
import std;
import mcxx.testing;
import mcxx.plugin;

namespace plugin = mcxx::plugin;

namespace {

plugin::Feature standard(std::string id, plugin::Level level = plugin::Level::deny, std::string paper = "P2662R3",
                         plugin::Category category = plugin::Category::standard) {
    return plugin::Feature { .id = std::move(id), .category = category, .standard = std::move(paper), .summary = "a feature for the test",
                             .default_level = level };
}

struct Cpp26 final : plugin::LanguageProvider {
    std::vector<plugin::Feature> features_ {
        standard("c++26:pack-indexing"),                                        // well formed
        standard("pack-indexing-unnamed"),                                      // no standard in its id
        standard("c++26:on-by-default", plugin::Level::allow),                  // not off by default
        standard("c++26:no-paper", plugin::Level::deny, ""),                    // names no paper
        standard("c++29:named-for-a-standard", plugin::Level::deny, "", plugin::Category::policy),   // another category
    };
    std::string_view name() const override { return "test.cpp26"; }
    std::span<const plugin::Feature> features() const override { return features_; }
    std::vector<std::string> arguments(const plugin::LanguageContext&) const override { return {}; }
};
plugin::Registration<Cpp26> registration;

bool reported(std::string_view id, std::string_view rule) {
    return std::ranges::any_of(plugin::catalog()->problems, [&](const std::string& p) { return p.contains(std::format("`{}`", id)) && p.contains(rule); });
}

} // namespace

int main() {
    using namespace mcxx::testing;

    "a standard feature is named for its standard, off by default and names its paper; the rest are problems"_test = [] {
        expect(!std::ranges::any_of(plugin::catalog()->problems, [](const std::string& p) { return p.contains("`c++26:pack-indexing`"); }))
            << "a well-formed one is no problem";
        expect(reported("pack-indexing-unnamed", "(MC1-2.1-5)"));
        expect(reported("c++26:on-by-default", "(MC1-2.1-6)"));
        expect(reported("c++26:no-paper", "(MC1-2-3)"));
        expect(reported("c++29:named-for-a-standard", "(MC1-2.1-5)"));
        // A standard's prefix is not a malformed id (MC1-2-1).
        expect(!reported("c++26:pack-indexing", "(MC1-2-1)") && !reported("c++26:on-by-default", "(MC1-2-1)"));
    };

    "the category has its name both ways"_test = [] {
        expect(plugin::to_string(plugin::Category::standard) == "standard");
        expect(plugin::parse_category("standard") == plugin::Category::standard);
    };

    "a plugin declares one without replacing anything (unlike iso, MC1-2.1-1)"_test = [] {
        expect(!reported("c++26:pack-indexing", "(MC1-2.1-1)"));
        const plugin::Feature* f { plugin::find_feature("c++26:pack-indexing") };
        expect(f != nullptr && f->category == plugin::Category::standard && f->default_level == plugin::Level::deny);
    };

    return report();
}
