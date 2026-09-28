// example.naming: declared names are snake_case -- no capital letter in a declaration's own name.
// Written against the plugin SDK only, like every plugin; registered at static initialization.
export module example.naming;

import std;
import mcxx.msa;
import mcxx.plugin;

export namespace example::naming {

inline constexpr std::string_view FEATURE { "snake-case-names" };

class Rules final : public mcxx::plugin::Rule {
public:
    std::string_view name() const override { return "example.naming"; }
    std::span<const mcxx::plugin::Feature> features() const override { return features_; }
    void check(const mcxx::plugin::Context& c, std::vector<mcxx::plugin::Finding>& out) const override;

private:
    std::vector<mcxx::plugin::Feature> features_ { mcxx::plugin::Feature {
        .id = std::string { FEATURE },
        .category = mcxx::plugin::Category::policy,
        .layer = "decl",
        .summary = "a declared name with a capital letter",
        .fix = "write it in snake_case",
        .needs = mcxx::msa::fact::Kinds::declarations,
    } };
};

} // namespace example::naming

namespace example::naming {

void Rules::check(const mcxx::plugin::Context& c, std::vector<mcxx::plugin::Finding>& out) const {
    if (!c.wants(FEATURE)) return;
    for (const auto& d : c.facts.declarations) {
        const std::string_view q { d.qualified_name };
        const auto colon = q.rfind(':');
        const std::string_view last { colon == std::string_view::npos ? q : q.substr(colon + 1) };
        if (std::ranges::any_of(last, [](char ch) { return ch >= 'A' && ch <= 'Z'; }))
            out.push_back({ std::string { FEATURE }, d.name, std::format("`{}` is not snake_case", d.qualified_name), d.container });
    }
}

mcxx::plugin::Registration<Rules> registration;

} // namespace example::naming
