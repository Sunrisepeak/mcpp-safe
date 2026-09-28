// Rules for nlohmann::json.
//
// json-brace-init: `Json x { expr };` is list-initialization, and nlohmann::basic_json has an
// initializer_list constructor, so it builds a ONE-ELEMENT ARRAY holding expr -- `Json x { nullptr }`
// is `[null]`, `Json id { other.id }` is `[id]` -- never a copy. It is the right way to write an
// object literal (`Json x { { "k", v } }`: the element is itself a braced list), and nearly always a
// bug otherwise. Found in the mcxx engine more than once, and twice in mcppls itself.
export module mcxx.plugins.json;

import std;
import mcxx.msa;
import mcxx.plugin;

export namespace mcxx::plugins::json {

inline constexpr std::string_view BRACE_INIT { "json-brace-init" };

class Rules final : public plugin::Rule {
public:
    std::string_view name() const override { return "mcxx.plugins.json"; }
    std::span<const plugin::Feature> features() const override { return features_; }
    void check(const plugin::Context& context, std::vector<plugin::Finding>& out) const override;

private:
    std::vector<plugin::Feature> features_ { plugin::Feature {
        .id = std::string { BRACE_INIT },
        .category = plugin::Category::pitfall,
        .layer = "expr",
        .summary = "a nlohmann::json list-initialized from one value holds it in a one-element array",
        .fix = "write `Json x = value;` for a copy, or `Json::array({ value })` for an array",
        .default_level = plugin::Level::deny,
        .needs = msa::fact::Kinds::initializations,
        .requires_declaration = "nlohmann",   // a file without nlohmann is not walked for it
    } };
};

} // namespace mcxx::plugins::json

namespace mcxx::plugins::json {

void Rules::check(const plugin::Context& context, std::vector<plugin::Finding>& out) const {
    for (const auto& init : context.facts.initializations) {
        const bool list { init.form == msa::fact::InitForm::direct_list || init.form == msa::fact::InitForm::copy_list };
        if (!list || init.type_template != "nlohmann::basic_json" || !init.initializer_list_constructor) continue;
        if (init.elements != 1 || init.element_braced) continue;
        out.push_back({ std::string { BRACE_INIT }, init.range,
                        std::format("`{}` is list-initialized from one value: it holds that value in a one-element array, not a copy of it",
                                    init.variable.empty() ? std::string { "this json" } : init.variable),
                        init.container });
    }
}

// Linked into a program, the rules are there: a static registration, since a static program
// cannot load one (V0.5).
plugin::Registration<Rules> registration;

} // namespace mcxx::plugins::json
