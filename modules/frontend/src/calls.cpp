// What a call gives (mcxx.frontend:lookup): :resolver's overloads chosen by a call's arguments. When the
// overloads of a called function return different types, which one is called is what C++'s overload
// resolution decides from the arguments' types; F1 decides only what is plain -- the number of
// arguments, a literal's kind, an argument whose class F1 knows -- and only when exactly one overload
// fits. A function template whose return type is one of its parameters' types returns the argument's
// type (`json.value("k", Json::object())`). Anything else is not typed, and is said uncertain where a
// member of it is named.
module mcxx.frontend;

import std;
import mcxx.msa;
import :resolver;

namespace mcxx::frontend::resolution {

namespace {

// A parameter's type without the ` =` that says it has a default argument.
std::string without_default(std::string parameter) {
    if (parameter.ends_with(" =")) parameter.resize(parameter.size() - 2);
    return parameter;
}

bool arithmetic_word(std::string_view w) {
    static constexpr std::string_view words[] { "bool", "char", "signed", "unsigned", "short", "int", "long", "float", "double",
                                                "wchar_t", "char8_t", "char16_t", "char32_t", "__int128" };
    return std::ranges::contains(words, w);
}

} // namespace

std::vector<Target> Resolver::overloads(const Target& callee) const {
    std::vector<Target> out;
    if (const auto s = scopes_.find(scope_of(callee.qualified)); s != scopes_.end())
        if (const auto m = s->second.find(last_component(callee.qualified)); m != s->second.end())
            for (const auto& t : m->second)
                if (t.kind == callee.kind) out.push_back(t);
    if (out.empty()) out.push_back(callee);
    return out;
}

std::optional<std::vector<std::string>> Resolver::function_parameters(const Target& f) const {
    if (f.declaration < 0) {
        if (f.imported < 0 || static_cast<std::size_t>(f.imported) >= imported_.declarations.size()) return std::nullopt;
        return imported_.declarations[static_cast<std::size_t>(f.imported)].parameters;
    }
    std::vector<std::string> out;
    for (std::size_t j { static_cast<std::size_t>(f.declaration) + 1 }; j < ds_.size(); ++j) {
        const auto& p = ds_[j];
        if (p.parent != f.declaration || p.kind != msa::Kind::parameter || p.in_lambda) continue;
        std::string type { type_text(syntax_, p) };
        if (type.empty()) return std::nullopt;
        // A default argument: an `=` after its declarator, before the `,` or `)` that ends it.
        std::size_t k { std::max<std::size_t>(p.declarator_end, p.id_end) };
        for (int nesting { 0 }; k < t_.size(); ++k) {
            if (nesting == 0 && (is(k, Kind::comma) || is(k, Kind::r_paren))) break;
            if (is(k, Kind::l_paren) || is(k, Kind::l_square) || is(k, Kind::l_brace)) ++nesting;
            else if (is(k, Kind::r_paren) || is(k, Kind::r_square) || is(k, Kind::r_brace)) --nesting;
            else if (nesting == 0 && is(k, Kind::equal)) {
                type += " =";
                break;
            }
        }
        out.push_back(std::move(type));
    }
    return out;
}

std::vector<Resolver::Argument> Resolver::arguments(std::size_t open, std::size_t close, int depth) {
    std::vector<Argument> out;
    if (close <= open + 1) return out;
    std::size_t begin { open + 1 };
    int nesting { 0 }, angle { 0 };
    for (std::size_t k { open + 1 }; k <= close; ++k) {
        const bool end { k == close || (nesting == 0 && angle == 0 && is(k, Kind::comma)) };
        if (!end) {
            if (is(k, Kind::l_paren) || is(k, Kind::l_square) || is(k, Kind::l_brace)) ++nesting;
            else if (is(k, Kind::r_paren) || is(k, Kind::r_square) || is(k, Kind::r_brace)) --nesting;
            // Template arguments: a `<` after a name that is not an object's.
            else if (nesting == 0 && is(k, Kind::less) && k > 0 && is(k - 1, Kind::raw_identifier) &&
                     !(resolved_[k - 1] && (resolved_[k - 1]->kind == msa::Kind::variable || resolved_[k - 1]->kind == msa::Kind::field ||
                                            resolved_[k - 1]->kind == msa::Kind::parameter)))
                ++angle;
            else if (nesting == 0 && angle > 0 && is(k, Kind::greater)) --angle;
            else if (nesting == 0 && angle > 0 && is(k, Kind::greatergreater)) angle = std::max(0, angle - 2);
            continue;
        }
        Argument a;
        const std::size_t last { k - 1 };
        if (last >= begin) {
            const auto& first = t_[begin];
            if (begin == last) {
                const Kind kind { first.kind };
                if (kind == Kind::string_literal || kind == Kind::utf8_string_literal) a.sort = Argument::Sort::string_literal;
                else if (kind == Kind::numeric_constant) {
                    const std::string_view s { first.spelling };
                    const bool hex { s.starts_with("0x") || s.starts_with("0X") };
                    a.sort = !hex && (s.find('.') != std::string_view::npos || s.find('e') != std::string_view::npos || s.find('E') != std::string_view::npos)
                                 ? Argument::Sort::floating
                                 : Argument::Sort::integer;
                } else if (kind == Kind::char_constant) a.sort = Argument::Sort::integer;
                else if (word(begin, "true") || word(begin, "false")) a.sort = Argument::Sort::boolean;
                else if (word(begin, "nullptr")) a.sort = Argument::Sort::null_pointer;
            }
            if (a.sort == Argument::Sort::unknown)
                if (auto type = initializer_type(begin, last, depth + 1)) {
                    a.sort = Argument::Sort::typed;
                    a.type = std::move(type);
                }
        }
        out.push_back(std::move(a));
        begin = k + 1;
    }
    return out;
}

bool Resolver::accepts(const std::string& parameter, const Typed& where, const Argument& argument, const std::vector<Parameter>& templates) {
    if (argument.sort == Argument::Sort::unknown) return true;
    const std::string text { bare(without_default(parameter)) };
    if (text.find("...") != std::string::npos) return true;
    // A function template's own parameter takes anything its deduction does.
    const std::string name { class_name_of(text) };
    if (std::ranges::any_of(templates, [&](const Parameter& p) { return !p.name.empty() && p.name == name; })) return true;
    const bool pointer { text.ends_with('*') };
    const Typed param { unbound(Typed { text, where.at, where.context, where.imported, {}, where.bindings }) };
    const std::string resolved { bare(param.text) };
    const bool builtin { !pointer && !resolved.ends_with('*') && arithmetic_word(class_name_of(resolved)) };
    // The class (or enumeration) the parameter names; none for a builtin or a pointer. A parameter
    // F1 cannot resolve at all is unknown: it may take anything.
    const std::optional<std::string> cls { pointer || builtin || resolved.ends_with('*') ? std::nullopt : class_of(param) };
    const bool unknown { !pointer && !builtin && !resolved.ends_with('*') && !cls };
    const auto string_like = [](const std::optional<std::string>& c) {
        return c && (*c == "std::basic_string" || *c == "std::basic_string_view" || *c == "std::filesystem::path");
    };
    switch (argument.sort) {
    case Argument::Sort::string_literal:
        // `const char *`, a string class, or bool (a pointer converts to it).
        return unknown || (pointer && text.find("char") != std::string::npos) || resolved == "bool" || string_like(cls);
    case Argument::Sort::integer:
    case Argument::Sort::floating:
    case Argument::Sort::boolean:
        // Not a pointer, not an enumeration or a class (one may be made from a number: F1 does not know).
        return builtin || unknown;
    case Argument::Sort::null_pointer: return pointer || resolved.ends_with('*') || resolved.find("nullptr_t") != std::string::npos || !builtin;
    case Argument::Sort::typed: {
        const Typed arg { unbound(*argument.type) };
        const std::string arg_text { bare(arg.text) };
        if (arg_text.ends_with('*')) return pointer || resolved.ends_with('*') || resolved == "bool";
        const auto arg_class { class_of(arg) };
        if (!arg_class) return true;   // a builtin, or a type F1 cannot tell
        if (builtin || pointer || resolved.ends_with('*')) return false;
        if (!cls) return true;
        if (*cls == *arg_class || (string_like(cls) && string_like(arg_class))) return true;
        // A base of the argument's class.
        std::vector<std::string> todo { *arg_class };
        for (std::size_t i { 0 }; i < todo.size() && i < 32; ++i) {
            if (todo[i] == *cls) return true;
            if (const auto b = imported_bases_.find(todo[i]); b != imported_bases_.end()) todo.insert(todo.end(), b->second.begin(), b->second.end());
            if (const auto c = classes_.find(todo[i]); c != classes_.end())
                for (const auto& base : bases_of(c->second)) todo.push_back(base);
        }
        return false;
    }
    case Argument::Sort::unknown: break;
    }
    return true;
}

std::optional<Target> Resolver::chosen_by_arguments(const std::vector<Target>& candidates, std::size_t name) {
    const auto close { opening_forward(name + 1) };
    if (!close) return std::nullopt;
    // Its arguments' names come after it: resolved here first, the state of the name being resolved
    // kept aside.
    {
        const bool overloaded { overloaded_ }, placeholder { deduced_placeholder_ }, known { object_known_ };
        auto candidates { std::move(overload_candidates_) };
        for (std::size_t j { name + 2 }; j < *close; ++j) {
            if (resolved_[j] || !is(j, Kind::raw_identifier) || t_[j].expanded || keyword(t_[j].spelling)) continue;
            if (j > 0 && (word(j - 1, "goto") || word(j - 1, "operator"))) continue;
            if (auto target = resolve(j)) resolved_[j] = std::move(*target);
        }
        overloaded_ = overloaded;
        deduced_placeholder_ = placeholder;
        object_known_ = known;
        overload_candidates_ = std::move(candidates);
    }
    const auto args { arguments(name + 1, *close, 0) };
    std::optional<Target> chosen;
    for (const auto& candidate : candidates) {
        const auto params { function_parameters(candidate) };
        if (!params) return std::nullopt;   // one whose parameters are not known: no choice
        std::vector<Parameter> templates;
        if (candidate.declaration >= 0) templates = parameters_of(candidate);
        else if (candidate.imported >= 0)
            for (const auto& written : imported_.declarations[static_cast<std::size_t>(candidate.imported)].template_parameters) templates.push_back(parameter_of(written));
        const bool variadic { std::ranges::any_of(*params, [](const std::string& p) { return p.find("...") != std::string::npos; }) ||
                              (candidate.imported >= 0 && imported_.declarations[static_cast<std::size_t>(candidate.imported)].c_variadic) ||
                              (candidate.declaration >= 0 && ds_[static_cast<std::size_t>(candidate.declaration)].c_variadic) };
        const auto required = static_cast<std::size_t>(std::ranges::count_if(*params, [](const std::string& p) { return !p.ends_with(" ="); }));
        if (args.size() < required || (args.size() > params->size() && !variadic)) continue;
        const Typed where { {}, candidate.declaration >= 0 ? ds_[static_cast<std::size_t>(candidate.declaration)].name_token : 0,
                            candidate.declaration >= 0 ? std::string {} : scope_of(candidate.qualified), candidate.declaration < 0, {}, nullptr };
        bool fits { true };
        for (std::size_t i { 0 }; i < args.size() && i < params->size() && fits; ++i) fits = accepts((*params)[i], where, args[i], templates);
        if (!fits) continue;
        // Declarations of one function (a declaration and its definition) are one choice.
        if (chosen && chosen->qualified != candidate.qualified) return std::nullopt;
        if (!chosen) chosen = candidate;
    }
    return chosen;
}

std::optional<Typed> Resolver::call_typed(const Target& callee, std::size_t open, std::size_t close, const Typed* object, int depth) {
    const auto plain_call = [&](const Target& f, bool chosen) {
        return object != nullptr ? member_typed(f, *object, depth, chosen) : typed(f, depth, chosen);
    };
    if (callee.kind != msa::Kind::function && callee.kind != msa::Kind::method) return plain_call(callee, false);
    const auto set { overloads(callee) };
    const auto plain = [](std::string type) {
        type = bare(std::move(type));
        for (std::size_t at { type.find("const_") }; at != std::string::npos; at = type.find("const_")) type.erase(at, 6);
        return type;
    };
    const bool agree { std::ranges::all_of(set, [&](const Target& t) { return plain(t.type) == plain(set.front().type); }) };
    // A function template's own parameters, its interface's or its template-head's.
    const auto templates_of = [&](const Target& f) {
        if (f.declaration >= 0) return parameters_of(f);
        std::vector<Parameter> out;
        if (f.imported >= 0)
            for (const auto& written : imported_.declarations[static_cast<std::size_t>(f.imported)].template_parameters) out.push_back(parameter_of(written));
        return out;
    };
    const Target* chosen { &callee };
    std::vector<Argument> args;
    bool have_args { false };
    if (!agree) {
        args = arguments(open, close, depth);
        have_args = true;
        std::vector<const Target*> viable;
        for (const auto& candidate : set) {
            const auto params { function_parameters(candidate) };
            if (!params) return std::nullopt;   // an overload whose parameters are not known: no choice
            const auto templates { templates_of(candidate) };
            const bool variadic { std::ranges::any_of(*params, [](const std::string& p) { return p.find("...") != std::string::npos; }) ||
                                  (candidate.imported >= 0 && imported_.declarations[static_cast<std::size_t>(candidate.imported)].c_variadic) ||
                                  (candidate.declaration >= 0 && ds_[static_cast<std::size_t>(candidate.declaration)].c_variadic) };
            const auto required = static_cast<std::size_t>(std::ranges::count_if(*params, [](const std::string& p) { return !p.ends_with(" ="); }));
            if (args.size() < required || (args.size() > params->size() && !variadic)) continue;
            // Its parameters are read where it is declared, with its class's arguments (a member of a
            // specialization: `const string_t &`).
            const Typed where { {}, candidate.declaration >= 0 ? ds_[static_cast<std::size_t>(candidate.declaration)].name_token : 0,
                                candidate.declaration >= 0 ? std::string {} : scope_of(candidate.qualified), candidate.declaration < 0, {},
                                object != nullptr ? bindings_for(*object, scope_of(candidate.qualified)) : nullptr };
            bool fits { true };
            for (std::size_t i { 0 }; i < args.size() && i < params->size() && fits; ++i) fits = accepts((*params)[i], where, args[i], templates);
            if (fits && !std::ranges::any_of(viable, [&](const Target* v) { return plain(v->type) == plain(candidate.type); })) viable.push_back(&candidate);
        }
        if (viable.size() != 1) return std::nullopt;
        chosen = viable.front();
    }
    auto result { plain_call(*chosen, true) };
    // A function template's return type written as one of its parameters: the argument's type.
    const auto templates { templates_of(*chosen) };
    const std::string returned { bare(chosen->type) };
    const auto names_parameter = [&](std::string_view text) {
        return std::ranges::any_of(templates, [&](const Parameter& p) {
            if (p.name.empty()) return false;
            for (std::size_t at { text.find(p.name) }; at != std::string_view::npos; at = text.find(p.name, at + 1)) {
                const std::size_t end { at + p.name.size() };
                const bool starts { at == 0 || !(std::isalnum(static_cast<unsigned char>(text[at - 1])) || text[at - 1] == '_') };
                const bool ends { end >= text.size() || !(std::isalnum(static_cast<unsigned char>(text[end])) || text[end] == '_') };
                if (starts && ends) return true;
            }
            return false;
        });
    };
    if (!names_parameter(returned)) return result;
    const bool exact { std::ranges::any_of(templates, [&](const Parameter& p) { return !p.name.empty() && p.name == returned; }) };
    // std::move and std::forward give their argument (libc++ writes it remove_reference_t<_Tp>&&).
    const bool moves { chosen->qualified == "std::move" || chosen->qualified == "std::forward" };
    if (!exact && !moves) {
        deduced_placeholder_ = true;   // written with its parameters otherwise: what they are is deduction's
        return std::nullopt;
    }
    if (moves) {
        if (!have_args) args = arguments(open, close, depth);
        if (args.size() == 1 && args[0].sort == Argument::Sort::typed && args[0].type) {
            Typed moved { *args[0].type };
            moved.text = bare(moved.text);
            moved.through_template = true;
            return moved;
        }
        deduced_placeholder_ = true;
        return std::nullopt;
    }
    const auto params { function_parameters(*chosen) };
    if (!params) return std::nullopt;
    if (!have_args) args = arguments(open, close, depth);
    for (std::size_t i { 0 }; i < params->size() && i < args.size(); ++i)
        if (bare(without_default((*params)[i])) == returned && args[i].sort == Argument::Sort::typed && args[i].type) {
            Typed deduced { *args[i].type };
            deduced.text = bare(deduced.text);
            return deduced;
        }
    deduced_placeholder_ = true;   // deduced from something F1 does not type
    return std::nullopt;
}

} // namespace mcxx::frontend::resolution
