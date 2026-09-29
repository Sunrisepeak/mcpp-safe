// MC3 facts as JSON (specs/mc3-facts.md §4.11), both ways.
module mcxx.plugin.wire;

import std;
import nlohmann.json;
import mcxx.msa;
import mcxx.plugin;
import :json;

namespace mcxx::plugin::wire {

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

} // namespace mcxx::plugin::wire
