// The plugin side over string streams: handshake, version negotiation, check, filter, errors.
import std;
import nlohmann.json;
import mcxx.testing;
import mcxx.msa;
import mcxx.plugin;
import mcxx.plugin.wire;
import mcxx.plugin.remote;

namespace fact = mcxx::msa::fact;
namespace plugin = mcxx::plugin;
namespace wire = mcxx::plugin::wire;
using wire::Json;

namespace {

struct Naming final : plugin::Rule {
    std::vector<plugin::Feature> fs { plugin::Feature { .id = "acme-snake-case", .category = plugin::Category::policy, .summary = "not snake_case",
                                                       .default_level = plugin::Level::warn, .needs = fact::Kinds::declarations } };
    std::string_view name() const override { return "acme.naming"; }
    std::span<const plugin::Feature> features() const override { return fs; }
    void check(const plugin::Context& c, std::vector<plugin::Finding>& out) const override {
        for (const auto& d : c.facts.declarations)
            if (std::ranges::any_of(d.qualified_name, [](char ch) { return ch >= 'A' && ch <= 'Z'; }))
                out.push_back({ "acme-snake-case", d.name, std::format("`{}` is not snake_case", d.qualified_name), d.container });
    }
};

struct Upper final : plugin::SourceFilter {
    std::string_view name() const override { return "acme.upper"; }
    plugin::Filtered filter(const plugin::SourceContext&, std::string_view text) const override {
        std::string out { text };
        for (char& c : out) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        return { out, {}, {} };
    }
};

plugin::Registration<Naming> naming;
plugin::Registration<Upper> upper;

std::vector<Json> run(const std::vector<Json>& requests, int* status = nullptr) {
    std::string input;
    for (const auto& r : requests) input += wire::line(r);
    std::istringstream in { input };
    std::ostringstream out;
    const int s { mcxx::plugin::remote::serve(in, out) };
    if (status != nullptr) *status = s;
    std::vector<Json> answers;
    std::istringstream lines { out.str() };
    for (std::string l; std::getline(lines, l);) answers.push_back(Json::parse(l));
    return answers;
}

Json hello(std::vector<int> protocols = { 1 }) {
    Json j = Json::object();
    j["type"] = "hello";
    j["id"] = 0;
    j["host"] = "test";
    j["protocols"] = protocols;
    return j;
}

} // namespace

int main() {
    using namespace mcxx::testing;

    "hello: the plugin chooses protocol 1 and describes its providers"_test = [] {
        const auto a = run({ hello({ 2, 1 }) });
        expect(fatal(a.size() == 1));
        expect(a[0]["type"] == "welcome" && a[0]["protocol"] == 1 && a[0]["providers"].size() == 2);
        expect(a[0]["providers"][0]["name"] == "acme.naming" && a[0]["providers"][0]["extension-points"] == Json::array({ "rule" }));
        expect(a[0]["providers"][1]["extension-points"] == Json::array({ "source-filter" }));
    };

    "a host that speaks no protocol this side does gets an error, and the plugin exits"_test = [] {
        int status { 0 };
        const auto a = run({ hello({ 7 }), hello() }, &status);
        expect(a.size() == 1 && a[0]["type"] == "error" && a[0]["code"] == "protocol" && a[0]["protocols"] == Json::array({ 1 }));
        expect(status == 1);
    };

    "check and filter are answered with their ids; shutdown ends the session"_test = [] {
        fact::Facts facts;
        fact::Declaration d { { { { 1, 0 }, { 1, 9 } }, "app" } };
        d.name = { { 1, 4 }, { 1, 9 } };
        d.qualified_name = "app::Value";
        facts.declarations.push_back(d);
        Json check = Json::object();
        check["type"] = "check";
        check["id"] = 1;
        check["provider"] = "acme.naming";
        check["path"] = "/w/a.cpp";
        check["module"] = "";
        check["wanted"] = Json::array({ "acme-snake-case" });
        check["facts"] = wire::facts_to_json(facts, "/w/a.cpp", "");
        Json filter = Json::object();
        filter["type"] = "filter";
        filter["id"] = 2;
        filter["provider"] = "acme.upper";
        filter["path"] = "/w/a.cpp";
        filter["target"] = wire::to_json(plugin::Target { .triple = "x86_64-unknown-linux-gnu" });
        filter["text"] = "int x;\n";
        Json shutdown = Json::object();
        shutdown["type"] = "shutdown";
        shutdown["id"] = 3;
        int status { 1 };
        const auto a = run({ hello(), check, filter, shutdown, check }, &status);
        expect(fatal(a.size() == 3)) << a.size() << " answers: nothing after shutdown";
        expect(a[1]["type"] == "findings" && a[1]["id"] == 1 && a[1]["findings"].size() == 1);
        expect(a[1]["findings"][0]["message"] == "`app::Value` is not snake_case");
        expect(a[2]["type"] == "filtered" && a[2]["id"] == 2 && a[2]["text"] == "INT X;\n");
        expect(status == 0);
    };

    "what cannot be served is an error with the request's id, and the session goes on"_test = [] {
        Json bad = Json::object();
        bad["type"] = "check";
        bad["id"] = 5;
        bad["provider"] = "nobody";
        bad["wanted"] = Json::array();
        bad["facts"] = Json::object();
        Json unknown = Json::object();
        unknown["type"] = "reload";
        unknown["id"] = 6;
        const auto a = run({ hello(), bad, unknown, hello() });
        expect(fatal(a.size() == 4));
        expect(a[1]["type"] == "error" && a[1]["id"] == 5 && a[1]["code"] == "request");
        expect(a[2]["type"] == "error" && a[2]["id"] == 6);
        expect(a[3]["type"] == "welcome");
    };

    return report();
}
