// mc++.iso, the built-in ISO controls, through the gates: each feature's finding, the profiles
// (safe = UB sources, modules, strict, and several at once), and what a plan asks for.
import std;
import mcxx.testing;
import mcxx.msa;
import mcxx.plugin;
import mcxx.features;

namespace msa = mcxx::msa;
namespace fact = mcxx::msa::fact;
namespace features = mcxx::features;

namespace {

msa::Range at(std::uint32_t line) { return { { line, 0 }, { line, 5 } }; }

fact::Declaration declaration(std::uint32_t line, std::string name) {
    fact::Declaration d { { at(line), "" } };
    d.name = at(line);
    d.qualified_name = std::move(name);
    return d;
}

// One fact per feature, in the order the rule reports them.
fact::Facts every_feature() {
    fact::Facts f;
    f.pointer_arithmetic.push_back({ { at(1), "" }, "+", "int *" });
    f.allocations.push_back({ { at(2), "" }, false, false, "int" });
    f.casts.push_back({ { at(3), "" }, fact::CastKind::reinterpret_cast_, "int *", "long", true });
    f.casts.push_back({ { at(4), "" }, fact::CastKind::c_style, "double", "int", false });
    f.casts.push_back({ { at(5), "" }, fact::CastKind::const_cast_, "const int *", "int *", false });
    f.casts.push_back({ { at(6), "" }, fact::CastKind::dynamic_cast_, "B *", "D *", false });
    f.casts.push_back({ { at(7), "" }, fact::CastKind::static_cast_, "int", "long", false });
    auto array = declaration(8, "buf");
    array.type = "int[4]";
    array.c_array = true;
    f.declarations.push_back(array);
    auto u = declaration(9, "U");
    u.is_union = true;
    f.declarations.push_back(u);
    auto printf_like = declaration(10, "log");
    printf_like.c_variadic = true;
    f.declarations.push_back(printf_like);
    fact::Initialization x;
    x.range = x.name = at(11);
    x.variable = "x";
    x.type = "int";
    x.indeterminate = true;
    f.initializations.push_back(x);
    fact::Initialization y { x };
    y.name = at(12);
    y.indeterminate = false;
    f.initializations.push_back(y);
    f.uses.push_back({ { at(13), "" }, "va_arg", "" });
    f.uses.push_back({ { at(14), "" }, "asm", "" });
    f.uses.push_back({ { at(15), "" }, "throw", "" });
    f.uses.push_back({ { at(16), "" }, "typeid", "T" });
    f.includes.push_back({ { at(17), "" }, "<vector>", false });
    f.includes.push_back({ { at(18), "" }, "<cstdio>", true });
    f.gotos.push_back({ { at(19), "" }, "out" });
    f.macros.push_back({ { at(20), "" }, "MAX" });
    return f;
}

std::vector<std::string> codes(const features::Result& r) {
    std::vector<std::string> out;
    for (const auto& d : r.diagnostics) out.push_back(d.code);
    return out;
}

features::Config config(std::string_view metadata) {
    return features::parse_config(std::format("[package]\nname = \"p\"\n{}", metadata));
}

features::Result run(const fact::Facts& f, std::string_view metadata, std::string_view module = "") {
    return features::evaluate({ "/p/a.cpp", module, f }, config(metadata));
}

} // namespace

