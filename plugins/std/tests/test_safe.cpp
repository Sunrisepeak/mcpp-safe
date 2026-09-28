// mc++.safe v0 over facts: each feature's finding, the safe profile, a module opting out.
import std;
import mcxx.testing;
import mcxx.msa;
import mcxx.plugin;
import mcxx.features;
import mcxx.plugins.safe;

namespace msa = mcxx::msa;
namespace fact = mcxx::msa::fact;

namespace {

msa::Range at(std::uint32_t line) { return { { line, 0 }, { line, 5 } }; }

fact::Facts every_feature() {
    fact::Facts f;
    f.pointer_arithmetic.push_back({ { at(1), "" }, "+", "int *" });
    f.allocations.push_back({ { at(2), "" }, false, false, "int" });
    f.casts.push_back({ { at(3), "" }, fact::CastKind::reinterpret_cast_, "int *", "long", true });
    f.casts.push_back({ { at(4), "" }, fact::CastKind::static_cast_, "int", "long", false });
    f.gotos.push_back({ { at(5), "" }, "out" });
    f.macros.push_back({ { at(6), "" }, "MAX" });
    fact::Declaration array { { at(7), "" } };
    array.name = at(7);
    array.qualified_name = "buf";
    array.type = "int[4]";
    array.c_array = true;
    f.declarations.push_back(array);
    fact::Declaration u { { at(8), "" } };
    u.name = at(8);
    u.qualified_name = "U";
    u.is_union = true;
    f.declarations.push_back(u);
    fact::Declaration v { { at(9), "" } };
    v.name = at(9);
    v.qualified_name = "items";
    v.type = "std::vector<int>";
    v.templates = { "std::vector" };
    f.declarations.push_back(v);
    return f;
}

std::vector<std::string> codes(const mcxx::features::Result& r) {
    std::vector<std::string> out;
    for (const auto& d : r.diagnostics) out.push_back(d.code);
    return out;
}

} // namespace

int main() {
    using namespace mcxx::testing;
    const fact::Facts facts { every_feature() };

    "allowed by default: nothing is said"_test = [&] {
        expect(mcxx::features::evaluate({ "/p/a.cpp", "", facts }, {}).diagnostics.empty());
    };

    "the safe profile denies every feature, once each; a static_cast is not a reinterpretation"_test = [&] {
        const auto config = mcxx::features::parse_config("[package]\nname = \"p\"\n[package.metadata.mcxx]\nprofile = \"safe\"\n");
        const auto result = mcxx::features::evaluate({ "/p/a.cpp", "", facts }, config);
        expect(codes(result) == std::vector<std::string> { "raw-pointer-arithmetic", "new-delete", "reinterpret-cast", "goto", "macros", "c-array", "union",
                                                           "lib:std.vector" })
            << result.diagnostics.size();
        expect(std::ranges::all_of(result.diagnostics, [](const msa::Diagnostic& d) { return d.severity == msa::Severity::error; }));
    };

    "a module takes its own levels; its partitions do too"_test = [&] {
        const auto config = mcxx::features::parse_config(
            "[package]\nname = \"p\"\n[package.metadata.mcxx]\nprofile = \"safe\"\n[package.metadata.mcxx.modules.\"app.legacy\"]\ngoto = \"allow\"\nmacros = \"warn\"\n");
        const auto result = mcxx::features::evaluate({ "/p/a.cppm", "app.legacy:part", facts }, config);
        const auto c = codes(result);
        expect(std::ranges::find(c, "goto") == c.end());
        const auto macros = std::ranges::find_if(result.diagnostics, [](const msa::Diagnostic& d) { return d.code == "macros"; });
        expect(macros != result.diagnostics.end() && macros->severity == msa::Severity::warning);
    };

    return report();
}
