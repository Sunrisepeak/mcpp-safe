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
    : syntax_ { syntax }, t_ { syntax.pp.tokens }, ds_ { syntax.declarations } {
    // MCXX_LOG=frontend.lookup=debug: how long the scopes take to set up, and the lookup.
    const base::trace::Span span { "frontend.lookup", "scopes", std::format("{} declarations, {} imported", ds_.size(), imported.declarations.size()) };
    const Names names { names_of(syntax) };
    qualified_ = names.qualified;
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
    for (const auto& d : imported.declarations) {
        if (d.local || d.kind == msa::Kind::parameter) continue;
        Target target { -1, d.qualified_name, d.kind, 0, d.type };
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
    resolved_.assign(t_.size(), std::nullopt);
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
            if (k > 0 && (is(k - 1, Kind::period) || is(k - 1, Kind::arrow))) {
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
        if (scope_kind(d.kind) && d.kind != msa::Kind::enum_) add(qualified_[static_cast<std::size_t>(i)]);
        // An out-of-line member (`void S::f() {}`): its class's scope and the class's namespaces.
        if (function_kind(d.kind) && !d.qualifier.empty()) add_with_enclosing(scope_of(qualified_[static_cast<std::size_t>(i)]));
    }
    add(std::string {});
    return chains_.emplace(e, std::move(chain)).first->second;
}

std::optional<Target> Resolver::in_scope(const std::string& scope, const std::string& n, std::size_t k, int depth, bool scope_only) {
    if (depth > 8) return std::nullopt;
    if (const auto s = scopes_.find(scope); s != scopes_.end()) {
        if (const auto m = s->second.find(n); m != s->second.end()) {
            const bool is_class { classes_.contains(scope) || imported_classes_.contains(scope) };
            // A constructor or a destructor is not what a name finds: in its class, the class's
            // own name is the class (the injected-class-name).
            for (const auto& target : m->second)
                if ((is_class || target.declaration < 0 || target.declared_at < k) && target.kind != msa::Kind::constructor &&
                    target.kind != msa::Kind::destructor && (!scope_only || names_scope(target.kind))) {
                    // A function of an unnamed namespace in the same scope is an overload of it too.
                    if (function_kind(target.kind))
                        if (const auto t = transparent_.find(scope); t != transparent_.end())
                            for (const auto& unnamed : t->second)
                                if (const auto other = in_scope(unnamed, n, k, depth + 1, scope_only);
                                    other && function_kind(other->kind) && other->qualified != target.qualified)
                                    overloaded_ = true;
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
    if (alias.declaration < 0) return std::nullopt;
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
    if (found.kind != msa::Kind::type_alias) return found.qualified;
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

std::optional<Target> Resolver::unqualified(const std::string& n, std::size_t k, bool scope_only) {
    if (auto l = local(k, n, scope_only)) return l;
    for (const auto& scope : chain_at(k))
        if (auto found = in_scope(scope, n, k, 0, scope_only)) return found;
    for (auto u = syntax_.usings.rbegin(); u != syntax_.usings.rend(); ++u) {
        if (u->at >= k) continue;
        if (u->visible_end != 0 && k > u->visible_end) continue;
        if (u->parent >= 0) {
            const auto& p = ds_[static_cast<std::size_t>(u->parent)];
            if (k < p.first_token || k > p.last_token) continue;
        }
        if (u->directive) {
            for (const auto& scope : nominated(u->name, u->at))
                if (auto found = in_scope(scope, n, k, 0, scope_only)) return found;
        } else if (last_component(u->name) == n) {
            if (auto scope = scope_named(scope_of(u->name), u->at))
                if (auto found = in_scope(*scope, n, t_.size(), 0, scope_only)) return found;
        }
    }
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
    auto found { resolve_(k) };
    // libc++'s std re-exports the C library's functions by using-declarations: `std::tolower` is
    // `::tolower` as well as std's own overloads.
    if (found && found->declaration < 0 && function_kind(found->kind) && scope_of(found->qualified) == "std")
        if (const auto global = scopes_.find(std::string {}); global != scopes_.end())
            if (const auto same = global->second.find(std::string { last_component(found->qualified) }); same != global->second.end())
                overloaded_ = overloaded_ || std::ranges::any_of(same->second, [](const Target& t) { return t.declaration < 0 && function_kind(t.kind); });
    if (overloaded_) return std::nullopt;
    return found;
}

std::optional<Target> Resolver::resolve_(std::size_t k) {
    const std::string n { t_[k].spelling };
    // A designated initializer: `T { .n = ... }` names T's member.
    if (k > 1 && is(k - 1, Kind::period) && (is(k - 2, Kind::l_brace) || is(k - 2, Kind::comma)) && (is(k + 1, Kind::equal) || is(k + 1, Kind::l_brace))) {
        std::size_t open { k - 2 };
        for (int depth { 0 }; open > 0; --open) {
            if (is(open, Kind::r_brace) || is(open, Kind::r_paren)) ++depth;
            else if ((is(open, Kind::l_brace) || is(open, Kind::l_paren)) && depth-- == 0) break;
        }
        if (is(open, Kind::l_brace)) {
            if (auto type = type_named_before(open))
                if (auto scope = class_of(*type)) return in_scope(*scope, n, t_.size());
        }
        return std::nullopt;
    }
    // A member access: `x.n`, `p->n`.
    if (k > 0 && (is(k - 1, Kind::period) || is(k - 1, Kind::arrow))) {
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
    // A name before `::` names a namespace, a type or a template ([basic.lookup.qual]/1).
    return unqualified(n, k, is(k + 1, Kind::coloncolon));
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
    const auto name_of = [](const Declaration& d) { return d.name.empty() && d.kind == msa::Kind::namespace_ ? std::string { "(anonymous namespace)" } : d.name; };
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
                // An unscoped enumerator is named in its enum's scope, as Clang names it.
                if (parent.kind == msa::Kind::enum_ && !parent.scoped_enum) prefix = prefix_of[p];
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
                            if (ds[j].parent != static_cast<std::int32_t>(f) || ds[j].kind != msa::Kind::parameter) continue;
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

} // namespace mcxx::frontend