int main() {
    using namespace mcxx::testing;
    const fact::Facts facts { every_feature() };

    "ISO C++ by default: nothing is gated, nothing is asked for"_test = [&] {
        const auto plan = features::make_plan({});
        expect(!plan.gated && plan.needs == fact::Kinds::none && plan.idle());
        expect(features::evaluate({ "/p/a.cpp", "", facts }, plan).diagnostics.empty());
    };

    "every built-in feature is iso, with its stable names"_test = [] {
        const auto catalog = mcxx::plugin::catalog();
        std::size_t iso { 0 };
        for (const auto& e : catalog->features) {
            if (e.feature->category != mcxx::plugin::Category::iso) continue;
            ++iso;
            expect(e.origin == mcxx::plugin::Origin::builtin && e.feature->standard.starts_with("[")) << e.feature->id;
        }
        expect(iso == 15) << iso;
    };

    "safe denies the sources of undefined behavior, and only those"_test = [&] {
        const auto result = run(facts, "[package.metadata.mcxx]\nprofile = \"safe\"\n");
        expect(codes(result) == std::vector<std::string> { "raw-pointer-arithmetic", "new-delete", "reinterpret-cast", "c-style-cast", "const-cast",
                                                           "c-array", "union", "c-varargs", "uninitialized", "c-varargs", "asm" })
            << result.diagnostics.size();
        expect(std::ranges::all_of(result.diagnostics, [](const msa::Diagnostic& d) { return d.severity == msa::Severity::error; }));
    };

    "strict is safe and modules, and goto and macros; an include in a global module fragment is fine"_test = [&] {
        const auto c = codes(run(facts, "[package.metadata.mcxx]\nprofile = \"strict\"\n"));
        expect(c.size() == 14) << c.size();
        for (const auto id : { "include", "goto", "macros", "asm" }) expect(std::ranges::count(c, id) == 1) << id;
        expect(std::ranges::find(c, "exceptions") == c.end() && std::ranges::find(c, "rtti") == c.end());
    };

    "several profiles at once, the strictest level wins; the package's own levels come after"_test = [&] {
        const auto c = codes(run(facts, "[package.metadata.mcxx]\nprofile = [\"modules\", \"safe\"]\n[package.metadata.mcxx.features]\nexceptions = \"deny\"\nrtti = \"warn\"\nc-array = \"allow\"\n"));
        expect(std::ranges::count(c, "include") == 1 && std::ranges::count(c, "exceptions") == 1 && std::ranges::count(c, "rtti") == 2);
        expect(std::ranges::find(c, "c-array") == c.end() && std::ranges::find(c, "goto") == c.end());
    };

    "a plan asks only the features that are gated, for only the facts they need"_test = [] {
        const auto plan = features::make_plan(config("[package.metadata.mcxx.features]\ngoto = \"deny\"\n"));
        expect(plan.gated && plan.needs == (fact::Kinds::gotos | fact::Kinds::suppressions));
        std::size_t asked { 0 };
        for (const auto& w : plan.wanted) asked += w.size();
        expect(asked == 1);
        const auto* g = plan.gate("goto");
        expect(fatal(g != nullptr));
        expect(plan.level(*g, "", "") == features::Level::deny);
    };

    "a module's level, a namespace's, and a waiver"_test = [&] {
        const auto c = config("[package.metadata.mcxx]\nprofile = \"strict\"\n[package.metadata.mcxx.modules.\"app.legacy\"]\ngoto = \"allow\"\nmacros = \"warn\"\n");
        const auto result = features::evaluate({ "/p/a.cppm", "app.legacy:part", facts }, c);
        const auto found = codes(result);
        expect(std::ranges::find(found, "goto") == found.end());
        const auto macros = std::ranges::find_if(result.diagnostics, [](const msa::Diagnostic& d) { return d.code == "macros"; });
        expect(macros != result.diagnostics.end() && macros->severity == msa::Severity::warning);
        fact::Facts waived;
        waived.gotos.push_back({ { at(5), "" }, "out" });
        waived.suppressions.push_back({ { { { 4, 0 }, { 6, 1 } }, "" }, { "goto" }, "c:@F@f#", "f", "a state machine" });
        const auto w = run(waived, "[package.metadata.mcxx]\nprofile = \"strict\"\n");
        expect(w.diagnostics.empty() && w.waived.size() == 1 && w.waived[0].reason == "a state machine");
    };

    "what a configuration names and nobody provides is reported"_test = [&] {
        const auto result = run(facts, "[package.metadata.mcxx]\nprofile = \"saef\"\n[package.metadata.mcxx.features]\ngotoo = \"deny\"\n");
        expect(result.diagnostics.size() == 2) << result.diagnostics.size();
        for (const auto& d : result.diagnostics) expect(d.code == "mcxx-config" && d.severity == msa::Severity::warning) << d.message;
    };

    return report();
}
