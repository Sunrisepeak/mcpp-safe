// Declarations a block or a class holds: simple-declarations and their declarators, initializers, classes,
// enums, using, static_assert, structured bindings, templates, and function bodies with their constructor
// initializers. The same grammar reads a namespace's.
module mcxx.frontend;

import std;
import mcxx.msa;
import mcxx.base;
import :bodyparser;

namespace mcxx::frontend::bodies {

namespace {

bool starts_only_declaration(std::string_view w) {
    return w == "typedef" || w == "static" || w == "extern" || w == "thread_local" || w == "register" || w == "constinit" || w == "inline" ||
           w == "friend" || w == "using" || w == "template" || w == "_Thread_local" || w == "__thread" || w == "mutable" || w == "virtual" ||
           w == "explicit" || w == "__extension__" || w == "_Noreturn";
}

} // namespace

// ---- classification of a statement's start ----

// True where the tokens at the cursor can only be a declaration.
bool BodyParser::is_declaration_start(Site) {
    if (eof() || tok().kind != Kind::raw_identifier) return false;
    const std::string_view w { tok().spelling };
    if (w == "constexpr" || w == "consteval") return !(is(Kind::l_brace, 1));
    if (w == "namespace" && ident(1) && is(Kind::equal, 2)) return true;
    return starts_only_declaration(w);
}

LocalId BodyParser::link_outline(LocalId id) {
    if (outline_ == nullptr || !id) return id;
    Local& l { tree_.locals[id.index] };
    if (l.name_token == NONE) return id;
    if (const auto it { outline_->find(l.name_token) }; it != outline_->end()) l.outline = it->second;
    return id;
}

// ---- declaration statements ----

StmtId BodyParser::declaration_statement() {
    const std::uint32_t first { here() };
    bool definition { false };
    const std::vector<LocalId> locals { simple_declaration(Site::block, &definition) };
    Stmt s;
    s.kind = StmtKind::declaration;
    s.first = first;
    s.last = prev();
    std::vector<Handle> handles;
    for (const auto l : locals) handles.push_back(l.handle());
    s.list = emit(handles);
    return add(s);
}

// A simple-declaration, a function definition, or the non-simple declarations that start the same ways.
std::vector<LocalId> BodyParser::simple_declaration(Site site, bool* function_definition) {
    std::vector<LocalId> out;
    *function_definition = false;
    const std::uint32_t first { here() };
    skip_attributes();
    if (word("static_assert") || word("_Static_assert")) {
        out.push_back(static_assert_declaration());
        return out;
    }
    if (word("using")) {
        out.push_back(using_declaration(site));
        return out;
    }
    if (word("template") && !(is(Kind::l_paren, 1))) {
        out.push_back(template_declaration(site));
        return out;
    }
    if (word("namespace") && ident(1) && is(Kind::equal, 2)) {
        out.push_back(namespace_alias());
        return out;
    }
    if ((word("asm") || word("__asm__") || word("__asm")) && (is(Kind::l_paren, 1) || word("volatile", 1) || word("__volatile__", 1) || word("goto", 1))) {
        out.push_back(asm_declaration());
        return out;
    }
    if (word("extern") && is_string(kind(1))) {   // extern "C" { ... } or extern "C" declaration
        Local l;
        l.kind = LocalKind::linkage;
        l.first = first;
        next();
        next();
        if (is(Kind::l_brace)) {
            next();
            push_scope();
            std::vector<LocalId> members { member_specification_until_brace(site) };
            pop_scope();
            expect(Kind::r_brace, "`}`");
            std::vector<Handle> handles;
            for (const auto m : members) handles.push_back(m.handle());
            l.members = emit(handles);
        } else {
            bool def { false };
            std::vector<LocalId> inner { simple_declaration(site, &def) };
            std::vector<Handle> handles;
            for (const auto m : inner) handles.push_back(m.handle());
            l.members = emit(handles);
            *function_definition = def;
        }
        l.last = prev();
        out.push_back(add(l));
        return out;
    }
    if (is(Kind::semi)) {
        Local l;
        l.kind = LocalKind::empty;
        l.first = first;
        next();
        l.last = prev();
        out.push_back(add(l));
        return out;
    }
    // Extension declarations: consteval { ... }.
    if (!extensions_.empty() && word("consteval") && is(Kind::l_brace, 1)) {
        LocalId made;
        for (const auto& ext : extensions_)
            if (ext->declaration(*this, made)) {
                out.push_back(made);
                return out;
            }
    }
    DeclSpec spec { decl_specifiers(site) };
    if (failed_) return out;
    // No declarator: `struct S { ... };`, `enum E;`, `friend class X;`.
    if (is(Kind::semi)) {
        next();
        if (spec.defined) {
            out.push_back(spec.defined);
        } else if (spec.has_type && (spec.flags & Local::friend_) != 0) {
            Local l;
            l.kind = LocalKind::friend_;
            l.first = first;
            l.last = prev();
            l.type = spec.type;
            out.push_back(add(l));
        } else if (spec.has_type) {
            // an elaborated-type-specifier alone (`struct S;` made no class): nothing declared
            Local l;
            l.kind = LocalKind::empty;
            l.first = first;
            l.last = prev();
            l.type = spec.type;
            out.push_back(add(l));
        } else {
            fail("expected a declaration");
        }
        return out;
    }
    // Structured bindings: auto [a, b] = e;
    if (spec.placeholder && (is(Kind::l_square) || ((is(Kind::amp) || is(Kind::ampamp)) && is(Kind::l_square, 1)))) {
        out.push_back(structured_binding(spec, site));
        if (!failed_ && !accept(Kind::semi)) fail("expected `;` after a structured binding");
        return out;
    }
    for (bool again { true }; again && !failed_;) {
        again = false;
        bool definition { false };
        const LocalId l { init_declarator(spec, site, out.empty(), &definition) };
        if (failed_) break;
        out.push_back(l);
        if (definition) {
            *function_definition = true;
            return out;
        }
        if (accept(Kind::comma)) {
            again = true;
            continue;
        }
        if (!expect(Kind::semi, "`;` after a declaration")) break;
    }
    if (spec.defined && !out.empty()) {
        // The class or enum the specifiers define is a declaration of its own, before its declarators'.
        out.insert(out.begin(), spec.defined);
    }
    return out;
}

void BodyParser::bind_declared(const Local& l) {
    if (l.name_token == NONE) return;
    // A constructor has its class's name, which the class declared already: as a type, the injected-class-name.
    if (l.entity == msa::Kind::constructor || l.entity == msa::Kind::destructor || l.entity == msa::Kind::conversion) return;
    NameClass what { NameClass::value };
    if (l.kind == LocalKind::type_alias || l.kind == LocalKind::class_ || l.kind == LocalKind::enum_) what = NameClass::type;
    declare_name(l.name_token, what);
}

// A declarator, then what follows it: a function's tail and body, or an initializer.
LocalId BodyParser::init_declarator(const DeclSpec& spec, Site site, bool, bool* definition) {
    *definition = false;
    const std::uint32_t first { spec.first };
    Local l;
    l.first = first;
    l.flags = spec.flags;
    Declarator d { declarator(spec.has_type ? spec.type : TypeId {}, Mode::named, site) };
    if (failed_) return add(l);
    if (!d.ok || !d.name) {
        fail("expected a declarator");
        return add(l);
    }
    // `f(x);` could declare x of the type f; a name nothing knows as a type is a call.
    if (tentative_declaration_ && d.parenthesized_name && (is(Kind::semi) || is(Kind::comma)) && spec.has_type &&
        tree_.types[spec.type.index].kind == TypeKind::named && !type_like(last_type_class_) && !type_template(last_type_class_)) {
        fail("looks like a call");
        return add(l);
    }
    l.name = d.name;
    l.name_token = d.name_token;
    l.type = d.type;
    if (d.pack) l.flags |= Local::pack;
    const Name& name { tree_.names[d.name.index] };
    const NameComponent& last { tree_.components[name.components.begin + name.components.count - 1] };
    const bool in_class { site == Site::class_ };
    if (d.function) {
        l.kind = LocalKind::function;
        const std::string_view owner { in_class && !class_names_.empty() ? class_names_.back() : std::string_view {} };
        if (last.form == NameComponent::Form::destructor) l.entity = msa::Kind::destructor;
        else if (last.form == NameComponent::Form::conversion_function) l.entity = msa::Kind::conversion;
        else if (!spec.has_type && last.form == NameComponent::Form::identifier && !owner.empty() && spelled(last.token) == owner) l.entity = msa::Kind::constructor;
        else if (!spec.has_type && name.components.count > 1 && last.form == NameComponent::Form::identifier &&
                 spelled(tree_.components[name.components.begin + name.components.count - 2].token) == spelled(last.token))
            l.entity = msa::Kind::constructor;
        else l.entity = in_class || name.components.count > 1 ? msa::Kind::method : msa::Kind::function;
    } else if ((l.flags & Local::typedef_) != 0) {
        l.kind = LocalKind::type_alias;
        l.entity = msa::Kind::type_alias;
    } else if (in_class && (l.flags & Local::static_) == 0) {
        l.kind = LocalKind::field;
        l.entity = msa::Kind::field;
    } else {
        l.kind = LocalKind::variable;
        l.entity = msa::Kind::variable;
    }
    if (site == Site::condition || site == Site::for_init || site == Site::exception) {
        if (site == Site::condition) l.flags |= Local::in_condition;
    }
    skip_attributes();
    // GNU asm label: `int x asm("name");`
    if ((word("asm") || word("__asm__") || word("__asm")) && is(Kind::l_paren, 1)) {
        next();
        skip_balanced();
    }
    skip_attributes();
    bind_declared(l);
    if (d.function) {
        // a function: its tail (requires, contract specifiers, virt-specifiers), then its body; the parameters are visible in it
        push_scope();
        for (const auto h : tree_.list(d.params))
            if (tree_.locals[h.index].name_token != NONE) declare_name(tree_.locals[h.index].name_token, NameClass::value);
        struct Pop {
            BodyParser& p;
            ~Pop() { p.pop_scope(); }
        } pop_tail { *this };
        for (;;) {
            if (word("override")) {
                l.flags |= Local::override_;
                next();
            } else if (word("final")) {
                l.flags |= Local::final_;
                next();
            } else if (word("requires")) {
                next();
                l.constraint = constraint_expression();
            } else if (ident() && is(Kind::l_paren, 1) && !extensions_.empty()) {
                FunctionSpecifiers specs;
                bool taken { false };
                for (const auto& ext : extensions_)
                    if (ext->function_specifier(*this, specs)) {
                        taken = true;
                        break;
                    }
                if (!taken) break;
                for (const auto c : specs.contracts) d.contracts.push_back(c);
            } else break;
            if (failed_) return add(l);
        }
        std::vector<Handle> params;
        if (d.params.count != 0) params.assign(tree_.links.begin() + d.params.begin, tree_.links.begin() + d.params.begin + d.params.count);
        l.params = {};
        l.members = emit(params);
        if (!d.contracts.empty()) {
            // The function's contract specifiers in a run of Tree::contracts (copies: a nested lambda's may sit between).
            l.contracts = { static_cast<std::uint32_t>(tree_.contracts.size()), static_cast<std::uint32_t>(d.contracts.size()) };
            for (const auto c : d.contracts) tree_.contracts.push_back(tree_.contracts[c]);
        }
        const LocalId id { add(l) };
        if (is(Kind::l_brace) || is(Kind::colon) || word("try") || (is(Kind::equal) && (word("default", 1) || word("delete", 1) || (is(Kind::numeric_constant, 1))))) {
            *definition = !is(Kind::equal);
            const StmtId body { function_body(id) };
            tree_.locals[id.index].body = body;
            tree_.locals[id.index].last = prev();
            if (is(Kind::equal) == false && *definition) { /* a definition ends the declaration */ }
            if (tree_.statements[body.index].has(Stmt::defaulted)) tree_.locals[id.index].flags |= Local::defaulted;
            if (tree_.statements[body.index].has(Stmt::deleted)) tree_.locals[id.index].flags |= Local::deleted;
            if (tree_.statements[body.index].has(Stmt::pure)) tree_.locals[id.index].flags |= Local::pure;
            if (tree_.statements[body.index].e1) tree_.locals[id.index].flags |= Local::deleted_with_reason;
            *definition = tree_.statements[body.index].a.index != NONE;
            // `= default;` `= delete;` `= 0;` are followed by their `;` or a comma, as a declaration.
            return link_outline(id);
        }
        tree_.locals[id.index].last = prev();
        return link_outline(id);
    }
    // a variable, a field, a typedef: bit-field width, initializer
    if (is(Kind::colon) && in_class) {
        next();
        Nested nest { *this };
        l.width = conditional_expression();
    }
    const LocalId id { add(l) };
    initializer_of(tree_.locals[id.index], site);
    tree_.locals[id.index].last = prev();
    return link_outline(id);
}

// `= e`, `{ ... }`, `( ... )` after a declarator.
void BodyParser::initializer_of(Local& l, Site site) {
    const std::size_t index { static_cast<std::size_t>(&l - tree_.locals.data()) };
    const auto set = [&](ExprId init, std::uint32_t flag) {
        tree_.locals[index].init = init;
        tree_.locals[index].flags |= flag;
    };
    if (is(Kind::equal)) {
        next();
        Nested nest { *this, false, site == Site::template_parameter ? 1 : 0 };
        const ExprId init { initializer_clause() };
        set(init, Local::copy_init);
    } else if (is(Kind::l_brace)) {
        Nested nest { *this };
        const ExprId init { braced_init_list() };
        set(init, Local::brace_init);
    } else if (is(Kind::l_paren) && site != Site::parameter) {
        const std::uint32_t first { here() };
        next();
        Nested nest { *this };
        Expr list;
        list.kind = ExprKind::init_list;
        list.first = first;
        list.list = argument_list(Kind::r_paren);
        list.last = prev();
        set(add(list), Local::direct_init);
    }
}

// ---- function bodies ----

// The body of a function after its declarator and tail: constructor initializers, a block, a function-try-block,
// or `= default`, `= delete`, `= delete("why")`, `= 0`. `owner`: the function whose parameters are visible.
StmtId BodyParser::function_body(LocalId owner) {
    const std::uint32_t first { here() };
    Stmt body;
    body.kind = StmtKind::function_body;
    body.first = first;
    push_scope();
    if (owner) {
        const Local& fn { tree_.locals[owner.index] };
        for (const auto h : tree_.list(fn.members))
            if (tree_.locals[h.index].name_token != NONE) declare_name(tree_.locals[h.index].name_token, NameClass::value);
    }
    struct Pop {
        BodyParser& p;
        ~Pop() { p.pop_scope(); }
    } pop { *this };
    if (is(Kind::equal)) {
        next();
        if (word("default")) {
            next();
            body.flags |= Stmt::defaulted;
        } else if (word("delete")) {
            next();
            body.flags |= Stmt::deleted;
            if (is(Kind::l_paren)) {
                // `= delete("why")`: the reason is the extension's (C++26), a string-literal.
                bool taken { false };
                ExprId reason;
                for (const auto& ext : extensions_)
                    if (ext->delete_reason(*this, reason)) {
                        taken = true;
                        break;
                    }
                if (!taken) fail("expected `;` after `= delete`");
                body.e1 = reason;
            }
        } else if (is(Kind::numeric_constant)) {
            next();
            body.flags |= Stmt::pure;
        } else {
            fail("expected `default`, `delete` or `0` after `=`");
        }
        body.last = prev();
        return add(body);
    }
    std::vector<Handle> inits;
    const bool try_block { word("try") };
    const std::uint32_t try_first { here() };
    if (try_block) next();
    if (is(Kind::colon)) {
        next();
        while (!failed_ && !eof() && !is(Kind::l_brace)) {
            const std::uint32_t at { here() };
            Expr init;
            init.kind = ExprKind::member_init;
            init.first = at;
            init.name = qualified_name(NameUse::type);
            if (failed_) break;
            if (is(Kind::l_brace)) {
                const ExprId list { braced_init_list() };
                init.flags |= Expr::brace;
                init.list = tree_.expressions[list.index].list;
            } else if (is(Kind::l_paren)) {
                next();
                Nested nest { *this };
                init.list = argument_list(Kind::r_paren);
            } else {
                fail("expected `(` or `{` in a mem-initializer");
                break;
            }
            if (accept(Kind::ellipsis)) init.flags |= Expr::pack;
            init.last = prev();
            inits.push_back(add(init).handle());
            if (!accept(Kind::comma)) break;
        }
    }
    body.list = emit(inits);
    if (failed_) return add(body);
    if (!is(Kind::l_brace)) {
        fail("expected the function's body");
        return add(body);
    }
    StmtId block { committed_compound() };
    if (try_block) {
        // function-try-block: try [: inits] { } catch (...) { }
        std::vector<Handle> handlers;
        while (!failed_ && word("catch")) handlers.push_back(handler(here()).handle());
        Stmt t;
        t.kind = StmtKind::try_;
        t.first = try_first;
        t.last = prev();
        t.a = block;
        t.flags |= Stmt::function_try;
        t.list = emit(handlers);
        block = add(t);
    }
    body.a = block;
    body.last = prev();
    return add(body);
}

// ---- classes, enums ----

void BodyParser::read_base_clause(Local& l) {
    // `: [virtual] [access] name [...], ...`
    std::vector<Base> bases;
    for (;;) {
        Base b;
        skip_attributes();
        for (;;) {
            if (word("virtual")) {
                b.virtual_ = true;
                next();
            } else if (word("public")) {
                b.access = 1;
                next();
            } else if (word("protected")) {
                b.access = 2;
                next();
            } else if (word("private")) {
                b.access = 3;
                next();
            } else break;
        }
        const std::uint32_t first { here() };
        // The base is a class name (with arguments) or a decltype/typename specifier.
        TypeNode t;
        t.first = first;
        if (word("decltype") && is(Kind::l_paren, 1)) {
            b.type = decltype_type();
        } else {
            if (word("typename")) next();
            t.kind = TypeKind::named;
            t.name = qualified_name(NameUse::type);
            t.last = prev();
            if (failed_) return;
            b.type = add(t);
        }
        if (accept(Kind::ellipsis)) b.pack = true;
        bases.push_back(b);
        if (!accept(Kind::comma)) break;
    }
    l.bases = { static_cast<std::uint32_t>(tree_.bases.size()), static_cast<std::uint32_t>(bases.size()) };
    tree_.bases.insert(tree_.bases.end(), bases.begin(), bases.end());
}

// class-key [attrs] [name] [final] [: bases] [{ members }] -- a definition, a forward declaration, or an elaborated-type-specifier.
LocalId BodyParser::class_specifier(DeclSpec& spec, Site site) {
    const std::uint32_t first { here() };
    const std::string_view key { tok().spelling };
    const msa::Kind kind { key == "union" ? msa::Kind::union_ : key == "struct" ? msa::Kind::struct_ : msa::Kind::class_ };
    const std::uint32_t key_token { here() };
    next();
    skip_attributes();
    NameId name;
    if (ident() || is(Kind::coloncolon)) {
        name = qualified_name(NameUse::type);
        if (failed_) return {};
    }
    while (word("final") || word("sealed") || word("abstract")) next();
    skip_attributes();
    const bool definition { is(Kind::l_brace) || (is(Kind::colon) && !is(Kind::coloncolon)) };
    TypeNode type;
    type.kind = TypeKind::elaborated;
    type.first = first;
    type.token = key_token;
    type.name = name;
    if (!definition) {
        // A reference (`struct S *p`) or a forward declaration (`struct S;`).
        type.last = prev();
        if (is(Kind::semi) && name && site != Site::parameter && (spec.flags & Local::friend_) == 0) {
            Local l;
            l.kind = LocalKind::class_;
            l.entity = kind;
            l.first = first;
            l.name = name;
            l.name_token = tree_.components[tree_.names[name.index].components.begin + tree_.names[name.index].components.count - 1].token;
            l.last = prev();
            const LocalId id { link_outline(add(l)) };
            bind_declared(tree_.locals[id.index]);
            type.local = id;
            spec.defined = id;
            spec.defined_kind = kind;
        }
        spec.type = add(type);
        spec.has_type = true;
        return spec.defined;
    }
    Local l;
    l.kind = LocalKind::class_;
    l.entity = kind;
    l.first = first;
    l.name = name;
    if (name) l.name_token = tree_.components[tree_.names[name.index].components.begin + tree_.names[name.index].components.count - 1].token;
    if (is(Kind::colon)) {
        next();
        read_base_clause(l);
        if (failed_) return {};
    }
    const LocalId id { link_outline(add(l)) };
    if (name) bind_declared(tree_.locals[id.index]);
    if (!is(Kind::l_brace)) {
        fail("expected `{` after a class's name");
        return id;
    }
    next();
    class_names_.push_back(name ? spelled(tree_.locals[id.index].name_token) : std::string_view {});
    push_scope();
    const std::vector<LocalId> members { member_specification_until_brace(Site::class_) };
    pop_scope();
    class_names_.pop_back();
    expect(Kind::r_brace, "`}`");
    std::vector<Handle> handles;
    for (const auto m : members) handles.push_back(m.handle());
    tree_.locals[id.index].members = emit(handles);
    tree_.locals[id.index].last = prev();
    type.local = id;
    type.last = prev();
    spec.type = add(type);
    spec.has_type = true;
    spec.defined = id;
    spec.defined_kind = kind;
    return id;
}

// Members until the closing `}` (not consumed).
std::vector<LocalId> BodyParser::member_specification_until_brace(Site site) {
    std::vector<LocalId> members;
    while (!failed_ && !eof() && !is(Kind::r_brace)) {
        const std::size_t before { i_ };
        member_declaration(members, site);
        if (!failed_ && i_ == before) {
            fail(std::format("unexpected `{}` in a class", tok().spelling));
        }
    }
    return members;
}

LocalId BodyParser::member_declaration(std::vector<LocalId>& out, Site site) {
    skip_attributes();
    const std::uint32_t first { here() };
    if (site == Site::class_ && (word("public") || word("private") || word("protected")) && is(Kind::colon, 1)) {
        Local l;
        l.kind = LocalKind::access;
        l.first = first;
        l.form = word("public") ? 1 : word("protected") ? 2 : 3;
        advance(2);
        l.last = prev();
        out.push_back(add(l));
        return out.back();
    }
    bool definition { false };
    const std::vector<LocalId> made { simple_declaration(site, &definition) };
    for (const auto m : made) out.push_back(m);
    // A function's body ends the declaration: a `;` after it is an empty declaration.
    if (definition && is(Kind::semi)) next();
    return made.empty() ? LocalId {} : made.back();
}

LocalId BodyParser::enum_specifier(DeclSpec& spec, Site site) {
    const std::uint32_t first { here() };
    next();   // enum
    Local l;
    l.kind = LocalKind::enum_;
    l.entity = msa::Kind::enum_;
    l.first = first;
    if (word("class") || word("struct")) {
        l.flags |= Local::scoped;
        next();
    }
    skip_attributes();
    NameId name;
    if (ident() || is(Kind::coloncolon)) {
        name = qualified_name(NameUse::type);
        if (failed_) return {};
    }
    l.name = name;
    if (name) l.name_token = tree_.components[tree_.names[name.index].components.begin + tree_.names[name.index].components.count - 1].token;
    skip_attributes();
    if (is(Kind::colon)) {   // the underlying type
        next();
        const DeclSpec under { decl_specifiers(Site::parameter) };
        l.type = under.type;
    }
    TypeNode type;
    type.kind = TypeKind::elaborated;
    type.first = first;
    type.token = first;
    type.name = name;
    if (!is(Kind::l_brace)) {
        // `enum E;`, `enum class E : int;` (opaque), or `enum E e;` (a reference).
        type.last = prev();
        if (is(Kind::semi) && name && site != Site::parameter && (l.type || (l.flags & Local::scoped) != 0)) {
            l.last = prev();
            const LocalId id { link_outline(add(l)) };
            bind_declared(tree_.locals[id.index]);
            type.local = id;
            spec.type = add(type);
            spec.has_type = true;
            spec.defined = id;
            return id;
        }
        spec.type = add(type);
        spec.has_type = true;
        return {};
    }
    next();
    const LocalId id { link_outline(add(l)) };
    if (name) bind_declared(tree_.locals[id.index]);
    std::vector<Handle> enumerators;
    // Enumerators are visible in the enumeration's scope and, unscoped, in the enclosing one: here, as values.
    while (!failed_ && !is(Kind::r_brace) && !eof()) {
        skip_attributes();
        if (!ident()) {
            fail("expected an enumerator");
            break;
        }
        Local e;
        e.kind = LocalKind::enumerator;
        e.entity = msa::Kind::enumerator;
        e.first = here();
        e.name_token = here();
        next();
        skip_attributes();
        if (accept(Kind::equal)) {
            Nested nest { *this };
            e.init = conditional_expression();
            e.flags |= Local::copy_init;
        }
        e.last = prev();
        if ((tree_.locals[id.index].flags & Local::scoped) == 0) declare_name(e.name_token, NameClass::value);
        enumerators.push_back(link_outline(add(e)).handle());
        if (!accept(Kind::comma)) break;
    }
    expect(Kind::r_brace, "`}`");
    tree_.locals[id.index].members = emit(enumerators);
    tree_.locals[id.index].last = prev();
    type.local = id;
    type.last = prev();
    spec.type = add(type);
    spec.has_type = true;
    spec.defined = id;
    return id;
}

// ---- using, static_assert, aliases ----

LocalId BodyParser::using_declaration(Site site) {
    const std::uint32_t first { here() };
    next();   // using
    Local l;
    l.first = first;
    if (word("namespace")) {
        next();
        l.kind = LocalKind::using_directive;
        l.name = qualified_name(NameUse::expression);
        expect(Kind::semi, "`;`");
        l.last = prev();
        return add(l);
    }
    if (word("enum")) {
        next();
        l.kind = LocalKind::using_enum;
        const DeclSpec spec { decl_specifiers(Site::parameter) };
        l.type = spec.type;
        expect(Kind::semi, "`;`");
        l.last = prev();
        return add(l);
    }
    skip_attributes();
    if (ident() && (is(Kind::equal, 1) || (is(Kind::l_square, 1) && is(Kind::l_square, 2)))) return alias_declaration(first, false);
    // using [typename] name {, name} ;
    l.kind = LocalKind::using_declaration;
    for (;;) {
        accept_word("typename");
        const NameId n { qualified_name(NameUse::expression) };
        if (failed_) return add(l);
        if (!l.name) l.name = n;
        accept(Kind::ellipsis);
        if (l.name_token == NONE && n) {
            const Name& name { tree_.names[n.index] };
            l.name_token = tree_.components[name.components.begin + name.components.count - 1].token;
            // What it names is what the name it brings in is: a type, a function, a value -- when something knows.
            declare_name(l.name_token, classify_name(n, first).what);
        }
        if (!accept(Kind::comma)) break;
    }
    expect(Kind::semi, "`;`");
    (void)site;
    l.last = prev();
    return add(l);
}

// `using X [[attrs]] = type;` at the cursor on X.
LocalId BodyParser::alias_declaration(std::uint32_t first, bool) {
    Local l;
    l.kind = LocalKind::type_alias;
    l.entity = msa::Kind::type_alias;
    l.first = first;
    l.name_token = here();
    next();
    skip_attributes();
    expect(Kind::equal, "`=`");
    if (failed_) return add(l);
    l.type = type_id();
    skip_attributes();
    expect(Kind::semi, "`;`");
    l.last = prev();
    declare_name(l.name_token, NameClass::type);
    return link_outline(add(l));
}

LocalId BodyParser::static_assert_declaration() {
    const std::uint32_t first { here() };
    next();
    Local l;
    l.kind = LocalKind::static_assert_;
    l.first = first;
    if (!expect(Kind::l_paren, "`(`")) return add(l);
    {
        Nested nest { *this };
        l.init = conditional_expression();
        if (accept(Kind::comma)) l.width = conditional_expression();
    }
    expect(Kind::r_paren, "`)`");
    expect(Kind::semi, "`;`");
    l.last = prev();
    return add(l);
}

LocalId BodyParser::namespace_alias() {
    const std::uint32_t first { here() };
    next();   // namespace
    Local l;
    l.kind = LocalKind::namespace_alias;
    l.entity = msa::Kind::namespace_alias;
    l.first = first;
    l.name_token = here();
    next();
    expect(Kind::equal, "`=`");
    const std::uint32_t at { here() };
    const NameId target { qualified_name(NameUse::expression) };
    Expr e;
    e.kind = ExprKind::id;
    e.first = at;
    e.last = prev();
    e.name = target;
    l.init = add(e);
    expect(Kind::semi, "`;`");
    l.last = prev();
    declare_name(l.name_token, NameClass::namespace_);
    return link_outline(add(l));
}

LocalId BodyParser::asm_declaration() {
    const std::uint32_t first { here() };
    Local l;
    l.kind = LocalKind::asm_;
    l.first = first;
    next();
    while (word("volatile") || word("__volatile__") || word("goto") || word("inline")) next();
    if (is(Kind::l_paren)) skip_balanced();
    else fail("expected `(` after asm");
    expect(Kind::semi, "`;`");
    l.last = prev();
    return add(l);
}

// `auto [a, b] = e;`, `auto& [k, v] : m`: one group, its names the bindings. The cursor at `&`, `&&` or `[`.
LocalId BodyParser::structured_binding(const DeclSpec& spec, Site site) {
    Local group;
    group.kind = LocalKind::structured_binding;
    group.first = spec.first;
    group.flags = spec.flags;
    group.type = spec.type;
    if (accept(Kind::amp)) group.flags |= Local::by_ref;
    else if (accept(Kind::ampamp)) group.flags |= Local::by_rvalue_ref;
    next();   // [
    std::vector<Handle> names;
    while (!failed_ && !is(Kind::r_square) && !eof()) {
        Local b;
        b.kind = LocalKind::binding;
        b.entity = msa::Kind::variable;
        b.first = here();
        if (accept(Kind::ellipsis)) {
            b.flags |= Local::pack;
            group.flags |= Local::pack;
        }
        if (!ident()) {
            fail("expected a name in a structured binding");
            break;
        }
        b.name_token = here();
        next();
        skip_attributes();
        b.last = prev();
        declare_name(b.name_token, NameClass::value, (b.flags & Local::pack) != 0);
        names.push_back(link_outline(add(b)).handle());
        if (!accept(Kind::comma)) break;
    }
    expect(Kind::r_square, "`]`");
    group.members = emit(names);
    if (site == Site::condition) group.flags |= Local::in_condition;
    const LocalId id { add(group) };
    initializer_of(tree_.locals[id.index], site);
    tree_.locals[id.index].last = prev();
    return id;
}

// ---- templates ----

Span32 BodyParser::template_parameter_list(bool* ok) {
    // After `template`: `<` parameters `>`; the parameters are declared as dependent names.
    *ok = false;
    std::vector<Handle> params;
    if (!expect(Kind::less, "`<`")) return {};
    Nested nest { *this, false, 1 };
    while (!failed_ && !at_gt() && !eof()) {
        params.push_back(template_parameter().handle());
        if (!accept(Kind::comma)) break;
    }
    nest.restore();
    if (failed_) return {};
    if (!expect_gt()) return {};
    *ok = true;
    return emit(params);
}

LocalId BodyParser::template_parameter() {
    Local p;
    p.kind = LocalKind::template_parameter;
    p.entity = msa::Kind::template_parameter;
    p.first = here();
    skip_attributes();
    NameClass what { NameClass::value };
    if (word("template") && is(Kind::less, 1)) {
        // template <...> class|typename name [= id-expression]
        next();
        bool ok { false };
        p.params = template_parameter_list(&ok);
        if (!ok) return add(p);
        if (!(word("class") || word("typename") || word("concept"))) {
            fail("expected `class` after a template template parameter's list");
            return add(p);
        }
        next();
        p.form = 2;
        what = NameClass::class_template;
        if (accept(Kind::ellipsis)) p.flags |= Local::pack;
        if (ident()) {
            p.name_token = here();
            next();
        }
        if (accept(Kind::equal)) {
            const std::uint32_t at { here() };
            const NameId n { qualified_name(NameUse::type) };
            Expr e;
            e.kind = ExprKind::id;
            e.first = at;
            e.last = prev();
            e.name = n;
            p.init = add(e);
        }
    } else if (word("class") || word("typename")) {
        next();
        p.form = 0;
        what = NameClass::type;
        if (accept(Kind::ellipsis)) p.flags |= Local::pack;
        if (ident()) {
            p.name_token = here();
            next();
        }
        if (accept(Kind::equal)) p.type = type_id();   // the default type
    } else {
        // A constrained type parameter (`Concept T`) or a non-type parameter (`int N`): decided by the first name.
        bool constrained { false };
        if (ident() || is(Kind::coloncolon)) {
            const Mark m { mark() };
            ++speculating_;
            const bool was_failed { std::exchange(failed_, false) };
            const NameId n { qualified_name(NameUse::type) };
            const bool parsed { !failed_ && n };
            if (parsed) {
                const NameInfo info { classify_name(n, here()) };
                const bool followed { ident() || is(Kind::ellipsis) || is(Kind::comma) || at_gt() || is(Kind::equal) };
                const auto& parts { tree_.parts(tree_.names[n.index]) };
                // A concept, or an unknown name with arguments followed by a plain name: `std::integral T`.
                constrained = followed && (info.what == NameClass::concept_ || (info.what == NameClass::unknown && !parts.empty() &&
                                                                             (is(Kind::ellipsis) || ident()) && !is_known_type_name(info)));
            }
            failed_ = was_failed;
            --speculating_;
            rewind(m);
        }
        if (constrained) {
            p.form = 0;
            what = NameClass::type;
            const NameId concept_name { qualified_name(NameUse::type) };
            TypeNode placeholder;
            placeholder.kind = TypeKind::named;
            placeholder.name = concept_name;
            placeholder.first = p.first;
            placeholder.last = prev();
            p.type = {};
            (void)placeholder;
            if (accept(Kind::ellipsis)) p.flags |= Local::pack;
            if (ident()) {
                p.name_token = here();
                next();
            }
            if (accept(Kind::equal)) p.type = type_id();
        } else {
            p.form = 1;
            const DeclSpec spec { decl_specifiers(Site::template_parameter) };
            if (!spec.has_type) {
                fail("expected a template parameter");
                return add(p);
            }
            const Declarator d { declarator(spec.type, Mode::either, Site::template_parameter) };
            p.type = d.type ? d.type : spec.type;
            p.name = d.name;
            p.name_token = d.name_token;
            if (d.pack) p.flags |= Local::pack;
            if (accept(Kind::equal)) {
                p.flags |= Local::copy_init;
                p.init = conditional_expression();
            }
        }
    }
    p.last = prev();
    if (p.name_token != NONE) declare_name(p.name_token, what, true);
    return link_outline(add(p));
}

bool BodyParser::is_known_type_name(const NameInfo& info) const { return type_like(info.what) || type_template(info.what); }

LocalId BodyParser::template_declaration_inner(Site site) {
    const std::uint32_t first { here() };
    next();   // template
    Local t;
    t.kind = LocalKind::template_;
    t.first = first;
    if (!is(Kind::less)) {
        // template class X<int>;  explicit instantiation
        bool definition { false };
        const std::vector<LocalId> inner { simple_declaration(site, &definition) };
        if (!inner.empty()) t.target = inner.back();
        t.last = prev();
        return add(t);
    }
    bool ok { false };
    t.params = template_parameter_list(&ok);
    if (!ok) return add(t);
    if (word("requires")) {
        next();
        t.constraint = constraint_expression();
    }
    if (word("concept") && ident(1)) {
        Local c;
        c.kind = LocalKind::concept_;
        c.entity = msa::Kind::concept_;
        c.first = first;
        next();
        c.name_token = here();
        next();
        skip_attributes();
        expect(Kind::equal, "`=`");
        {
            Nested nest { *this };
            c.init = expression();
        }
        expect(Kind::semi, "`;`");
        c.last = prev();
        const LocalId id { link_outline(add(c)) };
        scope_.push_back({ spelled(tree_.locals[id.index].name_token), NameClass::concept_, false });
        t.target = id;
        t.last = prev();
        return add(t);
    }
    if (word("using")) {
        const std::uint32_t at { here() };
        next();
        skip_attributes();
        t.target = alias_declaration(at, true);
        t.last = prev();
        return add(t);
    }
    bool definition { false };
    const std::vector<LocalId> inner { simple_declaration(site, &definition) };
    if (!inner.empty()) t.target = inner.back();
    t.last = prev();
    return add(t);
}

// template < parameters > declaration: the parameters visible to it alone; its name, a template, to what follows.
LocalId BodyParser::template_declaration(Site site) {
    push_scope();
    const LocalId id { template_declaration_inner(site) };
    pop_scope();
    if (id && !failed_) {
        const Local& t { tree_.locals[id.index] };
        if (t.target) {
            const Local& made { tree_.locals[t.target.index] };
            if (made.name_token != NONE) {
                NameClass what { NameClass::value };
                switch (made.kind) {
                case LocalKind::class_: what = NameClass::class_template; break;
                case LocalKind::type_alias: what = NameClass::alias_template; break;
                case LocalKind::function: what = NameClass::function_template; break;
                case LocalKind::variable: case LocalKind::field: what = NameClass::variable_template; break;
                case LocalKind::concept_: what = NameClass::concept_; break;
                default: break;
                }
                declare_name(made.name_token, what);
            }
        }
    }
    return id;
}

// ---- conditions ----

// A condition that declares: `T x = e`, `auto [a, b] = e`, `T x{e}`. The cursor at its start; none when it is not one.
LocalId BodyParser::condition_declaration(Site site) {
    const DeclSpec spec { decl_specifiers(site) };
    if (failed_ || !spec.has_type) {
        fail("expected a declaration");
        return {};
    }
    if (spec.placeholder && (is(Kind::l_square) || ((is(Kind::amp) || is(Kind::ampamp)) && is(Kind::l_square, 1))))
        return structured_binding(spec, site);
    bool definition { false };
    return init_declarator(spec, site, true, &definition);
}

} // namespace mcxx::frontend::bodies
