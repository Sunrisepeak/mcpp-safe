// The agent view: one JSON object per diagnostic, on one line (schema/mc5-diagnostic.schema.json).
module mcxx.diagnostics;

import std;
import nlohmann.json;
import mcxx.msa;

namespace mcxx::diagnostics {

namespace {

using Json = nlohmann::json;

Json range_json(const msa::Range& r) {
    Json begin = Json::object();
    begin["line"] = r.begin.line;
    begin["column"] = r.begin.column;
    Json end = Json::object();
    end["line"] = r.end.line;
    end["column"] = r.end.column;
    Json out = Json::object();
    out["begin"] = begin;
    out["end"] = end;
    return out;
}

std::string_view severity_name(msa::Severity s) {
    switch (s) {
    case msa::Severity::error: return "error";
    case msa::Severity::warning: return "warning";
    case msa::Severity::information: return "note";
    case msa::Severity::hint: return "note";
    }
    return "error";
}

Json item_json(const Item& item, bool note) {
    const auto& d = item.diagnostic;
    Json j = Json::object();
    if (!note) j["mcxx-diagnostic"] = std::string { AGENT_VERSION };
    j["severity"] = std::string { note ? std::string_view { "note" } : severity_name(d.severity) };
    j["code"] = d.code;
    j["message"] = d.headline.empty() ? d.message : d.headline;
    if (!item.path.empty()) {
        j["file"] = item.path;
        j["range"] = range_json(d.range);
        j["location"] = std::format("{}:{}:{}", item.path, d.range.begin.line + 1, d.range.begin.column + 1);
    }
    if (!d.level.empty()) {
        j["level"] = d.level;
        j["level-from"] = d.level_from;
    }
    if (!d.fix.empty()) j["fix"] = d.fix;
    if (!d.waiver.empty()) j["waiver"] = d.waiver;
    Json fixits = Json::array();
    for (const auto& f : item.fixits) {
        Json x = Json::object();
        x["range"] = range_json(f.range);
        x["text"] = f.text;
        fixits.push_back(std::move(x));
    }
    if (!fixits.empty()) j["fixits"] = std::move(fixits);
    Json notes = Json::array();
    for (const auto& n : item.notes) notes.push_back(item_json(n, true));
    for (const auto& n : d.notes) {
        Json x = Json::object();
        x["severity"] = "note";
        x["code"] = "";
        x["message"] = n.message;
        if (!n.location.path.empty()) {
            x["file"] = n.location.path;
            x["range"] = range_json(n.location.range);
        }
        notes.push_back(std::move(x));
    }
    if (!notes.empty()) j["notes"] = std::move(notes);
    return j;
}

} // namespace

std::string agent(const Item& item) { return item_json(item, false).dump() + "\n"; }

} // namespace mcxx::diagnostics
