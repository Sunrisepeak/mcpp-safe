// json-brace-init over facts, and through the gates: no compiler involved.
import std;
import mcxx.testing;
import mcxx.msa;
import mcxx.plugin;
import mcxx.features;
import mcxx.plugins.json;

namespace msa = mcxx::msa;
namespace fact = mcxx::msa::fact;

namespace {

fact::Initialization json_init(fact::InitForm form, std::uint32_t elements, bool braced, std::string variable = "app::x") {
    fact::Initialization init;
    init.range = { { 3, 4 }, { 3, 20 } };
    init.variable = std::move(variable);
    init.type = "nlohmann::basic_json<>";
    init.type_template = "nlohmann::basic_json";
    init.form = form;
    init.constructor = "nlohmann::basic_json::basic_json(initializer_list_t, bool, value_t)";
    init.initializer_list_constructor = true;
    init.elements = elements;
    init.element_braced = braced;
    init.container = "app";
    return init;
}

mcxx::features::Result run(const fact::Facts& facts, const mcxx::features::Config& config = {}) {
    return mcxx::features::evaluate({ "/p/src/a.cpp", "", facts }, config);
}

} // namespace

int main() {
    using namespace mcxx::testing;
    using fact::InitForm;

    "one value in braces is the pitfall; an object literal and a copy are not"_test = [] {
        fact::Facts facts;
        facts.initializations.push_back(json_init(InitForm::direct_list, 1, false));      // Json x { y };
        facts.initializations.push_back(json_init(InitForm::copy_list, 1, false));        // Json x = { y };
        facts.initializations.push_back(json_init(InitForm::direct_list, 1, true));       // Json x { { "k", v } };
        facts.initializations.push_back(json_init(InitForm::direct_list, 2, false));      // Json x { a, b }: an array meant as one
        facts.initializations.push_back(json_init(InitForm::copy, 1, false));             // Json x = y;
        auto other = json_init(InitForm::direct_list, 1, false);
        other.type_template = "std::vector";
        facts.initializations.push_back(other);                                          // not a json
        const auto result = run(facts);
        expect(result.diagnostics.size() == 2) << result.diagnostics.size();
        for (const auto& d : result.diagnostics) {
            expect(d.code == "json-brace-init" && d.severity == msa::Severity::error);
            expect(d.message.contains("one-element array") && d.message.contains("mcpp::allow(\"json-brace-init\")")) << d.message;
        }
    };

    "a waiver on the declaration is recorded, not silent"_test = [] {
        fact::Facts facts;
        facts.initializations.push_back(json_init(InitForm::direct_list, 1, false));
        facts.suppressions.push_back({ { { { 2, 0 }, { 5, 1 } }, "app" }, { "json-brace-init" }, "c:@N@app@F@f#", "app::f", "an array is meant" });
        const auto result = run(facts);
        expect(result.diagnostics.empty());
        expect(fatal(result.waived.size() == 1));
        expect(result.waived[0].feature == "json-brace-init" && result.waived[0].reason == "an array is meant");
    };

    "the package's configuration decides the level"_test = [] {
        fact::Facts facts;
        facts.initializations.push_back(json_init(InitForm::direct_list, 1, false));
        const auto warn = mcxx::features::parse_config("[package]\nname = \"p\"\n[package.metadata.mcxx.features]\n\"json-brace-init\" = \"warn\"\n");
        expect(warn.problems.empty());
        const auto warned = run(facts, warn);
        expect(warned.diagnostics.size() == 1 && warned.diagnostics[0].severity == msa::Severity::warning);
        const auto off = mcxx::features::parse_config("[package]\nname = \"p\"\n[package.metadata.mcxx.namespaces.\"app\"]\n\"json-brace-init\" = \"allow\"\n");
        expect(run(facts, off).diagnostics.empty()) << "a namespace scope is more specific than the default";
        const auto bad = mcxx::features::parse_config("[package]\nname = \"p\"\n[package.metadata.mcxx.features]\n\"json-brace-init\" = \"loud\"\n");
        expect(bad.problems.size() == 1) << "an unknown level is reported, not guessed";
    };

    return report();
}
