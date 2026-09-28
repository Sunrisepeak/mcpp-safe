// mc++.policy: rules about how the language is used that are not one ISO feature -- MC++'s own
// ISO controls are built in (mc++.iso, mcxx.features) -- and a library control. Written against the
// plugin SDK only, as any third-party plugin is.
//
//   raw-pointers     (policy)  a declaration whose type holds a raw pointer. Whether a program
//                              may use pointers at all is a configuration's to say:
//                              [package.metadata.mcxx.features] raw-pointers = "deny"
//   lib:std.vector   (library) std::vector, the sample of library control: a configuration
//                              names the containers a program may use
export module mcxx.plugins.policy;

import std;
import mcxx.msa;
import mcxx.plugin;

export namespace mcxx::plugins::policy {

inline constexpr std::string_view RAW_POINTERS { "raw-pointers" };
inline constexpr std::string_view STD_VECTOR { "lib:std.vector" };

class Rules final : public plugin::Rule {
public:
    std::string_view name() const override { return "mc++.policy"; }
    std::span<const plugin::Feature> features() const override { return features_; }
    void check(const plugin::Context& context, std::vector<plugin::Finding>& out) const override;

private:
    std::vector<plugin::Feature> features_ {
        plugin::Feature { .id = std::string { RAW_POINTERS }, .category = plugin::Category::policy, .layer = "decl",
                          .summary = "a declaration whose type holds a raw pointer",
                          .fix = "use a reference, std::unique_ptr, std::shared_ptr, std::span or an index",
                          .needs = msa::fact::Kinds::declarations },
        plugin::Feature { .id = std::string { STD_VECTOR }, .category = plugin::Category::library, .layer = "decl",
                          .summary = "std::vector (a library-control sample)", .fix = "use the container the configuration names",
                          .needs = msa::fact::Kinds::declarations | msa::fact::Kinds::declaration_types },
    };
};

} // namespace mcxx::plugins::policy

namespace mcxx::plugins::policy {

void Rules::check(const plugin::Context& context, std::vector<plugin::Finding>& out) const {
    const bool pointers { context.wants(RAW_POINTERS) }, vector { context.wants(STD_VECTOR) };
    for (const auto& d : context.facts.declarations) {
        if (pointers && d.pointer)
            out.push_back({ std::string { RAW_POINTERS }, d.name, std::format("`{}` holds a raw pointer (`{}`)", d.qualified_name, d.type), d.container });
        if (vector && std::ranges::find(d.templates, "std::vector") != d.templates.end())
            out.push_back({ std::string { STD_VECTOR }, d.name, std::format("`{}` uses std::vector (`{}`)", d.qualified_name, d.type), d.container });
    }
}

plugin::Registration<Rules> registration;

} // namespace mcxx::plugins::policy
