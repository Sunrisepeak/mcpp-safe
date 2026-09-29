// MC4's messages (specs/mc4-plugins.md §6): findings, features, profiles, targets, providers, lines.
module mcxx.plugin.wire;

import std;
import nlohmann.json;
import mcxx.msa;
import mcxx.plugin;
import :json;

namespace mcxx::plugin::wire {

Json to_json(const Finding& finding) {
    Json j = Json::object();
    j["feature"] = finding.feature;
    j["range"] = to_json(finding.range);
    j["message"] = finding.message;
    j["container"] = finding.container;
    return j;
}

Read<Finding> finding_from(const Json& j) {
    Reader r { j, "finding", {} };
    Finding f { r.str("feature"), r.range("range"), r.str("message"), r.str("container") };
    if (!r.ok()) return std::unexpected(r.error);
    return f;
}

Json to_json(const Feature& f) {
    Json j = Json::object();
    j["id"] = f.id;
    j["category"] = std::string { name_of(CATEGORIES, f.category) };
    j["standard"] = f.standard;
    j["layer"] = f.layer;
    j["summary"] = f.summary;
    j["fix"] = f.fix;
    j["default"] = std::string { name_of(LEVELS, f.default_level) };
    j["waivable"] = f.waivable;
    Json& profiles = j["profiles"] = Json::array();
    for (const auto& [name, level] : f.profiles) {
        Json p = Json::object();
        p["profile"] = name;
        p["level"] = std::string { name_of(LEVELS, level) };
        profiles.push_back(std::move(p));
    }
    j["needs"] = kinds_json(f.needs);
    j["requires-declaration"] = f.requires_declaration;
    j["replaces"] = f.replaces;
    return j;
}

Read<Feature> feature_from(const Json& j) {
    Reader r { j, "feature", {} };
    Feature f;
    f.id = r.str("id");
    r.where = std::format("feature {}", f.id);
    const std::string category { r.str("category") };
    if (const auto c = lookup(CATEGORIES, category)) f.category = *c;
    else r.fail(std::format("{}: `{}` is not a category", r.where, category));
    f.standard = r.str_or("standard");
    f.layer = r.str_or("layer");
    f.summary = r.str("summary");
    f.fix = r.str_or("fix");
    const std::string level { r.str("default") };
    if (const auto l = lookup(LEVELS, level)) f.default_level = *l;
    else r.fail(std::format("{}: `{}` is not a level", r.where, level));
    f.waivable = r.flag("waivable");
    if (r.has("profiles")) {
        const Json& ps = j["profiles"];
        if (!ps.is_array()) r.fail(r.where + ".profiles is not a list");
        else
            for (const auto& p : ps) {
                Reader pr { p, r.where + ".profiles[]", {} };
                const std::string name { pr.str("profile") };
                const std::string lv { pr.str("level") };
                const auto l = lookup(LEVELS, lv);
                if (!pr.ok() || !l) r.fail(pr.ok() ? std::format("{}: `{}` is not a level", r.where, lv) : pr.error);
                else f.profiles.emplace_back(name, *l);
            }
    }
    if (r.has("needs")) {
        f.needs = fact::Kinds::none;
        for (const auto& name : r.strings("needs")) {
            if (const auto k = fact::parse_kind(name)) f.needs |= *k;
            else r.fail(std::format("{}.needs: `{}` is not a kind of facts", r.where, name));
        }
    }
    f.requires_declaration = r.str_or("requires-declaration");
    f.replaces = r.flag_or("replaces", false);
    if (!r.ok()) return std::unexpected(r.error);
    return f;
}

Json to_json(const Profile& p) {
    Json j = Json::object();
    j["name"] = p.name;
    j["summary"] = p.summary;
    j["includes"] = p.includes;
    Json& categories = j["categories"] = Json::array();
    for (const auto& [category, level] : p.categories) {
        Json c = Json::object();
        c["category"] = std::string { name_of(CATEGORIES, category) };
        c["level"] = std::string { name_of(LEVELS, level) };
        categories.push_back(std::move(c));
    }
    j["replaces"] = p.replaces;
    Json& levels = j["features"] = Json::array();
    for (const auto& [id, level] : p.features) {
        Json f = Json::object();
        f["id"] = id;
        f["level"] = std::string { name_of(LEVELS, level) };
        levels.push_back(std::move(f));
    }
    return j;
}

Read<Profile> profile_from(const Json& j) {
    Reader r { j, "profile", {} };
    Profile p;
    p.name = r.str("name");
    p.summary = r.str_or("summary");
    if (r.has("includes")) p.includes = r.strings("includes");
    if (r.has("categories") && j["categories"].is_array())
        for (const auto& c : j["categories"]) {
            Reader cr { c, "profile.categories[]", {} };
            const auto category = lookup(CATEGORIES, cr.str("category"));
            const auto level = lookup(LEVELS, cr.str("level"));
            if (!cr.ok() || !category || !level) r.fail(std::format("profile {}: a category's level is not one", p.name));
            else p.categories.emplace_back(*category, *level);
        }
    p.replaces = r.flag_or("replaces", false);
    if (r.has("features") && j["features"].is_array())
        for (const auto& f : j["features"]) {
            Reader fr { f, "profile.features[]", {} };
            const std::string id { fr.str("id") };
            const auto level = lookup(LEVELS, fr.str("level"));
            if (!fr.ok() || !level) r.fail(std::format("profile {}: a feature's level is not one", p.name));
            else p.features.emplace_back(id, *level);
        }
    if (!r.ok()) return std::unexpected(r.error);
    return p;
}

Json to_json(const Target& t) {
    Json j = Json::object();
    j["triple"] = t.triple;
    j["os"] = t.os;
    j["family"] = t.family;
    j["arch"] = t.arch;
    j["env"] = t.env;
    j["pointer-width"] = t.pointer_width;
    j["endian"] = t.endian;
    j["features"] = t.features;
    j["debug-assertions"] = t.debug_assertions;
    return j;
}

Read<Target> target_from(const Json& j) {
    Reader r { j, "target", {} };
    Target t;
    t.triple = r.str("triple");
    t.os = r.str("os");
    t.family = r.str("family");
    t.arch = r.str("arch");
    t.env = r.str("env");
    t.pointer_width = r.count("pointer-width");
    t.endian = r.str("endian");
    t.features = r.strings("features");
    t.debug_assertions = r.flag("debug-assertions");
    if (!r.ok()) return std::unexpected(r.error);
    return t;
}

Json to_json(const ProviderInfo& p) {
    Json j = Json::object();
    j["name"] = p.name;
    j["extension-points"] = p.extension_points;
    Json& features = j["features"] = Json::array();
    for (const auto& f : p.features) features.push_back(to_json(f));
    Json& profiles = j["profiles"] = Json::array();
    for (const auto& pr : p.profiles) profiles.push_back(to_json(pr));
    j["replaces"] = p.replaces;
    Json& attributes = j["attributes"] = Json::array();
    for (const auto& a : p.attributes) {
        Json x = Json::object();
        x["name"] = a.name;
        x["summary"] = a.summary;
        x["region"] = a.region;
        attributes.push_back(std::move(x));
    }
    return j;
}

Read<ProviderInfo> provider_from(const Json& j) {
    Reader r { j, "provider", {} };
    ProviderInfo p;
    p.name = r.str("name");
    p.extension_points = r.strings("extension-points");
    p.replaces = r.strings("replaces");
    r.at("features");
    r.at("profiles");
    if (!r.ok()) return std::unexpected(r.error);
    if (!j["features"].is_array() || !j["profiles"].is_array()) return std::unexpected(std::format("provider {}: features and profiles are lists", p.name));
    for (const auto& f : j["features"]) {
        auto feature = feature_from(f);
        if (!feature) return std::unexpected(std::format("provider {}: {}", p.name, feature.error()));
        p.features.push_back(std::move(*feature));
    }
    for (const auto& pr : j["profiles"]) {
        auto profile = profile_from(pr);
        if (!profile) return std::unexpected(std::format("provider {}: {}", p.name, profile.error()));
        p.profiles.push_back(std::move(*profile));
    }
    if (j.contains("attributes") && j["attributes"].is_array())
        for (const auto& a : j["attributes"]) {
            Reader ar { a, std::format("provider {}.attributes[]", p.name), {} };
            AttributeSpec spec { ar.str("name"), ar.str_or("summary"), ar.str_or("region") };
            if (!ar.ok()) return std::unexpected(ar.error);
            p.attributes.push_back(std::move(spec));
        }
    return p;
}

ProviderInfo describe(const Provider& provider, bool rule, bool filter) {
    ProviderInfo p;
    p.name = std::string { provider.name() };
    if (rule) p.extension_points.emplace_back("rule");
    if (filter) p.extension_points.emplace_back("source-filter");
    p.features.assign(provider.features().begin(), provider.features().end());
    p.profiles.assign(provider.profiles().begin(), provider.profiles().end());
    for (const auto name : provider.replaces()) p.replaces.emplace_back(name);
    p.attributes.assign(provider.attributes().begin(), provider.attributes().end());
    return p;
}

std::string line(const Json& message) { return message.dump(-1, ' ', false, Json::error_handler_t::replace) + "\n"; }

Read<Json> message_from(std::string_view text) {
    Json j = Json::parse(text, nullptr, false);
    if (j.is_discarded()) return std::unexpected(std::format("not JSON: {}", text.substr(0, 120)));
    if (!j.is_object() || !j.contains("type") || !j["type"].is_string()) return std::unexpected("a message is an object with a type");
    return j;
}

} // namespace mcxx::plugin::wire
