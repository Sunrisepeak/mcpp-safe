// MC++'s plugin wire formats: MC3 facts as JSON (specs/mc3-facts.md §4.11) and the MC4 protocol's
// pieces (specs/mc4-plugins.md §6): features, profiles, providers, findings, targets. Reading is
// strict -- a member of the wrong type is an error naming it -- and writing then reading gives the
// same value (MC3-4.11-2).
export module mcxx.plugin.wire;

import std;
import nlohmann.json;
import mcxx.msa;
import mcxx.plugin;

export namespace mcxx::plugin::wire {

using Json = nlohmann::json;

inline constexpr int PROTOCOL { 1 };
inline constexpr std::string_view MC3_VERSION { "0.4.0" };   // 0.4.0 adds imports, 0.3.0 a declaration's `local`, 0.2.0 attributes; older documents are read too

template <class T>
using Read = std::expected<T, std::string>;

Json to_json(const msa::Range& range);
Read<msa::Range> range_from(const Json& j);

// A facts document: the facts of `path` (in `module`).
Json facts_to_json(const msa::fact::Facts& facts, std::string_view path, std::string_view module);
Read<msa::fact::Facts> facts_from_json(const Json& j);

Json to_json(const Finding& finding);
Read<Finding> finding_from(const Json& j);
Json to_json(const Feature& feature);
Read<Feature> feature_from(const Json& j);
Json to_json(const Profile& profile);
Read<Profile> profile_from(const Json& j);
Json to_json(const Target& target);
Read<Target> target_from(const Json& j);

// A provider as a plugin describes itself in `welcome`: its extension points, features, profiles,
// and whom it replaces.
struct ProviderInfo {
    std::string name;
    std::vector<std::string> extension_points;   // "rule", "source-filter"
    std::vector<Feature> features;
    std::vector<Profile> profiles;
    std::vector<std::string> replaces;
    std::vector<AttributeSpec> attributes;
};
Json to_json(const ProviderInfo& provider);
Read<ProviderInfo> provider_from(const Json& j);
ProviderInfo describe(const Provider& provider, bool rule, bool filter);

// One protocol message as one line (no line break inside), and back.
std::string line(const Json& message);
Read<Json> message_from(std::string_view line);

} // namespace mcxx::plugin::wire

namespace mcxx::plugin::wire {

namespace {

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
    d.exported = r.flag("exported");
    d.c_array = r.flag("c-array");
    d.pointer = r.flag("pointer");
    d.is_union = r.flag("union");
    d.c_variadic = r.flag("c-variadic");
    d.local = r.j.contains("local") && r.flag("local");   // 0.1.0 and 0.2.0 documents have none: false
}

} // namespace

Json to_json(const msa::Range& range) {
    Json j = Json::object();
    j["begin"] = Json { { "line", range.begin.line }, { "column", range.begin.column } };
    j["end"] = Json { { "line", range.end.line }, { "column", range.end.column } };
    return j;
}

Read<msa::Range> range_from(const Json& j) {
    auto position = [](const Json* p) -> std::optional<msa::Position> {
        if (p == nullptr || !p->is_object() || !p->contains("line") || !p->contains("column") || !(*p)["line"].is_number_unsigned() ||
            !(*p)["column"].is_number_unsigned())
            return std::nullopt;
        return msa::Position { (*p)["line"].get<std::uint32_t>(), (*p)["column"].get<std::uint32_t>() };
    };
    if (!j.is_object() || !j.contains("begin") || !j.contains("end")) return std::unexpected("a range has begin and end");
    const auto b = position(&j["begin"]);
    const auto e = position(&j["end"]);
    if (!b || !e) return std::unexpected("a position has a line and a column, counts");
    return msa::Range { *b, *e };
}

