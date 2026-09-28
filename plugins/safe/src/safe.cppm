// mc++.safe v0: MC++'s safe dialect as a plugin. Each feature below is a thing the dialect gates;
// what a program may use is its configuration's to say (profile "safe" denies all of them).
export module mcxx.safe;

import std;
import mcxx.msa;
import mcxx.plugin;

export namespace mcxx::safe {

class Rules final : public plugin::Rule {
public:
    Rules();
    std::string_view name() const override { return "mc++.safe"; }
    std::span<const plugin::Feature> features() const override { return features_; }
    void check(const plugin::Context& context, std::vector<plugin::Finding>& out) const override;

private:
    std::vector<plugin::Feature> features_;
};

} // namespace mcxx::safe

namespace mcxx::safe {

namespace {

plugin::Feature feature(std::string id, std::string category, std::string layer, std::string summary, std::string fix) {
    return { .id = std::move(id), .category = std::move(category), .layer = std::move(layer), .summary = std::move(summary), .fix = std::move(fix),
             .default_level = plugin::Level::allow, .waivable = true, .profiles = { { "safe", plugin::Level::deny } } };
}

} // namespace

Rules::Rules()
    : features_ {
          feature("raw-pointer-arithmetic", "language", "expr", "arithmetic on a raw pointer",
                  "use std::span, an iterator or an index into a container"),
          feature("new-delete", "language", "expr", "a new or delete expression", "use std::make_unique, std::make_shared or a container"),
          feature("reinterpret-cast", "language", "expr", "a reinterpret_cast (or a C-style cast that is one)", "use std::bit_cast or a typed API"),
          feature("c-array", "language", "decl", "a C array", "use std::array or std::vector"),
          feature("goto", "language", "syntax", "a goto", "use structured control flow"),
          feature("macros", "language", "syntax", "a macro definition", "use constexpr, a template or an inline function"),
          feature("union", "language", "decl", "a union", "use std::variant"),
          feature("lib:std.vector", "library", "decl", "std::vector (a library-control sample)", "use the container the configuration names"),
      } {}

void Rules::check(const plugin::Context& context, std::vector<plugin::Finding>& out) const {
    const auto& facts = context.facts;
    for (const auto& p : facts.pointer_arithmetic)
        out.push_back({ "raw-pointer-arithmetic", p.range, std::format("pointer arithmetic (`{}`) on `{}`", p.op, p.type), p.container });
    for (const auto& a : facts.allocations)
        out.push_back({ "new-delete", a.range, std::format("`{}{}` of `{}`", a.is_delete ? "delete" : "new", a.array ? "[]" : "", a.type), a.container });
    for (const auto& c : facts.casts)
        if (c.reinterprets)
            out.push_back({ "reinterpret-cast", c.range, std::format("{} from `{}` to `{}` reinterprets its operand", msa::fact::to_string(c.kind), c.from, c.to),
                            c.container });
    for (const auto& g : facts.gotos) out.push_back({ "goto", g.range, std::format("`goto {}`", g.label), g.container });
    for (const auto& m : facts.macros) out.push_back({ "macros", m.range, std::format("macro `{}` defined", m.name), m.container });
    for (const auto& d : facts.declarations) {
        if (d.c_array) out.push_back({ "c-array", d.name, std::format("`{}` is a C array (`{}`)", d.qualified_name, d.type), d.container });
        if (d.is_union) out.push_back({ "union", d.name, std::format("`{}` is a union", d.qualified_name), d.container });
        if (std::ranges::find(d.templates, "std::vector") != d.templates.end())
            out.push_back({ "lib:std.vector", d.name, std::format("`{}` uses std::vector (`{}`)", d.qualified_name, d.type), d.container });
    }
}

plugin::Registration<Rules> registration;

} // namespace mcxx::safe
