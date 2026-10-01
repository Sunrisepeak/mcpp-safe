// Types: decl-specifiers, declarators (read inside out, as C++ derives a type), type-ids, parameter lists,
// and the one place the grammar gives a name two readings in an argument: type or expression.
module mcxx.frontend;

import std;
import mcxx.msa;
import mcxx.base;
import :bodyparser;

namespace mcxx::frontend::bodies {

namespace {

// What the type keywords of a simple-type-specifier add up to.
struct Builtins {
    int signed_ { 0 }, unsigned_ { 0 }, short_ { 0 }, long_ { 0 }, char_ { 0 }, int_ { 0 }, bool_ { 0 }, float_ { 0 }, double_ { 0 }, void_ { 0 },
        wchar { 0 }, char8 { 0 }, char16 { 0 }, char32 { 0 }, int128 { 0 }, other { 0 }, complex { 0 };
    std::uint32_t first { NONE }, last { 0 };

    bool any() const { return first != NONE; }

    bool add(std::string_view w, std::uint32_t at) {
        int* slot { nullptr };
        if (w == "signed" || w == "__signed" || w == "__signed__") slot = &signed_;
        else if (w == "unsigned" || w == "__unsigned") slot = &unsigned_;
        else if (w == "short") slot = &short_;
        else if (w == "long") slot = &long_;
        else if (w == "char") slot = &char_;
        else if (w == "int") slot = &int_;
        else if (w == "bool" || w == "_Bool") slot = &bool_;
        else if (w == "float") slot = &float_;
        else if (w == "double") slot = &double_;
        else if (w == "void") slot = &void_;
        else if (w == "wchar_t") slot = &wchar;
        else if (w == "char8_t") slot = &char8;
        else if (w == "char16_t") slot = &char16;
        else if (w == "char32_t") slot = &char32;
        else if (w == "__int128" || w == "__int128_t" || w == "__uint128_t") {
            slot = &int128;
            if (w == "__uint128_t") ++unsigned_;
        } else if (w == "_Complex") slot = &complex;
        else if (w == "_Float16" || w == "__fp16" || w == "__bf16" || w == "_Float32" || w == "_Float64" || w == "_Float128" || w == "__float128" ||
                 w == "_Float32x" || w == "_Float64x" || w == "_Float128x" || w == "__ibm128" || w == "_BitInt" || w == "_ExtInt")
            slot = &other;
        if (slot == nullptr) return false;
        ++*slot;
        if (first == NONE) first = at;
        last = at;
        return true;
    }

