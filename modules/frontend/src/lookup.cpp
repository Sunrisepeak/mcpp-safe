// Name lookup over what F1 reads, and MC3's qualified names: mcxx.frontend:lookup's definitions, and
// :resolver's scopes and lookup.
module mcxx.frontend;

import std;
import mcxx.msa;
import mcxx.base;
import :resolver;

namespace mcxx::frontend {

namespace resolution {

bool scope_kind(msa::Kind k) {
    return k == msa::Kind::namespace_ || k == msa::Kind::class_ || k == msa::Kind::struct_ || k == msa::Kind::union_ || k == msa::Kind::enum_;
}
bool class_kind(msa::Kind k) { return k == msa::Kind::class_ || k == msa::Kind::struct_ || k == msa::Kind::union_; }
// What a name before `::` can be ([basic.lookup.qual]/1): a namespace, a type, a template.
bool names_scope(msa::Kind k) {
    return k == msa::Kind::namespace_ || k == msa::Kind::namespace_alias || class_kind(k) || k == msa::Kind::enum_ || k == msa::Kind::type_alias ||
           k == msa::Kind::template_parameter || k == msa::Kind::concept_;
}
bool function_kind(msa::Kind k) {
    return k == msa::Kind::function || k == msa::Kind::method || k == msa::Kind::constructor || k == msa::Kind::destructor ||
           k == msa::Kind::conversion;
}

// The words that never name a declaration (`module` and `import` do: they are keywords only where a
// module declaration or an import is, which run() tells by the line; `final` and `override` are
// identifiers with a meaning in one place, and names elsewhere).
namespace {
constexpr std::string_view KEYWORDS[] {
    "alignas", "alignof", "and", "and_eq", "asm", "auto", "bitand", "bitor", "bool", "break", "case", "catch", "char", "char8_t", "char16_t",
    "char32_t", "class", "compl", "concept", "const", "consteval", "constexpr", "constinit", "const_cast", "continue", "co_await", "co_return",
    "co_yield", "decltype", "default", "delete", "do", "double", "dynamic_cast", "else", "enum", "explicit", "export", "extern", "false",
    "float", "for", "friend", "goto", "if", "inline", "int", "long", "mutable", "namespace", "new", "noexcept", "not",
    "not_eq", "nullptr", "operator", "or", "or_eq", "private", "protected", "public", "register", "reinterpret_cast", "requires", "return",
    "short", "signed", "sizeof", "static", "static_assert", "static_cast", "struct", "switch", "template", "this", "thread_local", "throw",
    "true", "try", "typedef", "typeid", "typename", "union", "unsigned", "using", "virtual", "void", "volatile", "wchar_t", "while", "xor",
    "xor_eq", "__attribute__", "__declspec", "__extension__", "__int128", "__restrict", "__restrict__", "__typeof__",
    "__asm__", "__asm", "__volatile__", "__inline", "__inline__", "_Alignas", "_Bool", "_Noreturn", "_Thread_local", "__thread",
    "__builtin_va_arg", "va_arg", "__func__", "__FUNCTION__", "__PRETTY_FUNCTION__",
};
} // namespace

bool keyword(std::string_view w) { return std::ranges::contains(KEYWORDS, w); }

std::string_view last_component(std::string_view q) {
    const auto at = q.rfind("::");
    return at == std::string_view::npos ? q : q.substr(at + 2);
}
std::string scope_of(std::string_view q) {
    const auto at = q.rfind("::");
    return at == std::string_view::npos ? std::string {} : std::string { q.substr(0, at) };
}

// A declared type's class name: cv, references, pointers and template arguments off
// ("const ns::S<int> &" -> "ns::S"). "" when it is not one (a builtin, a function type).
std::string class_name_of(std::string type) {
    for (std::string_view prefix : { "const ", "volatile ", "typename ", "struct ", "class ", "union " })
        while (type.starts_with(prefix)) type.erase(0, prefix.size());
    std::size_t depth { 0 }, end { type.size() };
    std::string out;
    for (std::size_t i { 0 }; i < type.size(); ++i) {
        const char c { type[i] };
        if (c == '<') ++depth;
        else if (c == '>') --depth;
        else if (depth == 0 && (c == ' ' || c == '*' || c == '&' || c == '(' || c == '[')) {
            end = i;
            break;
        } else if (depth == 0) out += c;
    }
    (void)end;
    return out;
}

Resolver::Resolver(const Syntax& syntax, const Imported& imported)
    : syntax_ { syntax }, imported_ { imported }, t_ { syntax.pp.tokens }, ds_ { syntax.declarations } {
    // MCXX_LOG=frontend.lookup=debug: how long the scopes take to set up, and the lookup.
    const base::trace::Span span { "frontend.lookup", "scopes", std::format("{} declarations, {} imported", ds_.size(), imported.declarations.size()) };
    const Names names { names_of(syntax) };
    qualified_ = names.qualified;
    members_ = qualified_;
    for (std::size_t j { 0 }; j < ds_.size(); ++j)
        if (const auto p = ds_[j].parent; p >= 0 && class_kind(ds_[static_cast<std::size_t>(p)].kind) && !ds_[j].name.empty())
            members_[static_cast<std::size_t>(p)] = std::string { scope_of(qualified_[j]) };
    // The file's declarations by the scope they are in (what qualified lookup finds), the locals
    // by name (what a block makes visible).
    for (std::size_t i { 0 }; i < ds_.size(); ++i) {
        const auto& d = ds_[i];
        if (d.name.empty() && d.kind != msa::Kind::namespace_) continue;
        Target target { static_cast<std::int32_t>(i), qualified_[i], d.kind, d.name_token, {} };
        if (d.kind == msa::Kind::variable || d.kind == msa::Kind::field || d.kind == msa::Kind::parameter) target.type = type_text(syntax, d);
        else if (d.kind == msa::Kind::function || d.kind == msa::Kind::method) target.type = type_text(syntax, d, true);
        if (function_local(i)) {
            locals_[d.name].push_back(static_cast<std::int32_t>(i));
            // A local class: its members are found in its scope, named through its function.
            if (class_kind(d.kind) && d.definition) {
                scopes_[members_[i]];
                classes_.try_emplace(members_[i], static_cast<std::int32_t>(i));
            }
            continue;
        }
        const std::string_view simple { d.kind == msa::Kind::namespace_ && d.name.empty() ? std::string_view { "(anonymous namespace)" } : std::string_view { d.name } };
        if (d.inline_namespace) {
            // An inline namespace's members are its enclosing namespace's too.
            scopes_[qualified_[i]];
            continue;
        }
        scopes_[scope_of(qualified_[i])][std::string { simple }].push_back(target);
        if (scope_kind(d.kind)) scopes_[qualified_[i]];
        if (d.kind == msa::Kind::namespace_) namespaces_.insert(qualified_[i]);
        if (d.kind == msa::Kind::namespace_ && d.name.empty()) {
            auto& list = transparent_[scope_of(qualified_[i])];
            if (!std::ranges::contains(list, qualified_[i])) list.push_back(qualified_[i]);
        }
        if (class_kind(d.kind) && d.definition) classes_.try_emplace(qualified_[i], static_cast<std::int32_t>(i));
    }
    for (std::size_t index { 0 }; index < imported.declarations.size(); ++index) {
        const auto& d = imported.declarations[index];
        if (d.local || d.kind == msa::Kind::parameter) continue;
        Target target { -1, d.qualified_name, d.kind, 0, d.type, static_cast<std::int32_t>(index) };
        scopes_[scope_of(d.qualified_name)][std::string { last_component(d.qualified_name) }].push_back(target);
        if (scope_kind(d.kind)) scopes_[d.qualified_name];
        if (d.kind == msa::Kind::namespace_) namespaces_.insert(d.qualified_name);
        // A template's parameters (MC3 0.6.0); an alias's templates, canonical as Clang has them.
        if (!d.template_parameters.empty()) imported_parameters_.try_emplace(d.qualified_name, d.template_parameters);
        if (d.kind == msa::Kind::type_alias) imported_templates_.try_emplace(d.qualified_name, d.templates);
        if (d.kind == msa::Kind::type_alias && d.pointer) imported_pointers_.insert(d.qualified_name);
        if (class_kind(d.kind)) {
            imported_classes_.insert(d.qualified_name);
            // Its bases (MC3 0.5.0), where a member it inherits is found.
            auto& bases = imported_bases_[d.qualified_name];
            for (const auto& b : d.bases)
                if (!std::ranges::contains(bases, b)) bases.push_back(b);
        }
        if (d.kind == msa::Kind::namespace_ && last_component(d.qualified_name) == "(anonymous namespace)") {
            auto& list = transparent_[scope_of(d.qualified_name)];
            if (!std::ranges::contains(list, d.qualified_name)) list.push_back(d.qualified_name);
        }
    }
    // What imported declarations are named in is a namespace where nothing declares it otherwise
    // (`std`: libc++'s std module declares no namespace, it only exports what is in one).
    // A scope that holds nothing but enumerators is their enumeration, declared in a unit whose
    // interface was not read: an enumeration, not a namespace.
    std::map<std::string, bool, std::less<>> only_enumerators;
    for (const auto& d : imported.declarations) {
        auto [at, fresh] = only_enumerators.try_emplace(scope_of(d.qualified_name), true);
        if (d.kind != msa::Kind::enumerator) at->second = false;
    }
    for (const auto& d : imported.declarations) {
        for (std::string scope { scope_of(d.qualified_name) }; !scope.empty(); scope = scope_of(scope)) {
            if (namespaces_.contains(scope)) break;
            const auto around = scopes_.find(scope_of(scope));
            const bool declared { around != scopes_.end() && around->second.contains(last_component(scope)) };
            if (declared) break;
            const auto enumerators = only_enumerators.find(scope);
            const bool enumeration { enumerators != only_enumerators.end() && enumerators->second };
            if (!enumeration) namespaces_.insert(scope);
            scopes_[scope_of(scope)][std::string { last_component(scope) }].push_back(
                Target { -1, scope, enumeration ? msa::Kind::enum_ : msa::Kind::namespace_, 0, {} });
        }
    }
    // Each token's innermost enclosing scope: a namespace, a class, a function (parents are
    // recorded before what they hold, so a later one is an inner one).
    enclosing_.assign(t_.size(), -1);
    for (std::size_t i { 0 }; i < ds_.size(); ++i) {
        const auto& d = ds_[i];
        if (!(scope_kind(d.kind) || function_kind(d.kind))) continue;
        for (std::size_t k { d.first_token }; k <= d.last_token && k < t_.size(); ++k) enclosing_[k] = static_cast<std::int32_t>(i);
    }
    // A trailing return type's `->`: after a function's parameters and what may follow them (cv, ref,
    // noexcept, attributes), or a lambda's.
    nominated_.assign(syntax.usings.size(), std::nullopt);
    trailing_.assign(t_.size(), 0);
    for (const auto& d : ds_) {
        if (!function_kind(d.kind) || d.name_token >= t_.size()) continue;
        std::size_t j { d.id_end > d.id_begin ? std::size_t { d.id_end } : std::size_t { d.name_token } + 1 };
        if (!is(j, Kind::l_paren)) continue;
        const auto close { opening_forward(j) };
        if (!close) continue;
        for (j = *close + 1; j < t_.size();) {
            if ((word(j, "noexcept") || word(j, "throw")) && is(j + 1, Kind::l_paren)) {
                const auto end { opening_forward(j + 1) };
                if (!end) break;
                j = *end + 1;
            } else if (is(j, Kind::l_square) && is(j + 1, Kind::l_square)) j = skip_attributes(j);
            else if (word(j, "const") || word(j, "volatile") || word(j, "noexcept") || is(j, Kind::amp) || is(j, Kind::ampamp)) ++j;
            else break;
        }
        if (is(j, Kind::arrow)) trailing_[j] = 1;
    }
    for (std::size_t j { 0 }; j < t_.size(); ++j)
        if (introduces_lambda(j))
            if (const auto lambda = lambda_at(j, t_.size() - 1); lambda && lambda->arrow != 0) trailing_[lambda->arrow] = 1;
    resolved_.assign(t_.size(), std::nullopt);
}

Resolver::Nature Resolver::nature_of(std::string_view written, std::size_t k) {
    Nature n;
    std::string_view rest { written };
    const bool global { rest.starts_with("::") };
    if (global) rest.remove_prefix(2);
    std::optional<Target> found;
    const auto at = rest.rfind("::");
    if (at == std::string_view::npos) {
        found = global ? in_scope(std::string {}, std::string { rest }, t_.size()) : unqualified(std::string { rest }, k);
    } else if (const auto scope { scope_named(std::string { global ? "::" : "" } + std::string { rest.substr(0, at) }, k) }) {
        found = in_scope(*scope, std::string { rest.substr(at + 2) }, t_.size());
    }
    if (!found) return n;
    n.found = true;
    n.kind = found->kind;
    if (found->declaration >= 0) {
        const auto& d = ds_[static_cast<std::size_t>(found->declaration)];
        n.template_ = d.first_token < t_.size() && word(d.first_token, "template") && d.kind != msa::Kind::template_parameter;
        if (d.kind == msa::Kind::template_parameter) {
            std::size_t j { d.name_token };
            if (j >= 1 && t_[j - 1].kind == Kind::ellipsis) --j;
            if (j >= 1 && (word(j - 1, "class") || word(j - 1, "typename"))) n.parameter_form = j >= 2 && t_[j - 2].kind == Kind::greater ? 2 : 0;
            else n.parameter_form = 1;
        }
    } else if (found->imported >= 0) {
        n.template_ = !imported_.declarations[static_cast<std::size_t>(found->imported)].template_parameters.empty();
    }
    return n;
}

std::vector<Reference> Resolver::run() {
    const base::trace::Span span { "frontend.lookup", "names", std::format("{} tokens", t_.size()) };
    std::vector<char> declares(t_.size(), 0);   // a declaration's own name: not a use
    for (const auto& d : ds_) {
        if (d.name_token >= t_.size() || d.name.empty()) continue;
        declares[d.name_token] = 1;
        // An alias template's outline is at its `using`; its name is the next token.
        if (d.kind == msa::Kind::type_alias && word(d.name_token, "using") && d.name_token + 1 < t_.size()) declares[d.name_token + 1] = 1;
    }
    std::vector<Reference> out;
    // A constructor's name, where it is declared, names its class too (Clang's injected-class-name).
    std::vector<std::int32_t> constructor_of(t_.size(), -1);
    for (std::size_t i { 0 }; i < ds_.size(); ++i)
        if (ds_[i].kind == msa::Kind::constructor && ds_[i].name_token < t_.size()) constructor_of[ds_[i].name_token] = static_cast<std::int32_t>(i);
    bool module_line { false };
    for (std::size_t k { 0 }; k < t_.size(); ++k) {
        const auto& t = t_[k];
        // `module m;`, `import m;`: module names, not declarations'.
        if (t.start_of_line) module_line = (word(k, "module") || word(k, "import") || (word(k, "export") && (word(k + 1, "module") || word(k + 1, "import"))));
        if (is(k, Kind::semi)) module_line = false;
        if (t.kind == Kind::l_square && is(k + 1, Kind::l_square)) {   // [[attributes]]
            k = skip_attributes(k) - 1;
            continue;
        }
        if (t.kind == Kind::raw_identifier && declares[k] && constructor_of[k] >= 0 && !t.expanded) {
            const std::string owner { scope_of(qualified_[static_cast<std::size_t>(constructor_of[k])]) };
            Reference r;
            r.range = token_range(syntax_, static_cast<std::uint32_t>(k), static_cast<std::uint32_t>(k));
            r.name = std::string { t.spelling };
            r.target = owner;
            if (const auto c = classes_.find(owner); c != classes_.end()) {
                r.target = qualified_[static_cast<std::size_t>(c->second)];   // a local class's: `Local`, not its members' scope
                r.kind = ds_[static_cast<std::size_t>(c->second)].kind;
                r.declaration = c->second;
                out.push_back(std::move(r));
            } else if (imported_classes_.contains(owner)) {
                // An out-of-line constructor of a class another unit declares (its module's
                // interface): its name is that class's.
                r.kind = imported_kind(owner);
                out.push_back(std::move(r));
            }
            continue;
        }
        if (t.kind != Kind::raw_identifier || t.expanded || module_line || declares[k] || keyword(t.spelling)) continue;
        if (k > 0 && (word(k - 1, "goto") || word(k - 1, "operator"))) continue;
        deduced_placeholder_ = false;
        object_known_ = false;
        auto target { resolve(k) };
        if (!target) {
            // A member F1 cannot find because it cannot type the object: said, not guessed.
            if (k > 0 && (is(k - 1, Kind::period) || (is(k - 1, Kind::arrow) && !trailing_[k - 1]))) {
                Reference r;
                r.range = token_range(syntax_, static_cast<std::uint32_t>(k), static_cast<std::uint32_t>(k));
                r.name = std::string { t.spelling };
                r.certain = false;
                // A type F1 cannot deduce; an object whose class it cannot tell; a class it knows
                // without that member (one its interface does not carry: a specialization's).
                r.why = deduced_placeholder_ ? "deduced" : object_known_ ? "member" : "unknown";
                out.push_back(std::move(r));
            }
            continue;
        }
        resolved_[k] = *target;
        Reference r;
        r.range = token_range(syntax_, static_cast<std::uint32_t>(k), static_cast<std::uint32_t>(k));
        r.name = std::string { t.spelling };
        r.target = target->qualified;
        r.kind = target->kind;
        r.declaration = target->declaration;
        out.push_back(std::move(r));
    }
    return out;
}

std::size_t Resolver::skip_attributes(std::size_t k) const {
    int depth { 0 };
    for (; k < t_.size(); ++k) {
        if (is(k, Kind::l_square)) ++depth;
        else if (is(k, Kind::r_square) && --depth == 0) return k + 1;
    }
    return k;
}

bool Resolver::function_local(std::size_t i) const {
    const auto& d = ds_[i];
    if (d.kind == msa::Kind::parameter || d.kind == msa::Kind::template_parameter || d.binding || d.in_lambda) return true;
    return d.parent >= 0 && !scope_kind(ds_[static_cast<std::size_t>(d.parent)].kind);
}

Target Resolver::target_of(std::int32_t i) const {
    const auto& d = ds_[static_cast<std::size_t>(i)];
    Target target { i, qualified_[static_cast<std::size_t>(i)], d.kind, d.name_token, {} };
    if (d.kind == msa::Kind::variable || d.kind == msa::Kind::field || d.kind == msa::Kind::parameter) target.type = type_text(syntax_, d);
    else if (d.kind == msa::Kind::function || d.kind == msa::Kind::method) target.type = type_text(syntax_, d, true);
    return target;
}

std::optional<Target> Resolver::local(std::size_t k, const std::string& n, bool scope_only) const {
    const auto found = locals_.find(n);
    if (found == locals_.end()) return std::nullopt;
    std::int32_t best { -1 };
    for (const auto i : found->second) {
        const auto& d = ds_[static_cast<std::size_t>(i)];
        if (d.name_token >= k || d.visible_begin > k || (scope_only && !names_scope(d.kind))) continue;
        std::size_t end { d.visible_end };
        if (end == 0) {
            // A function's parameter: its function's end; a local of an owner that is not a
            // function (a lambda's in an initializer): the owner's end.
            end = d.parent >= 0 ? ds_[static_cast<std::size_t>(d.parent)].last_token : t_.size();
        }
        if (k > end) continue;
        if (best < 0 || d.name_token > ds_[static_cast<std::size_t>(best)].name_token) best = i;
    }
    if (best < 0) return std::nullopt;
    return target_of(best);
}

const std::vector<std::string>& Resolver::chain_at(std::size_t k) {
    const std::int32_t e { k < enclosing_.size() ? enclosing_[k] : -1 };
    if (const auto it = chains_.find(e); it != chains_.end()) return it->second;
    std::vector<std::string> chain;
    const auto add = [&](const std::string& scope) {
        if (!std::ranges::contains(chain, scope)) chain.push_back(scope);
    };
    const auto add_with_enclosing = [&](std::string scope) {
        for (;;) {
            add(scope);
            if (scope.empty()) return;
            scope = scope_of(scope);
        }
    };
    for (std::int32_t i { e }; i >= 0; i = ds_[static_cast<std::size_t>(i)].parent) {
        const auto& d = ds_[static_cast<std::size_t>(i)];
        if (scope_kind(d.kind) && d.kind != msa::Kind::enum_) add(members_[static_cast<std::size_t>(i)]);
        // An out-of-line member (`void S::f() {}`): its class's scope and the class's namespaces.
        if (function_kind(d.kind) && !d.qualifier.empty()) add_with_enclosing(scope_of(qualified_[static_cast<std::size_t>(i)]));
    }
    add(std::string {});
    return chains_.emplace(e, std::move(chain)).first->second;
}

std::optional<Target> Resolver::through_using(const Target& using_, int depth, bool scope_only) {
    // One that names what has its own name (libc++'s `std`: `using std::vector;`) adds nothing: what
    // it names is among the reachable declarations already.
    if (using_.type.empty() || using_.type == using_.qualified) return std::nullopt;
    return in_scope(scope_of(using_.type), std::string { last_component(using_.type) }, t_.size(), depth + 1, scope_only);
}

std::optional<Target> Resolver::in_scope(const std::string& scope, const std::string& n, std::size_t k, int depth, bool scope_only) {
    if (depth > 8) return std::nullopt;
    if (const auto s = scopes_.find(scope); s != scopes_.end()) {
        if (const auto m = s->second.find(n); m != s->second.end()) {
            const bool is_class { classes_.contains(scope) || imported_classes_.contains(scope) };
            // A name another unit's using-declaration brings in is what it names (MC3 0.8.0).
            for (const auto& target : m->second)
                if (target.kind == msa::Kind::using_declaration)
                    if (auto named = through_using(target, depth, scope_only)) return named;
            // A constructor or a destructor is not what a name finds: in its class, the class's
            // own name is the class (the injected-class-name).
            for (const auto& target : m->second)
                if (target.kind != msa::Kind::using_declaration && (is_class || target.declaration < 0 || target.declared_at < k) && target.kind != msa::Kind::constructor &&
                    target.kind != msa::Kind::destructor && (!scope_only || names_scope(target.kind))) {
                    // A function of an unnamed namespace in the same scope is an overload of it too.
                    if (function_kind(target.kind))
                        if (const auto t = transparent_.find(scope); t != transparent_.end())
                            for (const auto& unnamed : t->second)
                                if (const auto other = in_scope(unnamed, n, k, depth + 1, scope_only);
                                    other && function_kind(other->kind) && other->qualified != target.qualified) {
                                    overloaded_ = true;
                                    overload_candidates_.push_back(target);
                                    overload_candidates_.push_back(*other);
                                }
                    return target;
                }
        }
    }
    if (const auto t = transparent_.find(scope); t != transparent_.end())
        for (const auto& unnamed : t->second)
            if (auto found = in_scope(unnamed, n, k, depth + 1, scope_only)) return found;
    if (const auto c = classes_.find(scope); c != classes_.end()) {
        for (const auto& base : bases_of(c->second))
            if (auto found = in_scope(base, n, k, depth + 1, scope_only)) return found;
    }
    if (const auto b = imported_bases_.find(scope); b != imported_bases_.end())
        for (const auto& base : b->second)
            if (auto found = in_scope(base, n, k, depth + 1, scope_only)) return found;
    return std::nullopt;
}

const std::vector<std::string>& Resolver::bases_of(std::int32_t c) {
    if (const auto known = bases_.find(c); known != bases_.end()) return known->second;
    auto& out = bases_[c];   // empty while being worked out: a class that names itself among its bases finds none
    const auto& d = ds_[static_cast<std::size_t>(c)];
    if (d.bases_end <= d.bases_begin) return out;
    const std::size_t outside { d.first_token > 0 ? d.first_token - 1 : 0 };
    std::vector<std::string> found;
    std::string name;
    const auto flush = [&] {
        if (!name.empty())
            if (auto scope = scope_named(name, outside)) found.push_back(*scope);
        name.clear();
    };
    for (std::size_t k { d.bases_begin }; k < d.bases_end; ++k) {
        if (is(k, Kind::comma)) {
            flush();
            continue;
        }
        if (word(k, "public") || word(k, "protected") || word(k, "private") || word(k, "virtual")) continue;
        if (is(k, Kind::less)) {   // template arguments: not part of the scope's name
            int depth { 0 };
            for (; k < d.bases_end; ++k) {
                if (is(k, Kind::less)) ++depth;
                else if (is(k, Kind::greater) && --depth == 0) break;
            }
            continue;
        }
        if (is(k, Kind::raw_identifier) || is(k, Kind::coloncolon)) name += t_[k].spelling;
    }
    flush();
    auto& stored = bases_[c];
    stored = std::move(found);
    return stored;
}

std::optional<std::string> Resolver::alias_target(const Target& alias) {
    // An imported one: the namespace MC3 0.8.0's `type` names.
    if (alias.declaration < 0) return alias.type.empty() ? std::nullopt : std::optional<std::string> { alias.type };
    const auto& d = ds_[static_cast<std::size_t>(alias.declaration)];
    std::string written;
    std::size_t k { d.name_token + 1 };
    if (!is(k, Kind::equal)) return std::nullopt;
    for (++k; k <= d.last_token && k < t_.size() && (is(k, Kind::raw_identifier) || is(k, Kind::coloncolon)); ++k) written += t_[k].spelling;
    if (written.empty()) return std::nullopt;
    return scope_named(written, d.name_token);
}

std::optional<std::string> Resolver::scope_named(std::string_view written, std::size_t k) {
    struct Deeper {
        int& depth;
        explicit Deeper(int& d) : depth { d } { ++depth; }
        ~Deeper() { --depth; }
    } deeper { depth_ };
    if (depth_ > 16) return std::nullopt;
    std::string_view rest { written };
    std::string scope;
    bool first { true };
    if (rest.starts_with("::")) {
        rest.remove_prefix(2);
        first = false;
    }
    while (!rest.empty()) {
        const auto at = rest.find("::");
        const std::string part { rest.substr(0, at) };
        std::optional<Target> found;
        if (first) found = unqualified(part, k, true);
        else found = in_scope(scope, part, t_.size(), 0, true);
        if (!found) return std::nullopt;
        auto next { follow(*found) };
        if (!next) return std::nullopt;
        scope = std::move(*next);
        first = false;
        if (at == std::string_view::npos) break;
        rest.remove_prefix(at + 2);
    }
    return scope;
}

std::optional<std::string> Resolver::scope_named_in(std::string_view written, const std::string& context) {
    struct Deeper {
        int& depth;
        explicit Deeper(int& d) : depth { d } { ++depth; }
        ~Deeper() { --depth; }
    } deeper { depth_ };
    if (depth_ > 16) return std::nullopt;
    std::string_view rest { written };
    std::string scope;
    bool first { true };
    if (rest.starts_with("::")) {
        rest.remove_prefix(2);
        first = false;
    }
    while (!rest.empty()) {
        const auto at = rest.find("::");
        const std::string part { rest.substr(0, at) };
        std::optional<Target> found;
        if (first) {
            for (std::string s { context };; s = scope_of(s)) {
                if ((found = in_scope(s, part, t_.size(), 0, true))) break;
                if (s.empty()) break;
            }
        } else {
            found = in_scope(scope, part, t_.size(), 0, true);
        }
        // An inline namespace a printed type names (libc++'s `__1`) is its enclosing one's: MC3's
        // names leave it out.
        if (!found && !first && part.starts_with("__") && at != std::string_view::npos) {
            rest.remove_prefix(at + 2);
            continue;
        }
        if (!found) return std::nullopt;
        auto next { follow(*found) };
        if (!next) return std::nullopt;
        scope = std::move(*next);
        first = false;
        if (at == std::string_view::npos) break;
        rest.remove_prefix(at + 2);
    }
    return scope;
}

std::optional<Target> Resolver::named_in(std::string_view written, const Typed& where) {
    struct Deeper {
        int& depth;
        explicit Deeper(int& d) : depth { d } { ++depth; }
        ~Deeper() { --depth; }
    } deeper { depth_ };
    if (depth_ > 16) return std::nullopt;
    std::string_view rest { written };
    std::optional<Target> found;
    std::string scope;
    bool first { true };
    if (rest.starts_with("::")) {
        rest.remove_prefix(2);
        first = false;
    }
    while (!rest.empty()) {
        const auto at = rest.find("::");
        const std::string part { rest.substr(0, at) };
        if (first && !where.imported) {
            found = unqualified(part, where.at, true);
        } else if (first) {
            found.reset();
            for (std::string s { where.context };; s = scope_of(s)) {
                if ((found = in_scope(s, part, t_.size(), 0, true))) break;
                if (s.empty()) break;
            }
        } else {
            found = in_scope(scope, part, t_.size(), 0, true);
        }
        // An inline namespace a printed type names (libc++'s `__1`) is its enclosing one's.
        if (!found && !first && part.starts_with("__") && at != std::string_view::npos) {
            rest.remove_prefix(at + 2);
            continue;
        }
        if (!found) return std::nullopt;
        first = false;
        if (at == std::string_view::npos) break;
        auto next { follow(*found) };
        if (!next) return std::nullopt;
        scope = std::move(*next);
        rest.remove_prefix(at + 2);
    }
    return found;
}

std::optional<std::string> Resolver::follow(const Target& found) {
    if (found.kind == msa::Kind::namespace_alias) return alias_target(found);
    if (found.kind != msa::Kind::type_alias) return found.declaration >= 0 ? members_[static_cast<std::size_t>(found.declaration)] : found.qualified;
    const std::string written { found.type.empty() && found.declaration >= 0 ? type_text(syntax_, ds_[static_cast<std::size_t>(found.declaration)])
                                                                           : found.type };
    const std::string aliased { class_name_of(written) };
    if (aliased.empty()) return std::nullopt;
    if (found.declaration >= 0) return scope_named(aliased, ds_[static_cast<std::size_t>(found.declaration)].name_token);
    return scope_named_in(aliased, scope_of(found.qualified));
}

msa::Kind Resolver::imported_kind(const std::string& qualified) const {
    if (const auto s = scopes_.find(scope_of(qualified)); s != scopes_.end())
        if (const auto m = s->second.find(std::string { last_component(qualified) }); m != s->second.end())
            for (const auto& target : m->second)
                if (class_kind(target.kind)) return target.kind;
    return msa::Kind::class_;
}

std::optional<std::vector<Member>> Resolver::members_at(msa::Position at) {
    // The token the cursor is in or just after: a name being typed after the operator, or the operator.
    std::optional<std::size_t> last;
    for (std::size_t k { 0 }; k < t_.size(); ++k) {
        if (t_[k].expanded) continue;
        if (at < token_range(syntax_, static_cast<std::uint32_t>(k), static_cast<std::uint32_t>(k)).begin) break;
        last = k;
    }
    if (!last) return std::nullopt;
    std::size_t op { *last };
    const auto end { token_range(syntax_, static_cast<std::uint32_t>(op), static_cast<std::uint32_t>(op)).end };
    if (is(op, Kind::raw_identifier)) {
        if (at > end || op == 0) return std::nullopt;   // after a whole name, not in one being typed
        --op;
    } else if (at < end) {
        return std::nullopt;
    }
    const bool member { is(op, Kind::period) || (is(op, Kind::arrow) && !trailing_[op]) };
    if ((!member && !is(op, Kind::coloncolon)) || op == 0) return std::nullopt;
    std::optional<std::string> scope;
    if (member) {
        auto type { expression_type(op - 1) };
        if (type && is(op, Kind::arrow)) type = pointee(*type);
        if (!type) return std::nullopt;
        scope = class_of(*type);
        if (!scope || (!classes_.contains(*scope) && !scopes_.contains(*scope))) return std::nullopt;
    } else {
        if (!is(op - 1, Kind::raw_identifier) || !resolved_[op - 1]) return std::nullopt;
        scope = follow(*resolved_[op - 1]);
        if (!scope) return std::nullopt;
    }
    // Its members, then its bases' a name it does not declare (what lookup would find).
    std::vector<Member> out;
    std::set<std::string, std::less<>> seen;
    const std::function<void(const std::string&, int)> collect = [&](const std::string& s, int depth) {
        if (depth > 8) return;
        if (const auto m = scopes_.find(s); m != scopes_.end())
            for (const auto& [name, targets] : m->second)
                for (const auto& found : targets) {
                    std::optional<Target> t { found };
                    if (found.kind == msa::Kind::using_declaration) t = through_using(found, depth, false);   // what it names
                    if (!t || t->kind == msa::Kind::constructor || t->kind == msa::Kind::destructor || name.empty()) continue;
                    if (seen.insert(name).second) out.push_back({ name, t->qualified, t->kind, t->type, t->declaration });
                    break;
                }
        if (const auto unnamed = transparent_.find(s); unnamed != transparent_.end())
            for (const auto& u : unnamed->second) collect(u, depth + 1);
        if (const auto c = classes_.find(s); c != classes_.end())
            for (const auto& base : bases_of(c->second)) collect(base, depth + 1);
        if (const auto b = imported_bases_.find(s); b != imported_bases_.end())
            for (const auto& base : b->second) collect(base, depth + 1);
    };
    collect(*scope, 0);
    return out;
}

std::optional<Target> Resolver::unqualified(const std::string& n, std::size_t k, bool scope_only) {
    if (auto l = local(k, n, scope_only)) {
        // In a local class's member function, the class's members hide the locals of the function
        // around the class ([basic.lookup.unqual]: its scope is searched before the enclosing blocks).
        if (l->declaration >= 0) {
            const auto& d = ds_[static_cast<std::size_t>(l->declaration)];
            for (std::int32_t c { k < enclosing_.size() ? enclosing_[k] : -1 }; c >= 0; c = ds_[static_cast<std::size_t>(c)].parent) {
                const auto& around = ds_[static_cast<std::size_t>(c)];
                if (d.name_token >= around.first_token && d.name_token <= around.last_token) break;   // the local is in it
                if (class_kind(around.kind))
                    if (auto member = in_scope(members_[static_cast<std::size_t>(c)], n, k, 0, scope_only)) return member;
            }
        }
        // A namespace alias a block declares again, naming the same namespace as one visible there,
        // redeclares that one: its uses name the first declaration, as Clang's redeclaration chain has it.
        while (l->kind == msa::Kind::namespace_alias && l->declaration >= 0) {
            auto earlier { local(ds_[static_cast<std::size_t>(l->declaration)].name_token, n, true) };
            if (!earlier || earlier->kind != msa::Kind::namespace_alias || earlier->declaration == l->declaration) break;
            const auto named { alias_target(*l) };
            if (!named || alias_target(*earlier) != named) break;
            l = std::move(earlier);
        }
        return l;
    }
    // The using-directives and using-declarations visible at k, each where lookup meets it: what a
    // directive nominates as if declared in the nearest namespace enclosing both the directive and
    // the nominated namespace ([namespace.udir]/2); a declaration in its own scope, a block's before
    // any namespace ([namespace.udecl]).
    struct Acting {
        std::optional<std::string> scope;   // none: a block's
        const Using* u;
        std::string nominated;
    };
    std::vector<Acting> acting;
    for (auto u = syntax_.usings.rbegin(); u != syntax_.usings.rend(); ++u) {
        if (u->at >= k) continue;
        if (u->visible_end != 0 && k > u->visible_end) continue;
        // What an unnamed or inline namespace declares is its enclosing namespace's to name too, to
        // the end of the file: one written in it is met where that namespace's is.
        std::int32_t in { u->parent };
        while (in >= 0 && ds_[static_cast<std::size_t>(in)].kind == msa::Kind::namespace_ &&
               (ds_[static_cast<std::size_t>(in)].name.empty() || ds_[static_cast<std::size_t>(in)].inline_namespace))
            in = ds_[static_cast<std::size_t>(in)].parent;
        if (in >= 0) {
            const auto& p = ds_[static_cast<std::size_t>(in)];
            if (k < p.first_token || k > p.last_token) continue;
        }
        std::string around;   // the namespace it is written in
        for (std::int32_t p { in }; p >= 0; p = ds_[static_cast<std::size_t>(p)].parent)
            if (ds_[static_cast<std::size_t>(p)].kind == msa::Kind::namespace_) {
                around = qualified_[static_cast<std::size_t>(p)];
                break;
            }
        if (u->directive) {
            auto& known = nominated_[static_cast<std::size_t>(&*u - syntax_.usings.data())];
            if (!known) known = nominated(u->name, u->at);
            for (auto scope : *known) {
                // The namespaces both are in: their common leading components.
                std::string common;
                for (std::size_t at { 0 };;) {
                    const auto next { around.find("::", at) };
                    const std::string part { around.substr(0, next) };
                    if (!(scope == part || scope.starts_with(part + "::"))) break;
                    common = part;
                    if (next == std::string::npos) break;
                    at = next + 2;
                }
                acting.push_back({ std::move(common), &*u, std::move(scope) });
            }
        } else if (last_component(u->name) == n) {
            const bool block { in >= 0 && !scope_kind(ds_[static_cast<std::size_t>(in)].kind) };
            acting.push_back({ block ? std::nullopt : std::optional<std::string> { in >= 0 ? members_[static_cast<std::size_t>(in)] : std::string {} }, &*u, {} });
        }
    }
    const auto through = [&](const Acting& a) -> std::optional<Target> {
        if (a.u->directive) return in_scope(a.nominated, n, k, 0, scope_only);
        if (auto scope = scope_named(scope_of(a.u->name), a.u->at)) return in_scope(*scope, n, t_.size(), 0, scope_only);
        return std::nullopt;
    };
    for (const auto& a : acting)
        if (!a.scope)
            if (auto found = through(a)) return found;
    const auto& chain = chain_at(k);
    for (const auto& scope : chain) {
        if (auto found = in_scope(scope, n, k, 0, scope_only)) return found;
        for (const auto& a : acting)
            if (a.scope == scope)
                if (auto found = through(a)) return found;
    }
    for (const auto& a : acting)
        if (a.scope && !std::ranges::contains(chain, *a.scope))
            if (auto found = through(a)) return found;
    return std::nullopt;
}

std::vector<std::string> Resolver::nominated(const std::string& name, std::size_t k) {
    std::vector<std::string> out;
    if (auto scope = scope_named(name, k)) out.push_back(*scope);
    else if (namespaces_.contains(name)) out.push_back(name);
    return out;
}

std::optional<std::string> Resolver::object_class(std::size_t k) {
    object_known_ = false;
    if (k < 2) return std::nullopt;
    auto type { expression_type(k - 2) };
    if (!type) return std::nullopt;
    if (is(k - 1, Kind::arrow)) {
        type = pointee(*type);
        if (!type) return std::nullopt;
    }
    const auto scope { class_of(*type) };
    if (!scope) return std::nullopt;
    // A class of the file, or an imported one: its members are the imports' own.
    if (!classes_.contains(*scope) && !scopes_.contains(*scope)) return std::nullopt;
    object_known_ = true;
    objects_.insert_or_assign(k, *type);
    return scope;
}

std::optional<Target> Resolver::resolve(std::size_t k) {
    overloaded_ = false;
    overload_candidates_.clear();
    auto found { resolve_(k) };
    // libc++'s std re-exports the C library's functions by using-declarations: `std::tolower` is
    // `::tolower` as well as std's own overloads.
    if (found && found->declaration < 0 && function_kind(found->kind) && scope_of(found->qualified) == "std")
        if (const auto global = scopes_.find(std::string {}); global != scopes_.end())
            if (const auto same = global->second.find(std::string { last_component(found->qualified) }); same != global->second.end())
                for (const auto& t : same->second)
                    if (t.declaration < 0 && function_kind(t.kind)) {
                        if (!overloaded_) overload_candidates_.push_back(*found);
                        overloaded_ = true;
                        overload_candidates_.push_back(t);
                    }
    if (!overloaded_) {
        // An unqualified call none of whose functions takes as many arguments: what the compiler calls
        // is another -- one argument-dependent lookup finds ([basic.lookup.argdep]) -- not this.
        if (found && function_kind(found->kind) && is(k + 1, Kind::l_paren) && !(k > 0 && (is(k - 1, Kind::coloncolon) || is(k - 1, Kind::period) || is(k - 1, Kind::arrow))))
            if (const auto close = opening_forward(k + 1); close && !takes(overloads(*found), argument_count(k + 1, *close))) return std::nullopt;
        return found;
    }
    // Functions of more than one scope: a call's arguments may choose among them.
    if (is(k + 1, Kind::l_paren)) {
        std::vector<Target> candidates;
        for (const auto& c : overload_candidates_)
            for (const auto& o : overloads(c))
                if (!std::ranges::any_of(candidates, [&](const Target& x) { return x.qualified == o.qualified && x.declaration == o.declaration && x.imported == o.imported; }))
                    candidates.push_back(o);
        if (auto chosen = chosen_by_arguments(candidates, k)) return chosen;
    }
    return std::nullopt;
}

std::optional<Target> Resolver::resolve_(std::size_t k) {
    const std::string n { t_[k].spelling };
    // A designated initializer: `T { .n = ... }` names T's member; `return { .n = ... }`, its
    // function's return type's.
    if (k > 1 && is(k - 1, Kind::period) && (is(k - 2, Kind::l_brace) || is(k - 2, Kind::comma)) && (is(k + 1, Kind::equal) || is(k + 1, Kind::l_brace))) {
        std::size_t open { k - 2 };
        for (int depth { 0 }; open > 0; --open) {
            if (is(open, Kind::r_brace) || is(open, Kind::r_paren)) ++depth;
            else if ((is(open, Kind::l_brace) || is(open, Kind::l_paren)) && depth-- == 0) break;
        }
        if (is(open, Kind::l_brace)) {
            auto type { type_named_before(open) };
            if (!type && open > 0 && (word(open - 1, "return") || word(open - 1, "co_return"))) type = returned_at(open - 1);
            if (!type && open > 0 && (is(open - 1, Kind::l_paren) || is(open - 1, Kind::comma))) type = argument_type(open);
            if (type)
                if (auto scope = class_of(*type)) return in_scope(*scope, n, t_.size());
        }
        return std::nullopt;
    }
    // A member access: `x.n`, `p->n` (not a trailing return type's `-> T`).
    if (k > 0 && (is(k - 1, Kind::period) || (is(k - 1, Kind::arrow) && !trailing_[k - 1]))) {
        const auto scope { object_class(k) };
        if (!scope) return std::nullopt;
        return in_scope(*scope, n, t_.size());
    }
    // A qualified name: `S::n`, `::n`.
    if (k > 0 && is(k - 1, Kind::coloncolon)) {
        if (k < 2 || !(is(k - 2, Kind::raw_identifier))) {
            if (k >= 2 && is(k - 2, Kind::greater)) return std::nullopt;   // `T<U>::n`: not followed
            return in_scope(std::string {}, n, t_.size(), 0, is(k + 1, Kind::coloncolon));   // `::n`
        }
        const auto& q = resolved_[k - 2];
        if (!q) return std::nullopt;
        const auto scope { follow(*q) };
        if (!scope) return std::nullopt;
        return in_scope(*scope, n, t_.size(), 0, is(k + 1, Kind::coloncolon));
    }
    // A name before `::` names a namespace, a type or a template ([basic.lookup.qual]/1); one after
    // `struct`, `class`, `union` or `enum` a type ([basic.lookup.elab]: `struct stat st;` beside a
    // function or a variable named `stat`).
    const bool elaborated { k > 0 && (word(k - 1, "struct") || word(k - 1, "class") || word(k - 1, "union") || word(k - 1, "enum")) };
    return unqualified(n, k, is(k + 1, Kind::coloncolon) || elaborated);
}

} // namespace resolution

using resolution::scope_kind;
using resolution::function_kind;
using resolution::class_kind;

Names names_of(const Syntax& syntax) {
    const auto& ds = syntax.declarations;
    Names out;
    out.container.resize(ds.size());
    out.qualified.resize(ds.size());
    std::vector<std::string> prefix_of(ds.size());   // the scope each is named in
    const auto name_of = [](const Declaration& d) {
        if (!d.name.empty()) return d.name;
        // Unnamed, as Clang's plain style names them (MC3-4-4).
        switch (d.kind) {
        case msa::Kind::namespace_: return std::string { "(anonymous namespace)" };
        case msa::Kind::struct_: return std::string { "(unnamed struct)" };
        case msa::Kind::class_: return std::string { "(unnamed class)" };
        case msa::Kind::union_: return std::string { "(anonymous union)" };
        case msa::Kind::enum_: return std::string { "(unnamed enum)" };
        default: return d.name;
        }
    };
    for (std::size_t i { 0 }; i < ds.size(); ++i) {
        const auto& d = ds[i];
        std::string prefix;
        std::string ns;
        if (d.parent >= 0) {
            const auto p = static_cast<std::size_t>(d.parent);
            const auto& parent = ds[p];
            if (parent.kind == msa::Kind::namespace_) {
                ns = parent.inline_namespace ? out.container[p] : out.qualified[p];
                prefix = ns;
            } else {
                ns = out.container[p];
                // An unscoped enumerator is named in its enum's scope, as Clang names it; a local
                // enumeration's in its function's (`f(int)::red`), as a local class's members are.
                if (parent.kind == msa::Kind::enum_ && parent.parent >= 0 && function_kind(ds[static_cast<std::size_t>(parent.parent)].kind)) {
                    const auto f = static_cast<std::size_t>(parent.parent);
                    std::string params;
                    bool known { true };
                    for (std::size_t j { f + 1 }; j < ds.size() && j < p; ++j) {
                        if (ds[j].parent != static_cast<std::int32_t>(f) || ds[j].kind != msa::Kind::parameter || ds[j].in_lambda) continue;
                        const std::string type { type_text(syntax, ds[j]) };
                        if (type.empty()) known = false;
                        params += (params.empty() ? "" : ", ") + type;
                    }
                    if (ds[f].c_variadic) params += params.empty() ? "..." : ", ...";
                    if (known) prefix = std::format("{}({})", out.qualified[f], params) + (parent.scoped_enum ? "::" + name_of(parent) : std::string {});
                    else prefix = parent.scoped_enum ? out.qualified[p] : prefix_of[p];
                } else if (parent.kind == msa::Kind::enum_ && !parent.scoped_enum) prefix = prefix_of[p];
                else if (scope_kind(parent.kind)) {
                    prefix = out.qualified[p];
                    // What a local class or enum declares is named through its function, as Clang
                    // prints a function context: its name and its parameters' types
                    // (`ns::f(int, const S &)::Local::m`).
                    if (parent.parent >= 0 && function_kind(ds[static_cast<std::size_t>(parent.parent)].kind) && prefix.find("::") == std::string::npos) {
                        const auto f = static_cast<std::size_t>(parent.parent);
                        std::string params;
                        bool known { true };
                        for (std::size_t j { f + 1 }; j < ds.size() && j < p; ++j) {
                            if (ds[j].parent != static_cast<std::int32_t>(f) || ds[j].kind != msa::Kind::parameter || ds[j].in_lambda) continue;
                            const std::string type { type_text(syntax, ds[j]) };
                            if (type.empty()) known = false;
                            params += (params.empty() ? "" : ", ") + type;
                        }
                        if (ds[f].c_variadic) params += params.empty() ? "..." : ", ...";
                        if (known) prefix = std::format("{}({})::{}", out.qualified[f], params, prefix);
                    }
                }
            }
        }
        out.container[i] = ns;
        prefix_of[i] = prefix;
        const std::string own { d.qualifier + name_of(d) };
        out.qualified[i] = d.inline_namespace ? prefix : prefix.empty() ? own : prefix + "::" + own;
    }
    // A template parameter is named in its template's scope, as Clang names it: a class template's in
    // the class (`ns::S::T`), a function template's by its name alone, an alias or variable
    // template's in its namespace. Its template is the declaration after it that is not another one.
    for (std::size_t i { 0 }; i < ds.size(); ++i) {
        if (ds[i].kind != msa::Kind::template_parameter) continue;
        std::size_t templated { i + 1 };
        while (templated < ds.size() && ds[templated].kind == msa::Kind::template_parameter) ++templated;
        if (templated >= ds.size() || ds[templated].parent != ds[i].parent) continue;
        const auto& t = ds[templated];
        if (function_kind(t.kind)) out.qualified[i] = ds[i].name;
        else if (class_kind(t.kind)) out.qualified[i] = out.qualified[templated] + "::" + ds[i].name;
    }
    return out;
}

std::vector<Reference> references(const Syntax& syntax, const Imported& imported) {
    resolution::Resolver resolver { syntax, imported };
    return resolver.run();
}

std::optional<std::vector<Member>> members_at(const Syntax& syntax, const Imported& imported, msa::Position at) {
    const base::trace::Span span { "frontend.lookup", "members", std::format("at {}:{}", at.line, at.column) };
    resolution::Resolver resolver { syntax, imported };
    (void)resolver.run();
    return resolver.members_at(at);
}

} // namespace mcxx::frontend
