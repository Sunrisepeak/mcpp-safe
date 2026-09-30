// What a declared type names (mcxx.frontend:declared): MC3's `templates` and `pointer` of each
// declaration as Clang has them -- Clang's collect_templates and holds_pointer over the canonical type
// (modules/backend/clang/src/facts.cpp): a specialization's template, then each of its type arguments'
// (the defaulted ones too, a pack not looked into); a dependent specialization's template and the
// arguments as written; an alias's what it names (an imported one's as its interface gives them). A
// placeholder's type is what it is deduced to be, whose text F1 does not print (Clang prints the sugar
// the deduction went through): said, not guessed.
module mcxx.frontend;

import std;
import mcxx.msa;
import mcxx.base;
import :resolver;

namespace mcxx::frontend {

namespace resolution {

Parameter parameter_of(std::string_view written) {
    Parameter p;
    std::string_view rest { written };
    if (const auto equal = written.find(" = "); equal != std::string_view::npos) {
        p.default_type = std::string { written.substr(equal + 3) };
        rest = written.substr(0, equal);
    }
    if (rest.starts_with("template class")) {
        p.sort = Parameter::Sort::template_;
        rest.remove_prefix(std::string_view { "template class" }.size());
    } else if (rest == "class" || rest.starts_with("class ")) {
        p.sort = Parameter::Sort::type;
        rest.remove_prefix(std::string_view { "class" }.size());
    } else {
        // `std::size_t N`: its name after the type.
        p.sort = Parameter::Sort::value;
        const auto space = rest.rfind(' ');
        rest = space == std::string_view::npos ? std::string_view {} : rest.substr(space + 1);
    }
    if (p.sort != Parameter::Sort::type) p.default_type.clear();
    while (rest.starts_with(' ')) rest.remove_prefix(1);
    if (rest.starts_with("...")) {
        p.pack = true;
        rest.remove_prefix(3);
    }
    p.name = std::string { rest };
    if (p.sort == Parameter::Sort::value) {
        const auto space = written.substr(0, written.find(" = ")).rfind(' ');
        if (space != std::string_view::npos) p.value_type = std::string { written.substr(0, space) };
    }
    return p;
}

std::string written(const Parameter& p) {
    const auto named = [&](std::string head) {
        if (p.pack) return head + " ..." + p.name;
        return p.name.empty() ? head : head + " " + p.name;
    };
    switch (p.sort) {
    case Parameter::Sort::type: return named("class") + (p.default_type.empty() ? "" : " = " + p.default_type);
    case Parameter::Sort::value: return named(p.value_type.empty() ? "auto" : p.value_type);
    case Parameter::Sort::template_: return named("template class");
    }
    return {};
}

namespace {

// The names a type's text writes, each with whether it follows `::` (a qualified name's later part).
std::vector<std::pair<std::string, bool>> words_of(std::string_view text) {
    std::vector<std::pair<std::string, bool>> out;
    for (std::size_t i { 0 }; i < text.size();) {
        const auto c = static_cast<unsigned char>(text[i]);
        if (std::isalpha(c) || c == '_') {
            std::size_t j { i };
            while (j < text.size() && (std::isalnum(static_cast<unsigned char>(text[j])) || text[j] == '_')) ++j;
            const bool qualified { i >= 2 && text.substr(i - 2, 2) == "::" };
            out.emplace_back(std::string { text.substr(i, j - i) }, qualified);
            i = j;
        } else if (std::isdigit(c)) {
            while (i < text.size() && (std::isalnum(static_cast<unsigned char>(text[i])) || text[i] == '_' || text[i] == '.')) ++i;
        } else {
            ++i;
        }
    }
    return out;
}

// Whether `(` opens a function type's parameters or a declarator's parentheses at the text's own level
// (not inside template arguments).
bool function_like(std::string_view text) {
    int depth { 0 };
    for (const char c : text) {
        if (c == '<') ++depth;
        else if (c == '>') --depth;
        else if (c == '(' && depth == 0) return true;
    }
    return false;
}

// A literal argument (`3`, `true`, `'a'`): not a type.
bool literal(std::string_view text) {
    return !text.empty() && (std::isdigit(static_cast<unsigned char>(text.front())) || text.front() == '\'' || text.front() == '"' || text.front() == '-' ||
                             text == "true" || text == "false" || text == "nullptr");
}

} // namespace

bool Resolver::dependent(const Typed& type, int depth) {
    if (depth > 8) return false;
    for (const auto& [name, qualified] : words_of(type.text)) {
        if (qualified) continue;
        if (name == "auto") return true;
        if (keyword(name)) continue;
        if (type.bindings) {
            if (const auto bound = type.bindings->by_name.find(name); bound != type.bindings->by_name.end()) {
                if (dependent(bound->second, depth + 1)) return true;
                continue;
            }
        }
        if (type.imported) continue;   // an imported type's names are its own template's, not the file's
        if (const auto found = unqualified(name, type.at, true); found && found->kind == msa::Kind::template_parameter) return true;
    }
    return false;
}

std::optional<Target> Resolver::template_named(const Typed& type, bool& inside) {
    inside = false;
    const std::string text { bare(type.text) };
    if (text.find('<') != std::string::npos) return std::nullopt;
    const std::string name { class_name_of(text) };
    if (name.empty() || keyword(name)) return std::nullopt;
    const auto found { named_in(name, type) };
    if (!found || !class_kind(found->kind) || parameters_of(*found).empty()) return std::nullopt;
    if (found->declaration >= 0 && !type.imported) {
        const auto& d = ds_[static_cast<std::size_t>(found->declaration)];
        inside = type.at >= d.first_token && type.at <= d.last_token;
    }
    return found;
}

bool Resolver::collect(const Typed& written, std::vector<std::string>& out, bool& pointer, int depth) {
    if (depth > 8) return true;
    const auto add = [&](const std::string& name) {
        if (!std::ranges::contains(out, name)) out.push_back(name);
    };
    std::string text { bare(written.text) };
    // A pointer, an array: what they are of. A function type, a pointer to one: nothing in it is
    // looked into (Clang's collect_templates does not); a pointer to one is a pointer.
    for (int guard { 0 }; guard < 8; ++guard) {
        if (text.ends_with("::*")) return true;   // a member pointer: no pointer, nothing looked into
        if (text.ends_with("*const")) text.erase(text.size() - 5);
        if (text.ends_with('*')) {
            pointer = true;
            text.pop_back();
            text = bare(text);
            continue;
        }
        if (const auto square = text.find('['); square != std::string::npos && square > 0 && text.back() == ']' &&
                                                 (text.find('<') == std::string::npos || text.rfind('>') < square)) {
            text = bare(text.substr(0, square));
            continue;
        }
        break;
    }
    if (function_like(text)) {
        if (text.find("(*") != std::string::npos) pointer = true;
        return true;
    }
    if (written.bindings) {
        if (const auto bound = written.bindings->by_name.find(text); bound != written.bindings->by_name.end())
            return collect(bound->second, out, pointer, depth + 1);
    }
    const Typed type { derived(written, text) };
    if (text.find(">::") != std::string::npos) {
        // A member of a specialization (`std::vector<int>::iterator`): a dependent one is nothing
        // Clang looks into; another is what that specialization defines it as, which F1 does not see.
        return dependent(type);
    }
    const std::string name { class_name_of(text) };
    if (name.empty()) return true;
    if (name == "decltype" || name == "__typeof__") return false;
    if (keyword(name)) return true;   // a builtin; `auto` of a generic lambda's parameter
    const auto found { named_in(name, type) };
    if (!found) return false;
    const auto args { arguments_of(text) };
    if (found->kind == msa::Kind::template_parameter || found->kind == msa::Kind::enum_ || found->kind == msa::Kind::enumerator) return true;
    if (found->kind == msa::Kind::type_alias) {
        const auto parameters { parameters_of(*found) };
        if (!parameters.empty()) {
            // A decltype among its arguments: what that is, F1 does not evaluate.
            if (std::ranges::any_of(args, [](const std::string& a) { return a.find("decltype") != std::string::npos; })) return false;
            // An alias template: in a dependent type, itself and its arguments as written; otherwise
            // what it names, its parameters the arguments.
            const bool depends { std::ranges::any_of(args, [&](const std::string& a) { return dependent(derived(type, a)); }) };
            if (depends) {
                add(found->qualified);
                bool known { true };
                for (const auto& a : args)
                    if (!literal(a)) known = collect(derived(type, a), out, pointer, depth + 1) && known;
                return known;
            }
            // Otherwise what it names -- when that is a class template's specialization, a pointer or an
            // array (Clang's collect_templates takes those first, whatever sugar they are under). Else
            // Clang finds the alias template's specialization in the sugar: itself and its arguments as
            // written (`std::remove_extent_t<T> *`, what `shared_ptr<T>::get()` returns).
            std::vector<std::string> named;
            bool named_pointer { false };
            const Typed target { unbound(type) };
            if (!collect(target, named, named_pointer, depth + 1)) return false;
            if (!named.empty() || named_pointer || bare(target.text).ends_with(']')) {
                for (const auto& n : named) add(n);
                pointer = pointer || named_pointer;
                return true;
            }
            add(found->qualified);
            bool known { true };
            for (std::size_t i { 0 }; i < args.size(); ++i) {
                const bool type_argument { i < parameters.size() ? parameters[i].sort == Parameter::Sort::type || parameters[i].pack : !literal(args[i]) };
                if (type_argument && !literal(args[i])) known = collect(derived(type, args[i]), out, pointer, depth + 1) && known;
            }
            return known;
        }
        if (!args.empty()) return false;
        // A member alias of a class template (`std::map::iterator`): what it names in the
        // specialization it is read in, its text with the specialization's arguments.
        const std::string owner { scope_of(found->qualified) };
        if (found->declaration < 0 && imported_parameters_.contains(owner)) {
            if (!written.bindings || written.bindings->owner != owner) return false;
            const Typed aliased { found->type, 0, owner, true, {}, written.bindings };
            if (aliased.text.empty()) return false;
            return collect(aliased, out, pointer, depth + 1);
        }
        if (found->declaration < 0) {
            // An imported alias: the templates Clang found in what it names.
            if (const auto known = imported_templates_.find(found->qualified); known != imported_templates_.end())
                for (const auto& t : known->second) add(t);
            if (imported_pointers_.contains(found->qualified)) pointer = true;
            return true;
        }
        const auto& d = ds_[static_cast<std::size_t>(found->declaration)];
        const bool member { written.bindings && scope_of(found->qualified) == written.bindings->owner };
        const Typed aliased { type_text(syntax_, d), d.name_token, {}, false, {}, member ? written.bindings : nullptr };
        if (aliased.text.empty()) return false;
        return collect(aliased, out, pointer, depth + 1);
    }
    if (!class_kind(found->kind)) return false;
    const auto parameters { parameters_of(*found) };
    if (args.empty()) {
        if (parameters.empty()) return true;   // a class
        // A class template's name alone: inside it, the injected-class-name (itself, as it is written
        // there: dependent); outside, a specialization whose arguments are deduced.
        bool inside { false };
        (void)template_named(type, inside);
        return inside;   // the injected-class-name: nothing Clang collects
    }
    add(found->qualified);
    if (std::ranges::any_of(args, [&](const std::string& a) { return dependent(derived(type, a)); })) {
        bool known { true };
        for (std::size_t i { 0 }; i < args.size(); ++i) {
            const bool type_argument { i < parameters.size() ? parameters[i].sort == Parameter::Sort::type || parameters[i].pack : !literal(args[i]) };
            if (type_argument && !literal(args[i])) known = collect(derived(type, args[i]), out, pointer, depth + 1) && known;
        }
        return known;
    }
    if (parameters.empty()) return false;   // a template whose parameters F1 cannot see
    const auto bindings { bindings_for(type, found->qualified) };
    for (std::size_t i { 0 }; i < parameters.size(); ++i) {
        const auto& p = parameters[i];
        if (p.pack) break;   // Clang's one pack argument: not looked into
        if (p.sort != Parameter::Sort::type) continue;
        if (i < args.size()) {
            if (!collect(derived(type, args[i]), out, pointer, depth + 1)) return false;
            continue;
        }
        if (!bindings) return false;
        const auto fallback = bindings->by_name.find(p.name);
        if (fallback == bindings->by_name.end()) return false;
        if (!collect(fallback->second, out, pointer, depth + 1)) return false;
    }
    return true;
}

std::optional<std::string> Resolver::using_target(std::size_t u) {
    const auto& using_ = syntax_.usings[u];
    if (using_.directive) return std::nullopt;
    const auto scope { scope_named(scope_of(using_.name), using_.at) };
    if (!scope) return std::nullopt;
    const auto found { in_scope(*scope, std::string { last_component(using_.name) }, t_.size()) };
    if (!found) return std::nullopt;
    return found->qualified;
}

DeclaredType Resolver::declared(std::size_t i) {
    const auto& d = ds_[i];
    DeclaredType out;
    const bool returns { d.kind == msa::Kind::function || d.kind == msa::Kind::method };
    const bool typed_kind { returns || d.kind == msa::Kind::variable || d.kind == msa::Kind::field || d.kind == msa::Kind::parameter ||
                            d.kind == msa::Kind::type_alias };
    out.pointer = d.pointer;
    if (class_kind(d.kind) && d.definition) out.bases = bases_of(static_cast<std::int32_t>(i));
    if (class_kind(d.kind) || d.kind == msa::Kind::type_alias || function_kind(d.kind))
        for (const auto& p : parameters_of(target_of(static_cast<std::int32_t>(i)))) out.template_parameters.push_back(written(p));
    if (function_kind(d.kind)) out.parameters = function_parameters(target_of(static_cast<std::int32_t>(i)));
    // An enumerator's type is its enumeration, a namespace alias's the namespace it names (MC3 0.8.0).
    if (d.kind == msa::Kind::enumerator) {
        if (d.parent >= 0) out.type = qualified_[static_cast<std::size_t>(d.parent)];
        return out;
    }
    if (d.kind == msa::Kind::namespace_alias) {
        if (const auto named = alias_target(target_of(static_cast<std::int32_t>(i)))) out.type = *named;
        else {
            out.type_certain = false;
            out.why = "unknown";
        }
        return out;
    }
    if (!typed_kind) return out;
    out.type = type_text(syntax_, d, returns);
    const auto uncertain = [&](std::string why, bool type_too) {
        out.templates_certain = false;
        // A pointer the declarator writes is one (not a `*` inside a function type's parameters).
        const std::string written_type { bare(out.type) };
        out.pointer_certain = !out.type.empty() && (written_type.ends_with('*') || written_type.ends_with("*const")) &&
                              written_type.find("::*") == std::string::npos;
        if (out.pointer_certain) out.pointer = true;
        if (type_too) out.type_certain = false;
        out.why = std::move(why);
    };
    // A structured binding's group (`auto [a, b] = e;`): what the whole is deduced to be.
    if (d.name.starts_with('[')) {
        uncertain("deduced", true);
        if (const auto whole = binding_group(i, 0); whole && !whole->through_template) {
            std::vector<std::string> templates;
            bool pointer { false };
            if (collect(*whole, templates, pointer)) {
                out.templates = std::move(templates);
                out.pointer = pointer;
                out.templates_certain = out.pointer_certain = true;
            }
        }
        return out;
    }
    if (placeholder(d) && d.kind != msa::Kind::parameter) {
        // A deduced type: its text is not F1's to print; what it names is, when F1 deduces it -- not
        // in a template, where an initializer that depends on its parameters leaves it undeduced.
        uncertain("deduced", true);
        bool in_template { false };
        for (auto a = d.parent; a >= 0 && !in_template; a = ds_[static_cast<std::size_t>(a)].parent)
            in_template = !parameters_of(target_of(a)).empty();
        // A generic lambda's body is a template's too: an `auto` parameter visible where it is.
        for (std::size_t j { 0 }; j < i && !in_template && d.in_lambda; ++j) {
            const auto& p = ds_[j];
            in_template = p.kind == msa::Kind::parameter && p.in_lambda && placeholder(p) && p.name_token < d.name_token && p.visible_end >= d.name_token;
        }
        if (!returns && !in_template) {
            if (const auto deduced = typed(target_of(static_cast<std::int32_t>(i))); deduced && !deduced->through_template) {
                std::vector<std::string> templates;
                bool pointer { false };
                if (collect(*deduced, templates, pointer)) {
                    out.templates = std::move(templates);
                    out.pointer = pointer;
                    out.templates_certain = out.pointer_certain = true;
                }
            }
        }
        return out;
    }
    if (out.type.empty()) {
        uncertain("unknown", true);
        return out;
    }
    const Typed written { out.type, d.name_token, {}, false, {}, nullptr };
    bool inside { false };
    if (const auto t = template_named(written, inside)) {
        if (!inside) {
            // `std::lock_guard lock { m };`: the arguments are deduced from the initializer.
            uncertain("deduced", true);
            return out;
        }
        // The injected-class-name, as Clang prints it: the template's own name with its parameters.
        std::string params;
        for (const auto& p : parameters_of(*t)) params += (params.empty() ? "" : ", ") + p.name + (p.pack ? "..." : "");
        const std::string simple { last_component(t->qualified) };
        for (std::size_t at { out.type.find(simple) }; at != std::string::npos; at = out.type.find(simple, at + 1)) {
            const std::size_t end { at + simple.size() };
            const bool starts { at == 0 || !(std::isalnum(static_cast<unsigned char>(out.type[at - 1])) || out.type[at - 1] == '_') };
            const bool ends { end >= out.type.size() || !(std::isalnum(static_cast<unsigned char>(out.type[end])) || out.type[end] == '_') };
            if (starts && ends) {
                out.type.insert(end, "<" + params + ">");
                break;
            }
        }
    }
    // `T x[] = { a, b, c }`: the bound its initializer gives, as Clang prints it (`T[3]`). A parameter's
    // is as written (`char *argv[]`: Clang prints its type before the adjustment to a pointer).
    if (const auto open = out.type.find("[]"); open != std::string::npos && d.kind != msa::Kind::parameter) {
        std::size_t k { d.declarator_end };
        if (is(k, Kind::equal)) ++k;
        if (!is(k, Kind::l_brace)) {
            uncertain("deduced", true);   // a string literal's length, or no initializer
            return out;
        }
        std::size_t elements { 0 };
        bool any { false };
        int nesting { 0 };
        for (std::size_t j { k + 1 }; j < t_.size(); ++j) {
            if (nesting == 0 && is(j, Kind::r_brace)) break;
            if (is(j, Kind::l_paren) || is(j, Kind::l_square) || is(j, Kind::l_brace)) ++nesting;
            else if (is(j, Kind::r_paren) || is(j, Kind::r_square) || is(j, Kind::r_brace)) --nesting;
            if (nesting == 0 && is(j, Kind::comma)) {
                if (any) ++elements;
                any = false;
            } else {
                any = true;
            }
        }
        if (any) ++elements;
        out.type.replace(open, 2, std::format("[{}]", elements));
    }
    std::vector<std::string> templates;
    bool pointer { false };   // what the canonical type holds (a `*` in a function type's parameters is none)
    if (!collect(written, templates, pointer)) {
        uncertain("unknown", false);
        return out;
    }
    out.templates = std::move(templates);
    out.pointer = pointer;
    return out;
}

} // namespace resolution

msa::Range fact_name(const Syntax& syntax, const Declaration& d) {
    if (d.kind == msa::Kind::type_alias && d.name_token + 1 < syntax.pp.tokens.size() && syntax.pp.tokens[d.name_token].spelling == "using")
        return token_range(syntax, d.name_token + 1, d.name_token + 1);
    return selection_range(syntax, d);
}

std::vector<DeclaredType> declared_types(const Syntax& syntax, const Imported& imported) {
    resolution::Resolver resolver { syntax, imported };
    (void)resolver.run();
    const base::trace::Span span { "frontend.lookup", "declared types", std::format("{} declarations", syntax.declarations.size()) };
    std::vector<DeclaredType> out;
    out.reserve(syntax.declarations.size());
    for (std::size_t i { 0 }; i < syntax.declarations.size(); ++i) out.push_back(resolver.declared(i));
    for (std::size_t u { 0 }; u < syntax.usings.size(); ++u) {
        DeclaredType named;
        if (!syntax.usings[u].directive) {
            if (auto target = resolver.using_target(u)) named.type = std::move(*target);
            else {
                named.type_certain = false;
                named.why = "unknown";
            }
        }
        out.push_back(std::move(named));
    }
    return out;
}

} // namespace mcxx::frontend
