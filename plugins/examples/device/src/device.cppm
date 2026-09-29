// example.device: MC4's attribute and region extension points (M1.9), written against the SDK only.
//
//   [[acme::hot]] void on_frame();       an attribute: acme-hot-alloc reports allocation inside it
//   [[acme::device]] void kernel();      a region: profile acme.device applies inside it -- no
//                                        exceptions, no RTTI, no new or delete, whatever the package says
export module example.device;

import std;
import mcxx.msa;
import mcxx.plugin;

export namespace example::device {

inline constexpr std::string_view HOT { "acme::hot" };
inline constexpr std::string_view DEVICE { "acme::device" };
inline constexpr std::string_view HOT_ALLOC { "acme-hot-alloc" };
inline constexpr std::string_view PROFILE { "acme.device" };

class Rules final : public mcxx::plugin::Rule {
public:
    std::string_view name() const override { return "example.device"; }
    std::span<const mcxx::plugin::Feature> features() const override { return features_; }
    std::span<const mcxx::plugin::Profile> profiles() const override { return profiles_; }
    std::span<const mcxx::plugin::AttributeSpec> attributes() const override { return attributes_; }
    void check(const mcxx::plugin::Context& c, std::vector<mcxx::plugin::Finding>& out) const override;

private:
    std::vector<mcxx::plugin::Feature> features_ { mcxx::plugin::Feature {
        .id = std::string { HOT_ALLOC },
        .category = mcxx::plugin::Category::policy,
        .layer = "expr",
        .summary = "an allocation in a function marked [[acme::hot]]",
        .fix = "allocate before the hot path, or use storage the caller owns",
        .default_level = mcxx::plugin::Level::deny,
        .needs = mcxx::msa::fact::Kinds::attributes | mcxx::msa::fact::Kinds::allocations,
    } };
    std::vector<mcxx::plugin::Profile> profiles_ { mcxx::plugin::Profile {
        .name = std::string { PROFILE },
        .summary = "code that runs on the device: no exceptions, no RTTI, no new or delete",
        .features = { { "exceptions", mcxx::plugin::Level::deny }, { "rtti", mcxx::plugin::Level::deny }, { "new-delete", mcxx::plugin::Level::deny } },
    } };
    std::vector<mcxx::plugin::AttributeSpec> attributes_ {
        { std::string { HOT }, "a function on the hot path: nothing in it allocates", "" },
        { std::string { DEVICE }, "code that runs on the device: profile acme.device applies inside", std::string { PROFILE } },
    };
};

} // namespace example::device

namespace example::device {

void Rules::check(const mcxx::plugin::Context& c, std::vector<mcxx::plugin::Finding>& out) const {
    if (!c.wants(HOT_ALLOC)) return;
    for (const auto& a : c.facts.attributes) {
        if (a.name != HOT) continue;
        const auto inside = mcxx::plugin::subtree(c.facts, a.range);   // the declaration's own facts
        for (const auto& alloc : inside.allocations)
            if (!alloc.is_delete)
                out.push_back({ std::string { HOT_ALLOC }, alloc.range, std::format("`new` in [[acme::hot]] {}", a.declaration), alloc.container });
    }
}

mcxx::plugin::Registration<Rules> registration;

} // namespace example::device