Json facts_to_json(const fact::Facts& f, std::string_view path, std::string_view module) {
    Json j = Json::object();
    j["mc3-version"] = std::string { MC3_VERSION };
    j["path"] = std::string { path };
    j["module"] = std::string { module };
    j["certainty"] = f.certainty == msa::Certainty::certain ? "certain" : "unknown";
    j["collected"] = kinds_json(f.collected);
    Json& decls = j["declarations"] = Json::array();
    for (const auto& d : f.declarations) decls.push_back(declaration_json(d));
    Json& inits = j["initializations"] = Json::array();
    for (const auto& i : f.initializations) {
        Json x = place(i);
        x["name"] = to_json(i.name);
        x["entity"] = i.entity;
        x["variable"] = i.variable;
        x["type"] = i.type;
        x["type-template"] = i.type_template;
        x["form"] = std::string { name_of(FORMS, i.form) };
        x["constructor"] = i.constructor;
        x["initializer-list-constructor"] = i.initializer_list_constructor;
        x["elements"] = i.elements;
        x["element-braced"] = i.element_braced;
        x["member-default"] = i.member_default;
        x["indeterminate"] = i.indeterminate;
        inits.push_back(std::move(x));
    }
    Json& casts = j["casts"] = Json::array();
    for (const auto& c : f.casts) {
        Json x = place(c);
        x["kind"] = std::string { name_of(CASTS, c.kind) };
        x["from"] = c.from;
        x["to"] = c.to;
        x["reinterprets"] = c.reinterprets;
        x["to-scalar"] = c.to_scalar;
        casts.push_back(std::move(x));
    }
    Json& allocations = j["allocations"] = Json::array();
    for (const auto& a : f.allocations) {
        Json x = place(a);
        x["delete"] = a.is_delete;
        x["array"] = a.array;
        x["type"] = a.type;
        allocations.push_back(std::move(x));
    }
    Json& arithmetic = j["pointer-arithmetic"] = Json::array();
    for (const auto& p : f.pointer_arithmetic) {
        Json x = place(p);
        x["op"] = p.op;
        x["type"] = p.type;
        arithmetic.push_back(std::move(x));
    }
    Json& gotos = j["gotos"] = Json::array();
    for (const auto& g : f.gotos) {
        Json x = place(g);
        x["label"] = g.label;
        gotos.push_back(std::move(x));
    }
    Json& macros = j["macros"] = Json::array();
    for (const auto& m : f.macros) {
        Json x = place(m);
        x["name"] = m.name;
        macros.push_back(std::move(x));
    }
    Json& uses = j["uses"] = Json::array();
    for (const auto& u : f.uses) {
        Json x = place(u);
        x["construct"] = u.construct;
        x["detail"] = u.detail;
        uses.push_back(std::move(x));
    }
    Json& includes = j["includes"] = Json::array();
    for (const auto& i : f.includes) {
        Json x = place(i);
        x["header"] = i.header;
        x["global-module-fragment"] = i.global_module_fragment;
        includes.push_back(std::move(x));
    }
    Json& suppressions = j["suppressions"] = Json::array();
    for (const auto& s : f.suppressions) {
        Json x = place(s);
        x["ids"] = s.ids;
        x["entity"] = s.entity;
        x["declaration"] = s.declaration;
        x["reason"] = s.reason;
        suppressions.push_back(std::move(x));
    }
    Json& attributes = j["attributes"] = Json::array();
    for (const auto& a : f.attributes) {
        Json x = place(a);
        x["name"] = a.name;
        x["arguments"] = a.arguments;
        x["name-range"] = to_json(a.name_range);
        x["entity"] = a.entity;
        x["declaration"] = a.declaration;
        x["kind"] = kind_name(a.kind);
        attributes.push_back(std::move(x));
    }
    Json& imports = j["imports"] = Json::array();
    for (const auto& i : f.imports) {
        Json x = place(i);
        x["module"] = i.module;
        x["name"] = to_json(i.name);
        x["exported"] = i.exported;
        Json& interfaces = x["interfaces"] = Json::array();
        for (const auto& in : i.interfaces) {
            Json levels = Json::object();
            for (const auto& [id, level] : in.levels) levels[id] = level;
            Json exported = Json::array();
            for (const auto& d : in.exported) exported.push_back(declaration_json(d));
            Json y = Json::object();
            y["module"] = in.module;
            y["found"] = in.found;
            y["profiles"] = in.profiles;
            y["levels"] = std::move(levels);
            y["exported"] = std::move(exported);
            interfaces.push_back(std::move(y));
        }
        imports.push_back(std::move(x));
    }
    return j;
}

