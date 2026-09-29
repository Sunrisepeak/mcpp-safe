// The wire formats: MC3 facts read and written again unchanged (MC3-4.11-2), the specification's
// example included; strict reading; features, profiles, providers and messages.
import std;
import nlohmann.json;
import mcxx.testing;
import mcxx.msa;
import mcxx.plugin;
import mcxx.plugin.wire;

namespace msa = mcxx::msa;
namespace fact = mcxx::msa::fact;
namespace plugin = mcxx::plugin;
namespace wire = mcxx::plugin::wire;
using wire::Json;

namespace {

std::filesystem::path repository() {
    // $MCXX_REPOSITORY, for a test run elsewhere than it was built (a cross-built one: its source's path
    // is the build machine's); otherwise where the source was compiled from.
    if (const char* given = std::getenv("MCXX_REPOSITORY")) return given;
    // tests/test_wire.cpp -> wire -> plugin -> modules -> the repository
    return std::filesystem::path { std::source_location::current().file_name() }.parent_path().parent_path().parent_path().parent_path().parent_path();
}

msa::Range at(std::uint32_t line) { return { { line, 1 }, { line, 7 } }; }

fact::Facts every_kind() {
    fact::Facts f;
    f.certainty = msa::Certainty::unknown;
    f.collected = fact::Kinds::all;
    fact::Declaration d { { at(1), "app" } };
    d.name = at(1);
    d.entity = "c:@N@app@x";
    d.qualified_name = "app::x";
    d.kind = msa::Kind::type_alias;
    d.type = "int *[4]";
    d.templates = { "std::vector", "std::span" };
    d.exported = d.c_array = d.pointer = true;
    f.declarations.push_back(d);
    fact::Initialization i;
    i.range = at(2);
    i.name = at(2);
    i.variable = "j";
    i.form = fact::InitForm::copy_list;
    i.elements = 3;
    i.indeterminate = true;
    f.initializations.push_back(i);
    f.casts.push_back({ { at(3), "" }, fact::CastKind::functional, "double", "int", false, true });
    f.allocations.push_back({ { at(4), "" }, true, true, "char" });
    f.pointer_arithmetic.push_back({ { at(5), "" }, "+=", "int *" });
    f.gotos.push_back({ { at(6), "" }, "*" });
    f.macros.push_back({ { at(7), "" }, "MAX" });
    f.uses.push_back({ { at(8), "" }, "typeid", "T" });
    f.includes.push_back({ { at(9), "" }, "<vector>", true });
    f.suppressions.push_back({ { at(10), "app" }, { "goto", "macros" }, "c:@F@f#", "app::f", "why" });
    fact::Attribute a { { at(11), "app" } };
    a.name = "acme::device";
    a.arguments = { "gpu", "3" };
    a.name_range = at(11);
    a.entity = "c:@N@app@F@kernel#";
    a.declaration = "app::kernel";
    a.kind = msa::Kind::function;
    f.attributes.push_back(a);
    fact::Import im { { at(12), "" } };
    im.module = "legacy";
    im.name = at(12);
    im.exported = true;
    fact::Import::Interface in;
    in.module = "legacy";
    in.found = true;
    in.profiles = { "safe" };
    in.levels = { { "c-array", "allow" }, { "raw-pointers", "warn" } };
    in.exported.push_back(d);
    im.interfaces.push_back(in);
    im.interfaces.push_back({ "legacy:detail", false, {}, {}, {} });
    f.imports.push_back(im);
    return f;
}

} // namespace