    Builtin result() const {
        if (void_ != 0) return Builtin::void_;
        if (bool_ != 0) return Builtin::bool_;
        if (wchar != 0) return Builtin::wchar;
        if (char8 != 0) return Builtin::char8;
        if (char16 != 0) return Builtin::char16;
        if (char32 != 0) return Builtin::char32;
        if (other != 0) return Builtin::other;
        if (float_ != 0) return Builtin::float_;
        if (double_ != 0) return long_ != 0 ? Builtin::long_double : Builtin::double_;
        if (int128 != 0) return unsigned_ != 0 ? Builtin::unsigned_int128 : Builtin::int128;
        if (char_ != 0) return unsigned_ != 0 ? Builtin::unsigned_char : signed_ != 0 ? Builtin::signed_char : Builtin::char_;
        if (short_ != 0) return unsigned_ != 0 ? Builtin::unsigned_short : Builtin::short_;
        if (long_ >= 2) return unsigned_ != 0 ? Builtin::unsigned_long_long : Builtin::long_long;
        if (long_ == 1) return unsigned_ != 0 ? Builtin::unsigned_long : Builtin::long_;
        return unsigned_ != 0 ? Builtin::unsigned_int : Builtin::int_;
    }
};

constexpr std::string_view STORAGE_WORDS[] { "static", "extern", "thread_local", "register", "mutable", "typedef", "inline", "constexpr", "consteval",
                                             "constinit", "virtual", "explicit", "friend", "__inline", "__inline__", "_Thread_local", "__thread",
                                             "_Noreturn", "__forceinline", "__extension__" };

} // namespace

std::uint32_t BodyParser::cv_flags() {
    std::uint32_t flags { 0 };
    for (;;) {
        if (word("const") || word("__const")) flags |= TypeNode::const_;
        else if (word("volatile") || word("__volatile__")) flags |= TypeNode::volatile_;
        else if (word("restrict") || word("__restrict") || word("__restrict__")) flags |= TypeNode::restrict_;
        else if (word("_Nonnull") || word("_Nullable") || word("_Null_unspecified") || word("__unaligned") || word("__ptr32") || word("__ptr64")) {}
        else break;
        next();
    }
    return flags;
}

TypeId BodyParser::apply_cv(TypeId t, std::uint32_t flags) {
    if (flags != 0 && t) tree_.types[t.index].flags |= static_cast<std::uint16_t>(flags);
    return t;
}

// Attributes and their kin, skipped: [[...]], alignas(...), __attribute__((...)), __declspec(...).
void BodyParser::skip_attributes() {
    std::uint32_t first, last;
    skip_attributes(first, last);
}

void BodyParser::skip_attributes(std::uint32_t& first, std::uint32_t& last) {
    first = NONE;
    last = NONE;
    for (;;) {
        const std::uint32_t at { here() };
        if (is(Kind::l_square) && is(Kind::l_square, 1) && !(is(Kind::colon, 1) && adjacent(1))) skip_balanced();
        else if ((word("alignas") || word("__attribute__") || word("__declspec") || word("_Alignas") || word("__attribute")) && is(Kind::l_paren, 1)) {
            next();
            skip_balanced();
        } else return;
        if (first == NONE) first = at;
        last = prev();
    }
}

TypeId BodyParser::decltype_type() {
    const std::uint32_t first { here() };
    next();   // decltype
    if (!expect(Kind::l_paren, "`(`")) return error_type(first);
    TypeNode t;
    t.kind = TypeKind::decltype_;
    t.first = first;
    if (word("auto") && is(Kind::r_paren, 1)) {
        next();
        t.flags |= TypeNode::decltype_auto;
    } else {
        Nested nest { *this };
        t.expr = expression();
    }
    expect(Kind::r_paren, "`)`");
    t.last = prev();
    return add(t);
}

// One simple-type-specifier, no declarator: for T(x), T{x}.
TypeId BodyParser::simple_type() {
    const std::uint32_t first { here() };
    if (word("decltype") && is(Kind::l_paren, 1)) {
        const Mark m { mark() };
        const TypeId t { decltype_type() };
        if (!is(Kind::coloncolon)) return t;
        rewind(m);
        const NameId n { qualified_name(NameUse::type) };
        TypeNode named;
        named.kind = TypeKind::named;
        named.first = first;
        named.last = prev();
        named.name = n;
        return add(named);
    }
    if (word("auto")) {
        next();
        TypeNode t;
        t.kind = TypeKind::auto_;
        t.first = first;
        t.last = prev();
        return add(t);
    }
    if (word("typename")) return type_name_specifier(first, true, Site::block);
    Builtins b;
    while (ident_or_type_keyword() && b.add(tok().spelling, here())) next();
    if (!b.any()) {
        fail("expected a type");
        return error_type(first);
    }
    TypeNode t;
    t.kind = TypeKind::builtin;
    t.builtin = b.result();
    t.first = b.first;
    t.last = b.last;
    t.token = b.first;
    if (b.complex != 0) t.flags |= TypeNode::complex;
    return add(t);
}

// `typename` [::] nested-name-specifier identifier [<args>], or `typename [: r :]` through the extension.
TypeId BodyParser::type_name_specifier(std::uint32_t first, bool, Site) {
    const std::uint32_t key { here() };
    next();   // typename
    if (is(Kind::l_square) && !extensions_.empty()) {
        rewind_one_for_extension(key);
        TypeId out;
        for (const auto& ext : extensions_)
            if (ext->type_specifier(*this, out)) return out;
        fail("expected a name after `typename`");
        return error_type(first);
    }
    TypeNode t;
    t.kind = TypeKind::elaborated;
    t.first = first;
    t.token = key;
    t.name = qualified_name(NameUse::type);
    if (failed_) return error_type(first);
    t.last = prev();
    return add(t);
}

void BodyParser::rewind_one_for_extension(std::uint32_t key) {
    // The extension reads `typename [: r :]` from the `typename`.
    i_ = key;
    split_at_ = NPOS;
}

bool BodyParser::ident_or_type_keyword() const {
    return !eof() && tok().kind == Kind::raw_identifier && is_type_keyword(tok().spelling);
}

// A keyword only a type starts, at the cursor.
bool BodyParser::at_type_start(bool) {
    if (eof() || tok().kind != Kind::raw_identifier) return false;
    const std::string_view w { tok().spelling };
    if (is_type_keyword(w)) return true;
    return w == "typename" || w == "struct" || w == "class" || w == "union" || w == "enum" || w == "const" || w == "volatile" || w == "auto" ||
           w == "__typeof__" || w == "__typeof" || w == "_Atomic" || (w == "decltype" && is(Kind::l_paren, 1) && !decltype_followed_by_scope());
}

bool BodyParser::decltype_followed_by_scope() const {
    return is(Kind::coloncolon, 0) ? false : is(Kind::l_paren, 1) && is(Kind::coloncolon, 0 + (balanced(i_ + 1) - i_));
}

// ---- decl-specifiers ----

DeclSpec BodyParser::decl_specifiers(Site site) {
    DeclSpec spec;
    spec.first = here();
    Builtins b;
    std::uint32_t cv { 0 };
    TypeId named;                  // a type-name, class, enum, decltype, auto, typename
    std::uint32_t type_first { NONE };
    bool saw_signedness_only { false };
    (void)saw_signedness_only;
    for (;;) {
        if (failed_ || eof()) break;
        std::uint32_t af, al;
        skip_attributes(af, al);
        if (af != NONE) {
            if (spec.attributes_first == NONE) spec.attributes_first = af;
            spec.attributes_last = al;
        }
        // A type-name that starts at the global scope.
        if (is(Kind::coloncolon) && !named && !b.any()) {
            if (!type_name_in(spec, site, named, type_first)) break;
            continue;
        }
        if (eof() || tok().kind != Kind::raw_identifier) {
            // A splice type (`[: r :]`), through the extension.
            if (is(Kind::l_square) && !named && !b.any() && !extensions_.empty()) {
                TypeId out;
                bool taken { false };
                for (const auto& ext : extensions_)
                    if (ext->type_specifier(*this, out)) {
                        taken = true;
                        break;
                    }
                if (taken && !failed_) {
                    named = out;
                    type_first = tree_.types[out.index].first;
                    continue;
                }
            }
            break;
        }
        const std::string_view w { tok().spelling };
        // storage class and function specifiers
        if (std::ranges::contains(STORAGE_WORDS, w)) {
            if (w == "static") spec.flags |= Local::static_;
            else if (w == "extern") spec.flags |= Local::extern_;
            else if (w == "thread_local" || w == "_Thread_local" || w == "__thread") spec.flags |= Local::thread_local_;
            else if (w == "register") spec.flags |= Local::register_;
            else if (w == "mutable") spec.flags |= Local::mutable_;
            else if (w == "typedef") spec.flags |= Local::typedef_;
            else if (w == "inline" || w == "__inline" || w == "__inline__" || w == "__forceinline") spec.flags |= Local::inline_;
            else if (w == "constexpr") spec.flags |= Local::constexpr_;
            else if (w == "consteval") {
                // `consteval {` is a consteval block, not a specifier.
                if (is(Kind::l_brace, 1)) break;
                spec.flags |= Local::consteval_;
            } else if (w == "constinit") spec.flags |= Local::constinit_;
            else if (w == "virtual") spec.flags |= Local::virtual_;
            else if (w == "explicit") spec.flags |= Local::explicit_;
            else if (w == "friend") spec.flags |= Local::friend_;
            next();
            if (w == "explicit" && is(Kind::l_paren)) {
                next();
                Nested nest { *this };
                expression();
                expect(Kind::r_paren, "`)`");
            }
            continue;
        }
        if (w == "const" || w == "volatile" || w == "restrict" || w == "__restrict" || w == "__restrict__" || w == "__const" || w == "_Nonnull" ||
            w == "_Nullable" || w == "__volatile__") {
            cv |= cv_flags();
            continue;
        }
        if (named) break;   // one type-name only
        if (is_type_keyword(w)) {
            // A builtin after a type-name is the next declarator's problem (`Foo int`), not ours.
            if (type_first == NONE) type_first = here();
            b.add(w, here());
            next();
            if ((w == "_BitInt" || w == "_ExtInt") && is(Kind::l_paren)) skip_balanced();   // _BitInt(N): the width is its own
            continue;
        }
        if (b.any()) break;
        if (w == "class" || w == "struct" || w == "union") {
            type_first = here();
            const LocalId defined { class_specifier(spec, site) };
            (void)defined;
            named = spec.type;
            if (failed_) break;
            continue;
        }
        if (w == "enum") {
            type_first = here();
            enum_specifier(spec, site);
            named = spec.type;
            if (failed_) break;
            continue;
        }
        if (w == "typename") {
            type_first = here();
            named = type_name_specifier(type_first, true, site);
            continue;
        }
        if (w == "decltype" && is(Kind::l_paren, 1)) {
            type_first = here();
            const Mark m { mark() };
            const TypeId t { decltype_type() };
            if (is(Kind::coloncolon)) {
                rewind(m);
                const NameId n { qualified_name(NameUse::type) };
                TypeNode node;
                node.kind = TypeKind::named;
                node.first = type_first;
                node.last = prev();
                node.name = n;
                named = add(node);
            } else named = t;
            continue;
        }
        if (w == "auto" || w == "__auto_type") {
            type_first = here();
            next();
            TypeNode t;
            t.kind = TypeKind::auto_;
            t.first = type_first;
            t.last = prev();
            named = add(t);
            spec.placeholder = true;
            continue;
        }
        if (w == "__typeof__" || w == "__typeof" || w == "_Atomic") {
            if (is(Kind::l_paren, 1)) {
                type_first = here();
                const bool atomic { w == "_Atomic" };
                next();
                next();
                TypeNode t;
                t.kind = atomic ? TypeKind::atomic : TypeKind::typeof_;
                t.first = type_first;
                const Handle h { type_or_expression() };
                if (h.sort == Sort::type) t.inner = { h.index };
                else t.expr = { h.index };
                expect(Kind::r_paren, "`)`");
                t.last = prev();
                named = add(t);
                continue;
            }
            if (w == "_Atomic") {   // _Atomic as a qualifier
                next();
                continue;
            }
        }
        // A type-name: a (qualified) name that names a type.
        if (ident() || w == "template") {
            if (!(ident() || is(Kind::coloncolon))) break;
        }
        if (!(ident() || is(Kind::coloncolon))) break;
        if (!type_name_in(spec, site, named, type_first)) break;
    }
    while (!failed_ && (word("const") || word("volatile") || word("restrict") || word("__restrict") || word("__restrict__"))) cv |= cv_flags();
    if (b.any()) {
        TypeNode t;
        t.kind = TypeKind::builtin;
        t.builtin = b.result();
        t.first = b.first;
        t.last = b.last;
        t.token = b.first;
        if (b.complex != 0) t.flags |= TypeNode::complex;
        spec.type = add(t);
        spec.has_type = true;
    } else if (named) {
        spec.type = named;
        spec.has_type = true;
    }
    if (spec.has_type) apply_cv(spec.type, cv);
    spec.last = prev();
    return spec;
}

// A (qualified) type-name at the cursor, as the type of a declaration: false, nothing taken, when the name
// is not one (it names a value, or no declarator could follow it as a declaration).
bool BodyParser::type_name_in(DeclSpec& spec, Site site, TypeId& named, std::uint32_t& type_first) {
    const std::uint32_t first { here() };
    const Mark m { mark() };
    NameId n;
    {
        ++speculating_;
        const bool was_failed { std::exchange(failed_, false) };
        n = qualified_name(NameUse::type);
        const bool ok { !failed_ && n };
        failed_ = was_failed;
        --speculating_;
        if (!ok) {
            rewind(m);
            return false;
        }
    }
    const NameInfo info { classify_name(n, first) };
    last_type_class_ = info.what;
    last_type_basis_ = info.basis;
    // The name names a value, or a template used without arguments where a type is wanted: not a type.
    if (info.what == NameClass::value || info.what == NameClass::function_template || info.what == NameClass::variable_template ||
        info.what == NameClass::namespace_) {
        rewind(m);
        return false;
    }
    if (info.what == NameClass::class_template || info.what == NameClass::alias_template) {
        // A class template named without arguments: a placeholder for a deduced class type (`std::vector v{...}`).
    }
    // A name followed by `(` in a class is a constructor's name (the class's own), not a type.
    if (site == Site::class_ && is(Kind::l_paren) && !tree_.names[n.index].global &&
        (info.what != NameClass::type || (!class_names_.empty() && spelled(tree_.components[tree_.names[n.index].components.begin].token) == class_names_.back()))) {
        rewind(m);
        return false;
    }
    TypeNode t;
    t.kind = TypeKind::named;
    t.first = first;
    t.last = prev();
    t.name = n;
    TypeId id { add(t) };
    // C++26: a pack-indexed type `T...[n]`.
    if (is(Kind::ellipsis) && is(Kind::l_square, 1) && !extensions_.empty())
        for (const auto& ext : extensions_)
            if (ext->type_postfix(*this, id)) break;
    // A constrained placeholder: `Concept auto x`, `Concept<T> auto x`.
    if (word("auto") || (word("decltype") && is(Kind::l_paren, 1) && word("auto", 2))) {
        TypeNode placeholder;
        placeholder.first = first;
        placeholder.name = n;
        if (word("auto")) {
            placeholder.kind = TypeKind::auto_;
            next();
        } else {
            placeholder.kind = TypeKind::decltype_;
            placeholder.flags |= TypeNode::decltype_auto;
            advance(4);
        }
        placeholder.last = prev();
        id = add(placeholder);
        spec.placeholder = true;
    }
    if (type_first == NONE) type_first = first;
    named = id;
    return true;
}

// ---- declarators ----

TypeId BodyParser::pointer_operators(TypeId base) {
    for (;;) {
        skip_attributes();
        const std::uint32_t first { here() };
        if (is(Kind::star)) {
            next();
            TypeNode p;
            p.kind = TypeKind::pointer;
            p.first = first;
            p.inner = base;
            const std::uint32_t cv { cv_flags() };
            p.flags = static_cast<std::uint16_t>(cv);
            p.last = prev();
            base = add(p);
            continue;
        }
        if (is(Kind::amp) || is(Kind::ampamp)) {
            const bool rvalue { is(Kind::ampamp) };
            next();
            TypeNode r;
            r.kind = rvalue ? TypeKind::rvalue_ref : TypeKind::lvalue_ref;
            r.first = first;
            r.inner = base;
            r.last = prev();
            base = add(r);
            continue;
        }
        // class::*  (a nested-name-specifier then `*`)
        if (ident() || is(Kind::coloncolon)) {
            std::size_t k { 0 };
            if (is(Kind::coloncolon)) ++k;
            bool member { false };
            std::size_t j { i_ + k };
            // Scan a qualified name by tokens: id [<...>] :: ... ::*
            while (j < end_ && t_[j].kind == Kind::raw_identifier) {
                ++j;
                if (j < end_ && t_[j].kind == Kind::less) {
                    const std::size_t after { angle_end(j) };
                    if (after == j) break;
                    j = after;
                }
                if (!(j < end_ && t_[j].kind == Kind::coloncolon)) break;
                ++j;
                if (j < end_ && t_[j].kind == Kind::star) {
                    member = true;
                    break;
                }
            }
            if (member) {
                TypeNode p;
                p.kind = TypeKind::member_pointer;
                p.first = first;
                // The class name: everything before the final `::*`.
                const std::size_t star { j };
                Name n;
                n.first = first;
                n.last = static_cast<std::uint32_t>(star - 2);
                bool again { true };
                (void)again;
                const Mark m { mark() };
                const bool was_failed { std::exchange(failed_, false) };
                ++speculating_;
                const NameId name { qualified_name(NameUse::type) };
                --speculating_;
                const bool ok { !failed_ && name };
                failed_ = was_failed;
                if (!ok) {
                    rewind(m);
                    break;
                }
                if (!is(Kind::coloncolon) || !is(Kind::star, 1)) {
                    rewind(m);
                    break;
                }
                advance(2);   // ::*
                p.name = name;
                p.inner = base;
                p.flags = static_cast<std::uint16_t>(cv_flags());
                p.last = prev();
                base = add(p);
                continue;
            }
        }
        return base;
    }
}

bool BodyParser::starts_nested_declarator() const {
    // The `(` at the cursor opens a declarator when it holds one: a pointer operator, a pointer to member, a
    // parenthesized name, an attribute -- not a parameter list.
    std::size_t k { 1 };
    if (is(Kind::l_square, k) && is(Kind::l_square, k + 1)) return true;
    if (word("__attribute__", k)) return true;
    const Kind next_kind { kind(k) };
    if (next_kind == Kind::star || next_kind == Kind::amp || next_kind == Kind::ampamp || next_kind == Kind::caret) return true;
    if (next_kind == Kind::l_paren) return true;
    if (next_kind == Kind::raw_identifier || next_kind == Kind::coloncolon) {
        // class::*  -- an identifier chain ending in `::*`
        std::size_t j { i_ + k };
        if (j < end_ && t_[j].kind == Kind::coloncolon) ++j;
        while (j < end_ && t_[j].kind == Kind::raw_identifier) {
            ++j;
            if (j < end_ && t_[j].kind == Kind::less) {
                const std::size_t after { angle_end(j) };
                if (after == j) return false;
                j = after;
            }
            if (!(j < end_ && t_[j].kind == Kind::coloncolon)) break;
            ++j;
            if (j < end_ && t_[j].kind == Kind::star) return true;
        }
    }
    return false;
}

// Past the `>` of the `<` at `at_less` by counting brackets, or `at_less` when there is none in the part.
std::size_t BodyParser::angle_end(std::size_t at_less) const {
    int depth { 0 };
    for (std::size_t k { at_less }; k < end_; ++k) {
        const Kind kd { t_[k].kind };
        if (kd == Kind::less) ++depth;
        else if (kd == Kind::greater) {
            if (--depth == 0) return k + 1;
        } else if (kd == Kind::greatergreater) {
            depth -= 2;
            if (depth <= 0) return depth == 0 ? k + 1 : at_less;
        } else if (kd == Kind::l_paren || kd == Kind::l_square || kd == Kind::l_brace) k = balanced(k) - 1;
        else if (kd == Kind::semi || kd == Kind::r_brace || kd == Kind::r_paren || kd == Kind::r_square) return at_less;
    }
    return at_less;
}

// At a `(` after a declarator-id: a parameter list, or a variable's direct initializer?
bool BodyParser::starts_parameter_clause(Mode mode, Site site) {
    if (mode == Mode::new_declarator) return false;
    if (site == Site::class_ || site == Site::parameter || site == Site::template_parameter || site == Site::lambda_capture) return true;
    if (site != Site::block && site != Site::namespace_ && site != Site::for_init && site != Site::condition) return true;
    // `T x(...)` at a block or namespace: parameters when the first thing in the parentheses reads as one.
    if (is(Kind::r_paren, 1)) return true;
    if (is(Kind::ellipsis, 1)) return true;
    std::size_t k { 1 };
    // attributes before a parameter
    if (is(Kind::l_square, k) && is(Kind::l_square, k + 1)) return true;
    if (eof(k)) return false;
    const std::size_t at { i_ + k };
    const PpToken& first { t_[at] };
    if (first.kind == Kind::raw_identifier) {
        const std::string_view w { first.spelling };
        if (is_type_keyword(w) || w == "const" || w == "volatile" || w == "typename" || w == "struct" || w == "class" || w == "union" || w == "enum" ||
            w == "decltype" || w == "auto" || w == "register")
            return true;
        if (w == "this" || w == "true" || w == "false" || w == "nullptr" || w == "sizeof" || w == "new" || w == "alignof" || w == "noexcept" ||
            w == "static_cast" || w == "dynamic_cast" || w == "const_cast" || w == "reinterpret_cast" || w == "throw" || w == "requires")
            return false;
        // A name: a type when something knows it; a value likewise; unknown: two names in a row (`Bar b`)
        // read as a parameter, a name followed by `,` `)` `.` `->` an operator as an expression.
        // The whole qualified name is what is classified (`_Ops::f` is not `_Ops`).
        std::string written { w };
        for (std::size_t q { at + 1 }; q + 1 < end_ && t_[q].kind == Kind::coloncolon && t_[q + 1].kind == Kind::raw_identifier; q += 2)
            written += "::" + std::string { t_[q + 1].spelling };
        const NameInfo info { classify(written, at) };
        if (info.dependent) return false;   // a dependent qualified name is a value without `typename`
        if (info.what == NameClass::type || type_template(info.what)) {
            // `T x(U(y))` ...: with a type known the parentheses hold a parameter, unless a value follows it
            // as an initializer would be: `Foo f(Bar{})`, `Foo f(Bar(1))`; and every element must be one.
            if (is(Kind::l_brace, k + 1)) return false;
            return every_element_reads_as_parameter(i_);
        }
        if (info.what == NameClass::value) return false;
        // Unknown: look at what follows the name (qualified names and template-ids skipped by token).
        std::size_t j { at + 1 };
        while (j + 1 < end_ && t_[j].kind == Kind::coloncolon && t_[j + 1].kind == Kind::raw_identifier) j += 2;
        if (j < end_ && t_[j].kind == Kind::less) {
            const std::size_t after { angle_end(j) };
            if (after != j) j = after;
        }
        if (j >= end_) return false;
        const Kind f { t_[j].kind };
        if (f == Kind::raw_identifier && !reserved_operator_word(t_[j].spelling)) return true;
        if ((f == Kind::star || f == Kind::amp || f == Kind::ampamp) && j + 1 < end_ &&
            (t_[j + 1].kind == Kind::raw_identifier || t_[j + 1].kind == Kind::r_paren || t_[j + 1].kind == Kind::comma))
            return t_[j + 1].kind != Kind::raw_identifier || !(is_unary_word(t_[j + 1].spelling));
        return false;
    }
    if (first.kind == Kind::coloncolon) {
        // `T x(::ns::Type)` -- as a name starting at the global scope
        return false;
    }
    return false;
}

// The `(` at `open`: whether each of its comma-separated elements could be a parameter -- begins with a type the scopes or
// lookup know, a type keyword, or a name and a name, `*` or `&` -- rather than `std::move(x)`, `a + b`, a literal.
bool BodyParser::every_element_reads_as_parameter(std::size_t open) {
    const std::size_t close { balanced(open) - 1 };
    std::size_t k { open + 1 };
    while (k < close) {
        // the element's tokens up to a top-level comma
        std::size_t e { k };
        while (e < close && t_[e].kind != Kind::comma) {
            if (t_[e].kind == Kind::l_paren || t_[e].kind == Kind::l_square || t_[e].kind == Kind::l_brace) e = balanced(e);
            else if (t_[e].kind == Kind::less && e > k && t_[e - 1].kind == Kind::raw_identifier) {
                const std::size_t after { angle_end(e) };
                e = after == e ? e + 1 : after;
            } else ++e;
        }
        if (e == k) return false;
        if (t_[k].kind != Kind::raw_identifier && t_[k].kind != Kind::coloncolon) return false;
        const std::string_view w { t_[k].spelling };
        if (!(is_type_keyword(w) || w == "const" || w == "volatile" || w == "typename" || w == "struct" || w == "class" || w == "enum" || w == "decltype" || w == "auto")) {
            // a name: the qualified name, classified whole
            std::string written;
            std::size_t q { k };
            if (t_[q].kind == Kind::coloncolon) {
                written = "::";
                ++q;
            }
            while (q < e && t_[q].kind == Kind::raw_identifier) {
                written += t_[q].spelling;
                ++q;
                if (q < e && t_[q].kind == Kind::less) {
                    const std::size_t after { angle_end(q) };
                    if (after == q) break;
                    q = after;
                }
                if (q + 1 < e && t_[q].kind == Kind::coloncolon && t_[q + 1].kind == Kind::raw_identifier) {
                    written += "::";
                    ++q;
                } else break;
            }
            const NameInfo info { classify(written, k) };
            const bool known_type { info.what == NameClass::type || type_template(info.what) };
            if (info.what == NameClass::value || info.dependent) return false;
            if (!known_type) {
                // unknown: followed by a name, a pointer or reference, a pack
                if (q >= e) return false;
                const Kind f { t_[q].kind };
                if (!(f == Kind::raw_identifier || f == Kind::star || f == Kind::amp || f == Kind::ampamp || f == Kind::ellipsis)) return false;
            }
        }
        k = e + 1;
    }
    return true;
}

TypeId BodyParser::function_type(TypeId return_type, Span32 params, bool variadic, std::uint32_t first) {
    TypeNode f;
    f.kind = TypeKind::function;
    f.first = first;
    f.inner = return_type;
    f.list = params;
    if (variadic) f.flags |= TypeNode::c_variadic;
    f.last = prev();
    return add(f);
}

// What follows a function declarator's parameters: cv, ref, noexcept, dynamic exception, attributes,
// contract specifiers, a trailing return type. Into `fn` (the type), the contracts into `contracts`.
void BodyParser::function_qualifiers(TypeId fn, std::vector<std::uint32_t>* contracts) {
    for (;;) {
        if (failed_ || eof()) return;
        skip_attributes();
        TypeNode& f { tree_.types[fn.index] };
        if (word("const")) {
            f.flags |= TypeNode::const_method;
            next();
        } else if (word("volatile")) {
            f.flags |= TypeNode::volatile_method;
            next();
        } else if (is(Kind::amp)) {
            f.flags |= TypeNode::lvalue_method;
            next();
        } else if (is(Kind::ampamp)) {
            f.flags |= TypeNode::rvalue_method;
            next();
        } else if (word("noexcept")) {
            next();
            f.flags |= TypeNode::noexcept_;
            if (is(Kind::l_paren)) {
                next();
                Nested nest { *this };
                const ExprId cond { expression() };
                tree_.types[fn.index].expr = cond;
                expect(Kind::r_paren, "`)`");
            }
        } else if (word("throw") && is(Kind::l_paren, 1)) {
            next();
            skip_balanced();
            f.flags |= TypeNode::dynamic_throw;
        } else if (is(Kind::arrow)) {
            next();
            const TypeId ret { type_id() };
            tree_.types[fn.index].inner = ret;
            tree_.types[fn.index].flags |= TypeNode::trailing;
        } else if (ident() && is(Kind::l_paren, 1) && !extensions_.empty() && contracts != nullptr) {
            FunctionSpecifiers specs;
            bool taken { false };
            for (const auto& ext : extensions_)
                if (ext->function_specifier(*this, specs)) {
                    taken = true;
                    break;
                }
            if (!taken) return;
            for (const auto c : specs.contracts) contracts->push_back(c);
        } else {
            return;
        }
        tree_.types[fn.index].last = prev();
    }
}

LocalId BodyParser::parameter_declaration() {
    Local p;
    p.kind = LocalKind::parameter;
    p.entity = msa::Kind::parameter;
    p.first = here();
    skip_attributes();
    if (word("this") && !is(Kind::l_paren, 1)) {
        next();
        p.flags |= Local::explicit_object;
    }
    const DeclSpec spec { decl_specifiers(Site::parameter) };
    if (failed_) return add(p);
    if (!spec.has_type) {
        fail("expected a parameter's type");
        return add(p);
    }
    Declarator d { declarator(spec.type, Mode::either, Site::parameter) };
    p.type = d.type ? d.type : spec.type;
    p.name = d.name;
    p.name_token = d.name_token;
    p.flags |= spec.flags;
    if (d.pack) p.flags |= Local::pack;
    if (is(Kind::equal)) {
        next();
        p.flags |= Local::copy_init;
        Nested nest { *this };
        p.init = initializer_clause();
    }
    p.last = prev();
    return add(p);
}

// The parameter-declaration-clause after the `(`, through the `)`.
Span32 BodyParser::parameter_declaration_clause(bool* c_variadic) {
    std::vector<Handle> params;
    push_scope();
    Nested nest { *this };
    while (!failed_ && !is(Kind::r_paren) && !eof()) {
        if (is(Kind::ellipsis)) {
            next();
            *c_variadic = true;
            break;
        }
        const LocalId p { parameter_declaration() };
        if (failed_) break;
        const Local& made { tree_.locals[p.index] };
        if (made.name_token != NONE) declare_name(made.name_token, NameClass::value);
        params.push_back(p.handle());
        if (accept(Kind::comma)) {
            if (is(Kind::ellipsis)) {
                next();
                *c_variadic = true;
                break;
            }
            continue;
        }
        if (is(Kind::ellipsis)) {   // `T x...` after the last: C variadic, `int x, ...` w/o comma
            next();
            *c_variadic = true;
        }
        break;
    }
    pop_scope();
    if (!failed_) expect(Kind::r_paren, "`)`");
    // (void) is no parameter.
    if (params.size() == 1) {
        const Local& only { tree_.locals[params[0].index] };
        if (only.name_token == NONE && only.type && tree_.types[only.type.index].kind == TypeKind::builtin && tree_.types[only.type.index].builtin == Builtin::void_)
            params.clear();
    }
    return emit(params);
}

// Suffixes after a declarator-id: parameter lists and array bounds, applied inside out.
bool BodyParser::suffixes(TypeId& type, Mode mode, Site site, Declarator* d) {
    struct Suffix {
        bool function { false };
        std::uint32_t first { 0 };
        Span32 params;
        bool variadic { false };
        ExprId bound;
        bool vla { false };
        TypeId fn;   // function: the type, made with a placeholder return type
    };
    std::vector<Suffix> found;
    bool first_suffix { true };
    for (;;) {
        if (failed_ || eof()) break;
        skip_attributes();
        if (is(Kind::l_square) && !(is(Kind::colon, 1) && adjacent(1))) {
            Suffix s;
            s.first = here();
            next();
            if (is(Kind::star) && is(Kind::r_square, 1)) {
                next();
                s.vla = true;
            } else if (!is(Kind::r_square)) {
                Nested nest { *this };
                s.bound = assignment_expression();
            }
            expect(Kind::r_square, "`]`");
            found.push_back(s);
            first_suffix = false;
            continue;
        }
        if (is(Kind::l_paren) && mode != Mode::new_declarator) {
            if (first_suffix && mode == Mode::named && !starts_parameter_clause(mode, site)) break;
            Suffix s;
            s.function = true;
            s.first = here();
            next();
            s.params = parameter_declaration_clause(&s.variadic);
            if (failed_) break;
            // The function type, so its qualifiers have a node to go in; its return type is set when applied.
            s.fn = function_type(TypeId {}, s.params, s.variadic, s.first);
            // The parameters' names are visible in what follows them: a trailing return type, a contract's condition.
            push_scope();
            for (const auto h : tree_.list(s.params))
                if (tree_.locals[h.index].name_token != NONE) declare_name(tree_.locals[h.index].name_token, NameClass::value);
            function_qualifiers(s.fn, d != nullptr ? &d->contracts : nullptr);
            pop_scope();
            found.push_back(s);
            if (first_suffix && d != nullptr) {
                d->function = true;
                d->params = s.params;
            }
            first_suffix = false;
            continue;
        }
        break;
    }
    // Applied from the last: `int a[2][3]` is an array of 2 arrays of 3 ints.
    for (auto it = found.rbegin(); it != found.rend(); ++it) {
        if (it->function) {
            tree_.types[it->fn.index].inner = tree_.types[it->fn.index].has(TypeNode::trailing) ? tree_.types[it->fn.index].inner : type;
            // With a trailing return, `auto` was the leading type: the written one is the return type.
            type = it->fn;
        } else {
            TypeNode a;
            a.kind = TypeKind::array;
            a.first = it->first;
            a.last = prev();
            a.inner = type;
            a.expr = it->bound;
            if (it->vla) a.flags |= TypeNode::vla;
            type = add(a);
        }
    }
    return !failed_;
}

Declarator BodyParser::declarator(TypeId base, Mode mode, Site site) {
    Declarator d;
    d.first = here();
    if (failed_) return d;
    if (++depth_ > 200) {
        --depth_;
        fail("declarator nested too deeply");
        return d;
    }
    struct Depth {
        int& n;
        ~Depth() { --n; }
    } depth_guard { depth_ };
    TypeId type { pointer_operators(base) };
    skip_attributes();
    if (mode != Mode::new_declarator && is(Kind::l_paren) && (mode == Mode::named ? true : starts_nested_declarator()) &&
        !(mode == Mode::named && site != Site::block && false)) {
        // ( declarator ): its suffixes first, then what is inside, over the type they make.
        if (mode == Mode::named || starts_nested_declarator()) {
            const std::size_t open { i_ };
            const std::size_t close { balanced(open) - 1 };
            if (close >= end_ || !(t_[close].kind == Kind::r_paren)) {
                fail("unbalanced parentheses in a declarator");
                return d;
            }
            i_ = close + 1;
            split_at_ = NPOS;
            Declarator after;
            TypeId derived { type };
            suffixes(derived, mode, site, &after);
            const std::size_t resume { i_ };
            i_ = open + 1;
            Declarator inner { declarator(derived, mode, site) };
            if (!failed_ && i_ != close) {
                if (!(is(Kind::r_paren))) fail("expected `)` in a declarator");
            }
            i_ = resume;
            split_at_ = NPOS;
            inner.first = d.first;
            inner.last = prev();
            // A function declarator inside parentheses (`(*f)(int)`): the pointer is the declarator's; the
            // function is its type's.
            if (!inner.function && after.function) inner.params = after.params;
            inner.contracts.insert(inner.contracts.end(), after.contracts.begin(), after.contracts.end());
            // `T (x)`, `T (*x)`: a name (and its pointers) in parentheses, no parameters or bounds after them.
            inner.parenthesized_name = inner.name && !inner.function && !after.function && derived == type;
            return inner;
        }
    }
    if (mode != Mode::abstract && mode != Mode::new_declarator) {
        if (is(Kind::ellipsis) && !(is(Kind::l_square, 1))) {
            next();
            d.pack = true;
        }
        if (starts_name() && !(mode == Mode::either && !(ident() || is(Kind::coloncolon) || is(Kind::tilde) || word("operator")))) {
            d.name_token = here();
            d.name = qualified_name(NameUse::declarator);
            if (failed_) return d;
            d.name_token = tree_.components[tree_.names[d.name.index].components.begin + tree_.names[d.name.index].components.count - 1].token;
            d.ok = true;
        }
    }
    skip_attributes();
    suffixes(type, mode, site, &d);
    d.type = type;
    d.ok = d.ok || mode == Mode::abstract || mode == Mode::new_declarator || mode == Mode::either;
    d.last = prev();
    return d;
}

TypeId BodyParser::type_id() {
    const std::uint32_t first { here() };
    if (failed_) return error_type(first);
    const DeclSpec spec { decl_specifiers(Site::parameter) };
    if (failed_) return error_type(first);
    if (!spec.has_type) {
        fail("expected a type");
        return error_type(first);
    }
    const Declarator d { declarator(spec.type, Mode::abstract, Site::parameter) };
    return d.type ? d.type : spec.type;
}

// ---- type or expression ----

bool BodyParser::reserved_operator_word(std::string_view w) {
    return w == "and" || w == "or" || w == "not" || w == "xor" || w == "bitand" || w == "bitor" || w == "compl" || w == "and_eq" || w == "or_eq" ||
           w == "xor_eq" || w == "not_eq" || w == "const" || w == "volatile";
}

bool BodyParser::is_unary_word(std::string_view w) { return w == "sizeof" || w == "new" || w == "this" || w == "true" || w == "false" || w == "nullptr"; }

Handle BodyParser::type_or_expression() { return type_or_expression_biased(false); }

// `unknown_is_expression`: a lone unqualified name nothing knows, followed by `)`, is a value (sizeof(x)); else a type (f<T>).
Handle BodyParser::type_or_expression_biased(bool unknown_is_expression) {
    const std::uint32_t first { here() };
    if (failed_) return error_expr(first).handle();
    if (eof()) {
        fail("expected a type or an expression");
        return error_expr(first).handle();
    }
    bool as_type { false };
    bool template_name { false };
    ast::Basis basis { ast::Basis::syntax };
    if (at_type_start(false)) {
        as_type = true;
    } else if (ident() || is(Kind::coloncolon) || (word("decltype") && is(Kind::l_paren, 1))) {
        // Read the name, and ask what it is; then read the argument as that.
        NameInfo info;
        Kind follower { Kind::unknown };
        std::size_t follower_at { 0 };
        bool has_arguments { false };
        bool parsed { false };
        std::string written;
        {
            const Mark m { mark() };
            ++speculating_;
            const bool was_failed { std::exchange(failed_, false) };
            const NameId n { qualified_name(NameUse::type) };
            parsed = !failed_ && n;
            if (parsed) {
                info = classify_name(n, first);
                written = name_text(n);
                follower = kind();
                follower_at = i_;
                const auto& parts { tree_.parts(tree_.names[n.index]) };
                has_arguments = !parts.empty() && parts.back().has(NameComponent::has_arguments);
            }
            failed_ = was_failed;
            --speculating_;
            rewind(m);
        }
        if (!parsed) {
            as_type = false;
            basis = ast::Basis::syntax;
        } else if (type_like(info.what) || (type_template(info.what) && has_arguments)) {
            as_type = true;
            basis = info.basis;
        } else if ((type_template(info.what)) && !has_arguments) {
            // A class template named alone: a template template argument when the argument ends here.
            if (follower == Kind::comma || at_gt_kind(follower) || follower == Kind::ellipsis) {
                template_name = true;
                basis = info.basis;
            } else {
                as_type = true;   // `std::vector v` style placeholder use: still a type
                basis = info.basis;
            }
        } else if (info.what != NameClass::unknown) {
            as_type = false;   // a value, a function or variable template, a concept, a namespace
            basis = info.basis;
        } else if (info.dependent) {
            as_type = false;   // a dependent name is a value unless `typename` says otherwise
            basis = ast::Basis::scope;
        } else {
            basis = ast::Basis::shape;
            const std::string_view last { last_component(written) };
            if (last.ends_with("_v") || last == "value") as_type = false;
            else if (last.ends_with("_t") && has_arguments) as_type = true;
            else if (unknown_is_expression && (follower == Kind::r_paren || follower == Kind::comma) && written.find("::") == std::string::npos && !has_arguments) as_type = false;
            else if (follower == Kind::raw_identifier && (!reserved_operator_word(t_[follower_at].spelling) || t_[follower_at].spelling == "const" ||
                                                          t_[follower_at].spelling == "volatile"))
                as_type = true;   // `T x`, `T const`: a name and a name
            else if (follower == Kind::l_paren && !unknown_is_expression) {
                // `R(Args)`: a function type when it reads as a type that ends the argument; `f(x)` otherwise.
                const Mark m { mark() };
                ++speculating_;
                const bool was_failed { std::exchange(failed_, false) };
                type_id();
                as_type = !failed_ && (is(Kind::comma) || at_gt() || is(Kind::r_paren) || is(Kind::ellipsis));
                failed_ = was_failed;
                --speculating_;
                rewind(m);
            } else
                as_type = follower == Kind::comma || at_gt_kind(follower) || follower == Kind::r_paren || follower == Kind::ellipsis || follower == Kind::star ||
                          follower == Kind::amp || follower == Kind::ampamp || follower == Kind::unknown;
        }
    }
    note(Ambiguity::type_or_expression_argument, template_name || !as_type ? Resolution::expression_argument : Resolution::type_argument, basis, first);
    if (template_name) {
        const NameId n { qualified_name(NameUse::type) };
        return n.handle();
    }
    if (as_type) return type_id().handle();
    return assignment_expression().handle();
}

} // namespace mcxx::frontend::bodies