Read<fact::Facts> facts_from_json(const Json& j) {
    if (!j.is_object()) return std::unexpected("facts is not an object");
    if (!j.contains("mc3-version")
        || (j["mc3-version"] != std::string { MC3_VERSION } && j["mc3-version"] != "0.3.0" && j["mc3-version"] != "0.2.0" && j["mc3-version"] != "0.1.0"))
        return std::unexpected(std::format("facts are not MC3 {} (nor 0.3.0, 0.2.0, 0.1.0)", MC3_VERSION));
    fact::Facts f;
    std::string error;
    Reader top { j, "facts", {} };
    const std::string certainty { top.str("certainty") };
    if (certainty != "certain" && certainty != "unknown") top.fail("facts.certainty is neither certain nor unknown");
    f.certainty = certainty == "certain" ? msa::Certainty::certain : msa::Certainty::unknown;
    f.collected = fact::Kinds::none;
    for (const auto& name : top.strings("collected")) {
        if (const auto k = fact::parse_kind(name)) f.collected |= *k;
        else top.fail(std::format("facts.collected: `{}` is not a kind of facts", name));
    }
    if (!top.ok()) return std::unexpected(top.error);
    read_list(j, "declarations", f.declarations, error, [](Reader& r, fact::Declaration& d) { read_declaration(r, d); });
    read_list(j, "initializations", f.initializations, error, [](Reader& r, fact::Initialization& i) {
        i.name = r.range("name");
        i.entity = r.str("entity");
        i.variable = r.str("variable");
        i.type = r.str("type");
        i.type_template = r.str("type-template");
        const std::string form { r.str("form") };
        if (const auto v = lookup(FORMS, form)) i.form = *v;
        else r.fail(std::format("{}.form: `{}` is not a form of initialization", r.where, form));
        i.constructor = r.str("constructor");
        i.initializer_list_constructor = r.flag("initializer-list-constructor");
        i.elements = r.count("elements");
        i.element_braced = r.flag("element-braced");
        i.member_default = r.flag("member-default");
        i.indeterminate = r.flag("indeterminate");
    });
    read_list(j, "casts", f.casts, error, [](Reader& r, fact::Cast& c) {
        const std::string kind { r.str("kind") };
        if (const auto v = lookup(CASTS, kind)) c.kind = *v;
        else r.fail(std::format("{}.kind: `{}` is not a kind of cast", r.where, kind));
        c.from = r.str("from");
        c.to = r.str("to");
        c.reinterprets = r.flag("reinterprets");
        c.to_scalar = r.flag("to-scalar");
    });
    read_list(j, "allocations", f.allocations, error, [](Reader& r, fact::Allocation& a) {
        a.is_delete = r.flag("delete");
        a.array = r.flag("array");
        a.type = r.str("type");
    });
    read_list(j, "pointer-arithmetic", f.pointer_arithmetic, error, [](Reader& r, fact::PointerArithmetic& p) {
        p.op = r.str("op");
        p.type = r.str("type");
    });
    read_list(j, "gotos", f.gotos, error, [](Reader& r, fact::Goto& g) { g.label = r.str("label"); });
    read_list(j, "macros", f.macros, error, [](Reader& r, fact::MacroDefinition& m) { m.name = r.str("name"); });
    read_list(j, "uses", f.uses, error, [](Reader& r, fact::Use& u) {
        u.construct = r.str("construct");
        u.detail = r.str("detail");
    });
    read_list(j, "includes", f.includes, error, [](Reader& r, fact::Include& i) {
        i.header = r.str("header");
        i.global_module_fragment = r.flag("global-module-fragment");
    });
    read_list(j, "suppressions", f.suppressions, error, [](Reader& r, fact::Suppression& s) {
        s.ids = r.strings("ids");
        s.entity = r.str("entity");
        s.declaration = r.str("declaration");
        s.reason = r.str("reason");
    });
    if (j.contains("attributes"))   // MC3 0.1.0 documents written before attributes have none
        read_list(j, "attributes", f.attributes, error, [](Reader& r, fact::Attribute& a) {
            a.name = r.str("name");
            a.arguments = r.strings("arguments");
            a.name_range = r.range("name-range");
            a.entity = r.str("entity");
            a.declaration = r.str("declaration");
            const std::string kind { r.str("kind") };
            if (const auto k = kind_from(kind)) a.kind = *k;
            else r.fail(std::format("{}.kind: `{}` is not a kind of declaration", r.where, kind));
        });
    if (j.contains("imports"))   // documents before MC3 0.4.0 have none
        read_list(j, "imports", f.imports, error, [](Reader& r, fact::Import& i) {
            i.module = r.str("module");
            i.name = r.range("name");
            i.exported = r.flag("exported");
            const Json* list { r.at("interfaces") };
            if (list == nullptr) return;
            if (!list->is_array()) return r.fail(std::format("{}.interfaces is not a list", r.where));
            for (std::size_t k { 0 }; k < list->size(); ++k) {
                Reader ir { (*list)[k], std::format("{}.interfaces[{}]", r.where, k), {} };
                fact::Import::Interface in;
                in.module = ir.str("module");
                in.found = ir.flag("found");
                in.profiles = ir.strings("profiles");
                if (const Json* levels = ir.at("levels"); levels != nullptr) {
                    if (!levels->is_object()) ir.fail(std::format("{}.levels is not an object", ir.where));
                    else
                        for (auto it = levels->begin(); it != levels->end(); ++it) {
                            if (!it.value().is_string()) ir.fail(std::format("{}.levels.{} is not a string", ir.where, it.key()));
                            else in.levels.emplace_back(it.key(), it.value().get<std::string>());
                        }
                }
                if (const Json* exported = ir.at("exported"); exported != nullptr && exported->is_array()) {
                    for (std::size_t d { 0 }; d < exported->size(); ++d) {
                        Reader dr { (*exported)[d], std::format("{}.exported[{}]", ir.where, d), {} };
                        fact::Declaration decl;
                        read_place(dr, decl);
                        read_declaration(dr, decl);
                        if (!dr.ok()) ir.fail(dr.error);
                        in.exported.push_back(std::move(decl));
                    }
                } else ir.fail(std::format("{}.exported is missing or not a list", ir.where));
                if (!ir.ok()) return r.fail(ir.error);
                i.interfaces.push_back(std::move(in));
            }
        });
    if (!error.empty()) return std::unexpected(error);
    return f;
}

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
