// mcxx.plugin.wire:json (internal) -- what reading and writing MC3 facts and MC4 messages share: a reader
// whose missing or mistyped member is an error naming it, the words enums are written with, places,
// declarations.
module mcxx.plugin.wire:json;

import std;
import nlohmann.json;
import mcxx.msa;
import mcxx.plugin;
import :api;

namespace mcxx::plugin::wire {

namespace fact = msa::fact;

// ---- reading helpers: a missing or mistyped member is an error that names it -------------------

struct Reader {
    const Json& j;
    std::string where;
    std::string error;

    bool ok() const { return error.empty(); }
    const Json* at(std::string_view key) {
        if (!j.is_object()) {
            fail(std::format("{} is not an object", where));
            return nullptr;
        }
        const auto it = j.find(std::string { key });
        if (it == j.end()) {
            fail(std::format("{}.{} is missing", where, key));
            return nullptr;
        }
        return &*it;
    }
    void fail(std::string e) {
        if (error.empty()) error = std::move(e);
    }
    std::string str(std::string_view key) {
        const Json* v { at(key) };
        if (v == nullptr) return {};
        if (!v->is_string()) fail(std::format("{}.{} is not a string", where, key));
        return v->is_string() ? v->get<std::string>() : std::string {};
    }
    bool flag(std::string_view key) {
        const Json* v { at(key) };
        if (v == nullptr) return false;
        if (!v->is_boolean()) fail(std::format("{}.{} is not a boolean", where, key));
        return v->is_boolean() && v->get<bool>();
    }
    std::uint32_t count(std::string_view key) {
        const Json* v { at(key) };
        if (v == nullptr) return 0;
        if (!v->is_number_unsigned()) fail(std::format("{}.{} is not a count", where, key));
        return v->is_number_unsigned() ? v->get<std::uint32_t>() : 0;
    }
    std::vector<std::string> strings(std::string_view key) {
        std::vector<std::string> out;
        const Json* v { at(key) };
        if (v == nullptr) return out;
        if (!v->is_array()) {
            fail(std::format("{}.{} is not a list", where, key));
            return out;
        }
        for (const auto& s : *v) {
            if (!s.is_string()) {
                fail(std::format("{}.{} holds something that is not a string", where, key));
                return out;
            }
            out.push_back(s.get<std::string>());
        }
        return out;
    }
    msa::Range range(std::string_view key) {
        const Json* v { at(key) };
        if (v == nullptr) return {};
        auto r = range_from(*v);
        if (!r) fail(std::format("{}.{}: {}", where, key, r.error()));
        return r.value_or(msa::Range {});
    }
    // Optional members: absent is the default.
    std::string str_or(std::string_view key, std::string fallback = {}) { return has(key) ? str(key) : fallback; }
    bool flag_or(std::string_view key, bool fallback) { return has(key) ? flag(key) : fallback; }
    bool has(std::string_view key) const { return j.is_object() && j.contains(std::string { key }); }
};

template <class Enum, std::size_t N>
std::optional<Enum> lookup(const std::pair<Enum, std::string_view> (&table)[N], std::string_view name) {
    for (const auto& [value, n] : table)
        if (n == name) return value;
    return std::nullopt;
}

template <class Enum, std::size_t N>
std::string_view name_of(const std::pair<Enum, std::string_view> (&table)[N], Enum value) {
    for (const auto& [v, n] : table)
        if (v == value) return n;
    return table[0].second;
}

constexpr std::pair<fact::InitForm, std::string_view> FORMS[] {
    { fact::InitForm::default_init, "default" }, { fact::InitForm::copy, "copy" }, { fact::InitForm::direct, "direct" },
    { fact::InitForm::direct_list, "direct-list" }, { fact::InitForm::copy_list, "copy-list" },
};
constexpr std::pair<fact::CastKind, std::string_view> CASTS[] {
    { fact::CastKind::static_cast_, "static_cast" }, { fact::CastKind::dynamic_cast_, "dynamic_cast" },
    { fact::CastKind::const_cast_, "const_cast" },   { fact::CastKind::reinterpret_cast_, "reinterpret_cast" },
    { fact::CastKind::c_style, "c-style" },          { fact::CastKind::functional, "functional" },
};
constexpr std::pair<Level, std::string_view> LEVELS[] { { Level::allow, "allow" }, { Level::warn, "warn" }, { Level::deny, "deny" } };
constexpr std::pair<Category, std::string_view> CATEGORIES[] {
    { Category::iso, "iso" }, { Category::policy, "policy" }, { Category::library, "library" }, { Category::pitfall, "pitfall" }, { Category::extension, "extension" },
};

// msa::Kind by its MC3 name: to_string's words joined by -.
std::string kind_name(msa::Kind kind) {
    std::string out { msa::to_string(kind) };
    std::ranges::replace(out, ' ', '-');
    return out;
}
std::optional<msa::Kind> kind_from(std::string_view name) {
    for (int k { 0 }; k <= static_cast<int>(msa::Kind::label); ++k)
        if (kind_name(static_cast<msa::Kind>(k)) == name) return static_cast<msa::Kind>(k);
    return std::nullopt;
}

Json place(const fact::Place& p) {
    Json j = Json::object();
    j["range"] = to_json(p.range);
    j["container"] = p.container;
    return j;
}

void read_place(Reader& r, fact::Place& p) {
    p.range = r.range("range");
    p.container = r.str("container");
}

Json kinds_json(fact::Kinds set) {
    Json out = Json::array();
    for (const auto name : fact::names(set)) out.push_back(std::string { name });
    return out;
}

template <class T, class F>
void read_list(const Json& doc, std::string_view key, std::vector<T>& out, std::string& error, F read_one) {
    const auto it = doc.find(std::string { key });
    if (it == doc.end() || !it->is_array()) {
        if (error.empty()) error = std::format("facts.{} is missing or not a list", key);
        return;
    }
    std::size_t i { 0 };
    for (const auto& item : *it) {
        Reader r { item, std::format("facts.{}[{}]", key, i++), {} };
        T value;
        read_place(r, value);
        read_one(r, value);
        if (!r.ok()) {
            if (error.empty()) error = r.error;
            return;
        }
        out.push_back(std::move(value));
    }
}

Json declaration_json(const fact::Declaration& d) {
    Json x = place(d);
    x["name"] = to_json(d.name);
    x["entity"] = d.entity;
    x["qualified-name"] = d.qualified_name;
    x["kind"] = kind_name(d.kind);
    x["type"] = d.type;
    x["templates"] = d.templates;
    if (!d.bases.empty()) x["bases"] = d.bases;   // MC3 0.5.0: a class's, when there are any
    if (!d.template_parameters.empty()) x["template-parameters"] = d.template_parameters;   // MC3 0.6.0: a template's
    if (d.parameters) x["parameters"] = *d.parameters;   // MC3 0.7.0: a function's, when said
    x["exported"] = d.exported;
    x["c-array"] = d.c_array;
    x["pointer"] = d.pointer;
    x["union"] = d.is_union;
    x["c-variadic"] = d.c_variadic;
    x["local"] = d.local;
    return x;
}

void read_declaration(Reader& r, fact::Declaration& d) {
    d.name = r.range("name");
    d.entity = r.str("entity");
    d.qualified_name = r.str("qualified-name");
    const std::string kind { r.str("kind") };
    if (const auto k = kind_from(kind)) d.kind = *k;
    else r.fail(std::format("{}.kind: `{}` is not a kind of declaration", r.where, kind));
    d.type = r.str("type");
    d.templates = r.strings("templates");
    if (r.j.contains("bases")) d.bases = r.strings("bases");   // before 0.5.0: none
    if (r.j.contains("template-parameters")) d.template_parameters = r.strings("template-parameters");   // before 0.6.0: none
    if (r.j.contains("parameters")) d.parameters = r.strings("parameters");   // before 0.7.0: not said
    d.exported = r.flag("exported");
    d.c_array = r.flag("c-array");
    d.pointer = r.flag("pointer");
    d.is_union = r.flag("union");
    d.c_variadic = r.flag("c-variadic");
    d.local = r.j.contains("local") && r.flag("local");   // 0.1.0 and 0.2.0 documents have none: false
}

} // namespace mcxx::plugin::wire
