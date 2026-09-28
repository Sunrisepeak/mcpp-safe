// mc++.iso: MC++'s built-in feature controls -- ISO C++ language features, each named by the
// standard's stable names, each one a subtraction: a program that does not use it is still ISO
// C++, compiled by any compiler. Written against the plugin SDK like every plugin, registered as
// built-in; a plugin may provide one of these ids instead (Feature::replaces) or stand in for the
// whole provider (Provider::replaces "mc++.iso").
//
// Profiles defined here:
//   safe      the sources of undefined behavior a compiler cannot check for you: pointer
//             arithmetic, new/delete, reinterpret_cast and C-style casts, const_cast, unions,
//             C arrays, C varargs, uninitialized locals, inline asm
//   modules   every dependency by import: no #include outside a global module fragment
//   strict    safe and modules, and goto and macro definitions
//   portable  ISO C++ only: every extension feature (a plugin's, [[mcpp::cfg]]) is denied
// exceptions and rtti are in no profile: a package that does without them says so.
export module mcxx.features.iso;

import std;
import mcxx.msa;
import mcxx.plugin;

export namespace mcxx::features::iso {

inline constexpr std::string_view PROVIDER { "mc++.iso" };

class Rules final : public plugin::Rule {
public:
    Rules();
    std::string_view name() const override { return PROVIDER; }
    std::span<const plugin::Feature> features() const override { return features_; }
    std::span<const plugin::Profile> profiles() const override { return profiles_; }
    void check(const plugin::Context& context, std::vector<plugin::Finding>& out) const override;

private:
    std::vector<plugin::Feature> features_;
    std::vector<plugin::Profile> profiles_;
};

} // namespace mcxx::features::iso

