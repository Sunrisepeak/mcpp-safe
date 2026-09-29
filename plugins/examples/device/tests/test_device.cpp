// example.device over facts: [[acme::hot]]'s rule reads its declaration's subtree; [[acme::device]]
// raises levels inside its declaration only, and a waiver there still waives.
import std;
import mcxx.testing;
import mcxx.msa;
import mcxx.plugin;
import mcxx.features;
import example.device;

namespace msa = mcxx::msa;
namespace fact = mcxx::msa::fact;

namespace {

msa::Range lines(std::uint32_t from, std::uint32_t to) { return { { from, 0 }, { to, 80 } }; }
msa::Range at(std::uint32_t line) { return { { line, 4 }, { line, 20 } }; }

fact::Attribute attribute(std::string name, std::uint32_t from, std::uint32_t to, std::string declaration) {
    fact::Attribute a;
    a.range = lines(from, to);
    a.name = std::move(name);
    a.name_range = at(from);
    a.declaration = std::move(declaration);
    a.kind = msa::Kind::function;
    return a;
}

std::vector<std::pair<std::string, std::uint32_t>> found(const mcxx::features::Result& r) {
    std::vector<std::pair<std::string, std::uint32_t>> out;
    for (const auto& d : r.diagnostics)
        if (d.code != "mcxx-config") out.emplace_back(d.code, d.range.begin.line);
    return out;
}

} // namespace

int main() {
    using namespace mcxx::testing;

    "the catalog has the attributes, the region and its profile"_test = [] {
        const auto c = mcxx::plugin::catalog();
        const auto* hot = c->attribute("acme::hot");
        const auto* device = c->attribute("acme::device");
        expect(fatal(hot != nullptr && device != nullptr));
        expect(hot->attribute->region.empty() && device->attribute->region == "acme.device");
        expect(c->profile("acme.device") != nullptr && c->problems.empty());
    };

    "[[acme::hot]]: an allocation inside it is found, one outside is not"_test = [] {
        fact::Facts f;
        f.attributes.push_back(attribute("acme::hot", 1, 5, "on_frame"));
        f.allocations.push_back({ { at(3), "" }, false, false, "int" });    // inside
        f.allocations.push_back({ { at(9), "" }, false, false, "int" });    // outside
        f.allocations.push_back({ { at(4), "" }, true, false, "int" });     // a delete: not an allocation
        const auto r = mcxx::features::evaluate({ "/p/a.cpp", "", f }, mcxx::features::Config {});
        expect(found(r) == std::vector<std::pair<std::string, std::uint32_t>> { { "acme-hot-alloc", 3 } });
    };

    "[[acme::device]]: its profile applies inside it, not outside; a waiver there still waives"_test = [] {
        fact::Facts f;
        f.attributes.push_back(attribute("acme::device", 10, 20, "kernel"));
        f.uses.push_back({ { at(12), "" }, "throw", "" });                    // inside: denied by acme.device
        f.uses.push_back({ { at(30), "" }, "throw", "" });                    // outside: the package allows it
        f.allocations.push_back({ { at(14), "" }, false, false, "int" });     // inside: new-delete denied
        f.casts.push_back({ { at(15), "" }, fact::CastKind::dynamic_cast_, "B *", "D *", false });   // inside: rtti
        f.uses.push_back({ { at(16), "" }, "throw", "" });
        f.suppressions.push_back({ { lines(16, 16), "" }, { "exceptions" }, "c:@F@g#", "g", "reported by the host" });
        const auto r = mcxx::features::evaluate({ "/p/a.cpp", "", f }, mcxx::features::Config {});
        const auto got = found(r);
        expect(got == std::vector<std::pair<std::string, std::uint32_t>> { { "new-delete", 14 }, { "rtti", 15 }, { "exceptions", 12 } }) << got.size();
        expect(r.waived.size() == 1 && r.waived[0].feature == "exceptions");
        expect(std::ranges::all_of(r.diagnostics, [](const msa::Diagnostic& d) { return d.code == "mcxx-config" || d.severity == msa::Severity::error; }));
    };

    return report();
}
