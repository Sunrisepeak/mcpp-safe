// The plugin side of MC4 protocol 1 (specs/mc4-plugins.md §6). A plugin program is its plugin
// packages linked with this one and a main that calls serve():
//
//   import mcxx.plugin.remote;
//   int main() { return mcxx::plugin::remote::serve(); }
//
// It serves the providers registered in the program -- the SDK's Registration, as in a static
// composition, so one plugin package works both ways -- and answers requests until `shutdown` or
// the end of its input. Nothing but messages goes to the output (MC4-6.1-2).
export module mcxx.plugin.remote;

import std;
import nlohmann.json;
import mcxx.msa;
import mcxx.plugin;
import mcxx.plugin.wire;

export namespace mcxx::plugin::remote {

// Serves until shutdown or the end of `in`; 0 on a clean end, 1 when the host speaks no protocol
// this side does.
int serve(std::istream& in, std::ostream& out);
int serve();   // standard input and output

} // namespace mcxx::plugin::remote

namespace mcxx::plugin::remote {

namespace {

using wire::Json;

Json error(const Json& id, std::string_view code, std::string message) {
    Json j = Json::object();
    j["type"] = "error";
    j["id"] = id;
    j["code"] = std::string { code };
    j["message"] = std::move(message);
    return j;
}

Json welcome(const Json& id, const Catalog& catalog) {
    Json j = Json::object();
    j["type"] = "welcome";
    j["id"] = id;
    j["protocol"] = wire::PROTOCOL;
    Json& providers = j["providers"] = Json::array();
    for (const auto& [provider, origin] : catalog.providers) {
        if (origin == Origin::builtin) continue;   // the host has MC++'s own
        const bool rule { std::ranges::find(catalog.rules, provider) != catalog.rules.end() };
        const bool filter { std::ranges::find(catalog.filters, provider) != catalog.filters.end() };
        providers.push_back(wire::to_json(wire::describe(*provider, rule, filter)));
    }
    return j;
}

template <class T>
const T* named(const std::vector<const T*>& list, std::string_view name) {
    for (const T* p : list)
        if (p->name() == name) return p;
    return nullptr;
}

Json check(const Json& m, const Catalog& catalog) {
    const Json& id = m["id"];
    if (!m.contains("provider") || !m["provider"].is_string() || !m.contains("facts") || !m.contains("wanted") || !m["wanted"].is_array())
        return error(id, "request", "a check names a provider, the facts and the wanted features");
    const Rule* rule { named(catalog.rules, m["provider"].get<std::string>()) };
    if (rule == nullptr) return error(id, "request", std::format("no rule is named {}", m["provider"].get<std::string>()));
    auto facts = wire::facts_from_json(m["facts"]);
    if (!facts) return error(id, "request", facts.error());
    std::vector<std::string> wanted;
    for (const auto& w : m["wanted"])
        if (w.is_string()) wanted.push_back(w.get<std::string>());
    const std::string path { m.value("path", std::string {}) };
    const std::string module { m.value("module", std::string {}) };
    std::vector<Finding> findings;
    rule->check({ path, module, *facts, &wanted }, findings);
    Json j = Json::object();
    j["type"] = "findings";
    j["id"] = id;
    Json& out = j["findings"] = Json::array();
    for (const auto& f : findings) out.push_back(wire::to_json(f));
    return j;
}

Json filter(const Json& m, const Catalog& catalog) {
    const Json& id = m["id"];
    if (!m.contains("provider") || !m["provider"].is_string() || !m.contains("text") || !m["text"].is_string() || !m.contains("target"))
        return error(id, "request", "a filter names a provider, the target and the text");
    const SourceFilter* f { named(catalog.filters, m["provider"].get<std::string>()) };
    if (f == nullptr) return error(id, "request", std::format("no source filter is named {}", m["provider"].get<std::string>()));
    auto target = wire::target_from(m["target"]);
    if (!target) return error(id, "request", target.error());
    const std::string path { m.value("path", std::string {}) };
    const std::string text { m["text"].get<std::string>() };
    Filtered result { f->filter({ path, *target }, text) };
    Json j = Json::object();
    j["type"] = "filtered";
    j["id"] = id;
    j["text"] = result.text ? Json(*result.text) : Json(nullptr);
    Json& problems = j["problems"] = Json::array();
    for (const auto& p : result.problems) {
        Json x = Json::object();
        x["range"] = wire::to_json(p.range);
        x["message"] = p.message;
        problems.push_back(std::move(x));
    }
    Json& findings = j["findings"] = Json::array();
    for (const auto& finding : result.findings) findings.push_back(wire::to_json(finding));
    return j;
}

} // namespace

int serve(std::istream& in, std::ostream& out) {
    const auto catalog = plugin::catalog();
    auto send = [&](const Json& j) { out << wire::line(j) << std::flush; };
    for (std::string text; std::getline(in, text);) {
        if (text.empty()) continue;
        auto message = wire::message_from(text);
        if (!message) {
            send(error(Json(nullptr), "request", message.error()));
            continue;
        }
        const Json& m = *message;
        const Json id = m.contains("id") ? m["id"] : Json(nullptr);
        const std::string type { m["type"].get<std::string>() };
        try {
            if (type == "hello") {
                bool speaks { false };
                if (m.contains("protocols") && m["protocols"].is_array())
                    for (const auto& p : m["protocols"]) speaks = speaks || p == wire::PROTOCOL;
                if (!speaks) {
                    Json e = error(id, "protocol", std::format("this plugin speaks MC4 protocol {} only", wire::PROTOCOL));
                    e["protocols"] = Json::array({ wire::PROTOCOL });
                    send(e);
                    return 1;
                }
                send(welcome(id, *catalog));
            } else if (type == "check") {
                send(check(m, *catalog));
            } else if (type == "filter") {
                send(filter(m, *catalog));
            } else if (type == "shutdown") {
                return 0;
            } else {
                send(error(id, "request", std::format("`{}` is not a request", type)));
            }
        } catch (const std::exception& e) {
            send(error(id, "internal", e.what()));
        }
    }
    return 0;
}

int serve() {
    std::ios::sync_with_stdio(false);
    return serve(std::cin, std::cout);
}

} // namespace mcxx::plugin::remote