namespace mcxx::features::iso {

namespace {

using plugin::Level;
using K = msa::fact::Kinds;

plugin::Feature feature(std::string id, std::string standard, std::string layer, K needs, std::string summary, std::string fix,
                        std::vector<std::pair<std::string, Level>> profiles) {
    return { .id = std::move(id), .category = plugin::Category::iso, .standard = std::move(standard), .layer = std::move(layer),
             .summary = std::move(summary), .fix = std::move(fix), .default_level = Level::allow, .waivable = true,
             .profiles = std::move(profiles), .needs = needs };
}

const std::vector<std::pair<std::string, Level>> SAFE { { "safe", Level::deny } };
const std::vector<std::pair<std::string, Level>> STRICT { { "strict", Level::deny } };
const std::vector<std::pair<std::string, Level>> MODULES { { "modules", Level::deny } };

} // namespace

Rules::Rules()
    : features_ {
          // The safe profile: undefined behavior a compiler does not check.
          feature("raw-pointer-arithmetic", "[expr.add] [expr.sub] [expr.pre.incr]", "expr", K::pointer_arithmetic,
                  "arithmetic on (or a subscript of) a raw pointer: out of its array is undefined",
                  "use std::span, an iterator or an index into a container", SAFE),
          feature("new-delete", "[expr.new] [expr.delete]", "expr", K::allocations,
                  "a new or delete expression: a leak, a double or mismatched delete, a use after it",
                  "use std::make_unique, std::make_shared or a container", SAFE),
          feature("reinterpret-cast", "[expr.reinterpret.cast]", "expr", K::casts,
                  "a reinterpret_cast (or a cast that is one): an access through the wrong type is undefined",
                  "use std::bit_cast, std::start_lifetime_as or a typed API", SAFE),
          feature("c-style-cast", "[expr.cast] [expr.type.conv]", "expr", K::casts,
                  "a C-style cast (or `T(x)` to a scalar T): it may be a const_cast or a reinterpret_cast without saying so",
                  "use static_cast (or the named cast it has to be)", SAFE),
          feature("const-cast", "[expr.const.cast]", "expr", K::casts,
                  "a const_cast: writing to a const object through it is undefined",
                  "make the object mutable where it is declared, or copy it", SAFE),
          feature("union", "[class.union]", "decl", K::declarations,
                  "a union: reading a member that is not the active one is undefined", "use std::variant", SAFE),
          feature("c-array", "[dcl.array]", "decl", K::declarations,
                  "a C array: unchecked bounds, and it decays to a pointer", "use std::array or std::vector", SAFE),
          feature("c-varargs", "[dcl.fct] [cstdarg.syn]", "decl", K::declarations | K::uses,
                  "a C variadic function (`...`) or va_arg: a wrong type read is undefined",
                  "use a variadic template, std::initializer_list or std::span", SAFE),
          feature("uninitialized", "[dcl.init.general] [basic.indet]", "decl", K::initializations,
                  "a local whose default-initialization leaves its value indeterminate: reading it is undefined",
                  "give it an initializer (`T x {};` for zero)", SAFE),
          feature("asm", "[dcl.asm]", "decl", K::uses, "an asm declaration: outside the abstract machine",
                  "use an intrinsic, std::atomic or a function written in the toolchain's own files", SAFE),
          // Modules: dependencies by import.
          feature("include", "[cpp.include] [module.global.frag]", "text", K::includes,
                  "an #include outside a global module fragment: a textual dependency, not a module",
                  "import the module (or the header unit); keep #include in the global module fragment", MODULES),
          // strict: control flow and the preprocessor.
          feature("goto", "[stmt.goto]", "syntax", K::gotos, "a goto", "use structured control flow", STRICT),
          feature("macros", "[cpp.replace]", "syntax", K::macros, "a macro definition", "use constexpr, a template or an inline function", STRICT),
          // In no profile: a package states them.
          feature("exceptions", "[except.throw] [except.pre]", "expr", K::uses, "a throw expression or a try block",
                  "return std::expected or an error code", {}),
          feature("rtti", "[expr.typeid] [expr.dynamic.cast]", "expr", K::uses | K::casts, "typeid or dynamic_cast",
                  "use a virtual function or std::variant", {}),
      },
      profiles_ {
          { .name = "safe", .summary = "the sources of undefined behavior a compiler does not check" },
          { .name = "modules", .summary = "every dependency by import: #include only in a global module fragment" },
          { .name = "strict", .summary = "safe and modules, and goto and macro definitions", .includes = { "safe", "modules" } },
          { .name = "portable", .summary = "ISO C++ only: every extension is denied", .categories = { { plugin::Category::extension, Level::deny } } },
      } {}

void Rules::check(const plugin::Context& context, std::vector<plugin::Finding>& out) const {
    const auto& facts = context.facts;
    auto add = [&](std::string_view id, const msa::fact::Place& at, msa::Range range, std::string message) {
        out.push_back({ std::string { id }, range, std::move(message), at.container });
    };
    if (context.wants("raw-pointer-arithmetic"))
        for (const auto& p : facts.pointer_arithmetic)
            add("raw-pointer-arithmetic", p, p.range, std::format("pointer arithmetic (`{}`) on `{}`", p.op, p.type));
    if (context.wants("new-delete"))
        for (const auto& a : facts.allocations)
            add("new-delete", a, a.range, std::format("`{}{}` of `{}`", a.is_delete ? "delete" : "new", a.array ? "[]" : "", a.type));
    const bool reinterpret { context.wants("reinterpret-cast") }, c_style { context.wants("c-style-cast") };
    const bool const_cast_ { context.wants("const-cast") }, rtti { context.wants("rtti") };
    for (const auto& c : facts.casts) {
        using msa::fact::CastKind;
        if (reinterpret && c.reinterprets)
            add("reinterpret-cast", c, c.range, std::format("{} from `{}` to `{}` reinterprets its operand", msa::fact::to_string(c.kind), c.from, c.to));
        if (c_style && c.kind == CastKind::c_style) add("c-style-cast", c, c.range, std::format("C-style cast from `{}` to `{}`", c.from, c.to));
        if (c_style && c.kind == CastKind::functional && c.to_scalar)
            add("c-style-cast", c, c.range, std::format("functional cast from `{}` to `{}`: to a scalar, `T(x)` is `(T)x`", c.from, c.to));
        if (const_cast_ && c.kind == CastKind::const_cast_) add("const-cast", c, c.range, std::format("const_cast from `{}` to `{}`", c.from, c.to));
        if (rtti && c.kind == CastKind::dynamic_cast_) add("rtti", c, c.range, std::format("dynamic_cast from `{}` to `{}`", c.from, c.to));
    }
    const bool union_ { context.wants("union") }, c_array { context.wants("c-array") }, varargs { context.wants("c-varargs") };
    for (const auto& d : facts.declarations) {
        if (c_array && d.c_array) add("c-array", d, d.name, std::format("`{}` is a C array (`{}`)", d.qualified_name, d.type));
        if (union_ && d.is_union) add("union", d, d.name, std::format("`{}` is a union", d.qualified_name));
        if (varargs && d.c_variadic) add("c-varargs", d, d.name, std::format("`{}` takes C varargs (`...`)", d.qualified_name));
    }
    if (context.wants("uninitialized"))
        for (const auto& i : facts.initializations)
            if (i.indeterminate) add("uninitialized", i, i.name, std::format("`{}` (`{}`) is not initialized", i.variable, i.type));
    const bool asm_ { context.wants("asm") }, exceptions { context.wants("exceptions") };
    for (const auto& u : facts.uses) {
        if (varargs && u.construct == "va_arg") add("c-varargs", u, u.range, "va_arg reads a C variadic argument");
        if (asm_ && u.construct == "asm") add("asm", u, u.range, "an asm declaration");
        if (exceptions && (u.construct == "throw" || u.construct == "try")) add("exceptions", u, u.range, std::format("`{}`", u.construct));
        if (rtti && u.construct == "typeid") add("rtti", u, u.range, std::format("typeid of `{}`", u.detail));
    }
    if (context.wants("include"))
        for (const auto& i : facts.includes)
            if (!i.global_module_fragment)
                add("include", i, i.range, std::format("#include {}{}", i.header, context.module.empty() ? "" : std::format(" in module {}'s purview", context.module)));
    if (context.wants("goto"))
        for (const auto& g : facts.gotos) add("goto", g, g.range, std::format("`goto {}`", g.label));
    if (context.wants("macros"))
        for (const auto& m : facts.macros) add("macros", m, m.range, std::format("macro `{}` defined", m.name));
}

plugin::Registration<Rules, plugin::Origin::builtin> registration;

} // namespace mcxx::features::iso