int main() {
    using namespace mcxx::testing;

    "the specification's example facts are read, and written again unchanged"_test = [] {
        std::ifstream in { repository() / "specs/examples/mc3-facts.json" };
        expect(fatal(in.good()));
        const Json original = Json::parse(in);
        const auto facts = wire::facts_from_json(original);
        expect(fatal(facts.has_value())) << (facts ? "" : facts.error());
        const Json again = wire::facts_to_json(*facts, original["path"].get<std::string>(), original["module"].get<std::string>());
        expect(again == original) << again.dump(1).substr(0, 400);
    };

    "every kind of fact survives a round trip"_test = [] {
        const Json first = wire::facts_to_json(every_kind(), "/p/a.cpp", "m:p");
        const auto back = wire::facts_from_json(first);
        expect(fatal(back.has_value())) << (back ? "" : back.error());
        expect(wire::facts_to_json(*back, "/p/a.cpp", "m:p") == first);
        expect(back->declarations[0].kind == msa::Kind::type_alias && first["declarations"][0]["kind"] == "type-alias");
        expect(back->casts[0].to_scalar && back->includes[0].global_module_fragment && back->certainty == msa::Certainty::unknown);
        expect(back->attributes.size() == 1 && back->attributes[0].arguments == std::vector<std::string> { "gpu", "3" });
        expect(back->imports.size() == 1 && back->imports[0].exported && back->imports[0].interfaces.size() == 2);
        expect(back->imports[0].interfaces[0].level("raw-pointers") == "warn" && back->imports[0].interfaces[0].exported[0].c_array);
        expect(!back->imports[0].interfaces[1].found);
    };

    "reading is strict: a missing or mistyped member is an error that names it"_test = [] {
        Json j = wire::facts_to_json(every_kind(), "/p/a.cpp", "");
        Json missing = j;
        missing["casts"][0].erase("kind");
        expect(!wire::facts_from_json(missing) && wire::facts_from_json(missing).error().contains("facts.casts[0].kind"));
        Json typed = j;
        typed["declarations"][0]["pointer"] = "yes";
        expect(!wire::facts_from_json(typed) && wire::facts_from_json(typed).error().contains("pointer"));
        Json kind = j;
        kind["collected"] = Json::array({ "lifetimes" });
        expect(!wire::facts_from_json(kind) && wire::facts_from_json(kind).error().contains("lifetimes"));
        Json old = j;
        old["mc3-version"] = "0.1.0";
        old.erase("attributes");
        old.erase("imports");
        for (auto& d : old["declarations"]) d.erase("local");
        expect(wire::facts_from_json(old).has_value()) << "an MC3 0.1.0 document still reads";
        Json nested = j;
        nested["imports"][0]["interfaces"][0]["exported"][0].erase("entity");
        expect(!wire::facts_from_json(nested) && wire::facts_from_json(nested).error().contains("facts.imports[0].interfaces[0].exported[0].entity"));
        Json version = j;
        version["mc3-version"] = "9.0.0";
        expect(!wire::facts_from_json(version));
    };

    "features, profiles and providers round trip"_test = [] {
        struct Sample final : plugin::Rule {
            std::vector<plugin::Feature> fs { plugin::Feature { .id = "acme-snake-case", .category = plugin::Category::policy, .layer = "decl",
                                                               .summary = "not snake_case", .fix = "rename", .default_level = plugin::Level::warn,
                                                               .profiles = { { "acme", plugin::Level::deny } },
                                                               .needs = fact::Kinds::declarations, .requires_declaration = "acme" } };
            std::vector<plugin::Profile> ps { plugin::Profile { .name = "acme", .summary = "acme's", .includes = { "safe" },
                                                               .categories = { { plugin::Category::extension, plugin::Level::deny } } } };
            std::string_view name() const override { return "acme.naming"; }
            std::span<const plugin::Feature> features() const override { return fs; }
            std::span<const plugin::Profile> profiles() const override { return ps; }
            void check(const plugin::Context&, std::vector<plugin::Finding>&) const override {}
        } sample;
        const auto info = wire::describe(sample, true, false);
        const Json j = wire::to_json(info);
        const auto back = wire::provider_from(j);
        expect(fatal(back.has_value())) << (back ? "" : back.error());
        expect(wire::to_json(*back) == j);
        expect(back->features[0].requires_declaration == "acme" && back->features[0].needs == fact::Kinds::declarations);
        expect(back->profiles[0].categories.size() == 1 && back->extension_points == std::vector<std::string> { "rule" });
    };

    "a message is one line, and only an object with a type is one"_test = [] {
        Json m = Json::object();
        m["type"] = "findings";
        m["id"] = 3;
        m["findings"] = Json::array({ wire::to_json(plugin::Finding { "goto", at(2), "a\nb", "" }) });
        const std::string text { wire::line(m) };
        expect(text.ends_with("\n") && std::ranges::count(text, '\n') == 1);
        const auto back = wire::message_from(text);
        expect(back.has_value() && (*back)["id"] == 3);
        expect(!wire::message_from("[1,2]") && !wire::message_from("{\"id\":1}") && !wire::message_from("not json"));
        const auto target = wire::target_from(wire::to_json(plugin::Target { .triple = "t", .os = "linux", .family = "unix", .arch = "x86_64",
                                                                             .pointer_width = 64, .endian = "little", .features = { "SIMD" } }));
        expect(target.has_value() && target->features == std::vector<std::string> { "SIMD" });
    };

    return report();
}
