// Statements: blocks, the control statements, labels, and what could be a declaration or an expression --
// told apart by trying the declaration and keeping it when it is one a declaration could be (and, for a
// name nothing knows, when it does not read as a call).
module mcxx.frontend;

import std;
import mcxx.msa;
import mcxx.base;
import :bodyparser;

namespace mcxx::frontend::bodies {

// ---- the statement, with recovery ----

StmtId BodyParser::statement() {
    if (failed_) return error_stmt(here(), prev());
    const std::uint32_t first { here() };
    if (eof()) {
        fail("expected a statement");
        return error_stmt(first, first);
    }
    if (++depth_ > 300) {
        --depth_;
        fail("statements nested too deeply");
        return error_stmt(first, first);
    }
    struct Depth {
        int& n;
        ~Depth() { --n; }
    } depth_guard { depth_ };
    const StmtId s { statement_inner() };
    if (!failed_) return s;
    if (speculating_ > 0) return s;   // the attempt will rewind
    report_failure();
    recover_to_statement_end();
    failed_ = false;
    return error_stmt(first, prev());
}

StmtId BodyParser::compound_statement() {
    const std::uint32_t first { here() };
    if (!expect(Kind::l_brace, "`{`")) return error_stmt(first, first);
    push_scope();
    std::vector<Handle> body;
    while (!eof() && !is(Kind::r_brace)) {
        const std::size_t before { i_ };
        const StmtId s { statement() };
        body.push_back(s.handle());
        if (failed_) break;
        if (i_ == before) {   // no progress: skip the token, the diagnostic is recorded
            diagnostic(std::format("unexpected `{}`", tok().spelling), i_);
            part_failed_ = true;
            next();
        }
    }
    pop_scope();
    if (!failed_) expect(Kind::r_brace, "`}`");
    Stmt s;
    s.kind = StmtKind::compound;
    s.first = first;
    s.last = prev();
    s.list = emit(body);
    return add(s);
}

// A block that is read as what it is whatever is being tried around it: a lambda's body inside a declaration
// that may turn out an expression. Its statements recover from their own failures; a rewind discards what it noted.
StmtId BodyParser::committed_compound() {
    const int saved { std::exchange(speculating_, 0) };
    const StmtId s { compound_statement() };
    speculating_ = saved;
    return s;
}

StmtId BodyParser::attributed(std::uint32_t first, StmtId inner, std::uint32_t attr_first, std::uint32_t attr_last) {
    Stmt s;
    s.kind = StmtKind::attributed;
    s.first = first;
    s.last = prev();
    s.a = inner;
    const StmtId id { add(s) };
    tree_.attributes.push_back({ attr_first, attr_last, id.handle() });
    return id;
}

StmtId BodyParser::statement_inner() {
    const std::uint32_t first { here() };
    std::uint32_t attr_first, attr_last;
    skip_attributes(attr_first, attr_last);
    if (attr_first != NONE) {
        // [[likely]] statement; [[fallthrough]];
        const StmtId inner { statement() };
        return attributed(first, inner, attr_first, attr_last);
    }
    if (is(Kind::l_brace)) return compound_statement();
    if (is(Kind::semi)) {
        next();
        return add(Stmt { .first = first, .last = first, .kind = StmtKind::null });
    }
    if (tok().kind == Kind::raw_identifier) {
        const std::string_view w { tok().spelling };
        // Syntax a later generation of the language adds: template for, contract_assert, consteval { }.
        if ((w == "template" || w == "contract_assert" || w == "consteval") && !extensions_.empty()) {
            StmtId out;
            for (const auto& ext : extensions_)
                if (ext->statement(*this, out)) return out;
        }
        if (w == "if") return if_statement(first);
        if (w == "switch") return switch_statement(first);
        if (w == "while") return while_statement(first);
        if (w == "do") return do_statement(first);
        if (w == "for") return for_loop(first, false);
        if (w == "return") return return_statement(first);
        if (w == "co_return") return return_statement(first);
        if (w == "break" || w == "continue") {
            next();
            expect(Kind::semi, "`;`");
            Stmt s;
            s.kind = w == "break" ? StmtKind::break_ : StmtKind::continue_;
            s.first = first;
            s.last = prev();
            return add(s);
        }
        if (w == "goto") {
            next();
            Stmt s;
            s.kind = StmtKind::goto_;
            s.first = first;
            if (is(Kind::star)) {
                next();
                s.e1 = expression();
            } else if (ident()) {
                s.token = here();
                next();
            } else {
                fail("expected a label after `goto`");
            }
            expect(Kind::semi, "`;`");
            s.last = prev();
            return add(s);
        }
        if (w == "try") return try_statement(first);
        if (w == "case" || w == "default") {
            Stmt s;
            s.first = first;
            next();
            if (w == "case") {
                s.kind = StmtKind::case_;
                Nested nest { *this };
                s.e1 = conditional_expression();
                if (accept(Kind::ellipsis)) s.e2 = conditional_expression();
            } else {
                s.kind = StmtKind::default_;
            }
            expect(Kind::colon, "`:`");
            if (!failed_ && !is(Kind::r_brace) && !eof()) s.a = statement();
            s.last = prev();
            return add(s);
        }
        if (w == "asm" || w == "__asm__" || w == "__asm") {
            if (is(Kind::l_paren, 1) || word("volatile", 1) || word("__volatile__", 1) || word("goto", 1) || word("inline", 1)) return asm_statement(first);
        }
        if (w == "static_assert" || w == "_Static_assert") {
            const LocalId l { static_assert_declaration() };
            Stmt s;
            s.kind = StmtKind::static_assert_;
            s.first = first;
            s.last = prev();
            if (!failed_) {
                s.e1 = tree_.locals[l.index].init;
                s.e2 = tree_.locals[l.index].width;
            }
            return add(s);
        }
        // a label
        if (ident() && is(Kind::colon, 1) && !is(Kind::coloncolon, 1)) {
            Stmt s;
            s.kind = StmtKind::labeled;
            s.first = first;
            s.token = first;
            advance(2);
            if (!failed_ && !is(Kind::r_brace) && !eof()) s.a = statement();
            s.last = prev();
            return add(s);
        }
    }
    return declaration_or_expression_statement();
}

// ---- declaration or expression ----

// The statement's reading is a declaration when a declaration could be read from it: tried, and kept when it is one
// that does not read as a call (a name no one knows, a parenthesized name after it).
StmtId BodyParser::declaration_or_expression_statement() {
    const std::uint32_t first { here() };
    if (is_declaration_start(Site::block)) return declaration_statement();
    if (try_declaration_statement(first, Site::block)) return last_declaration_;
    return expression_statement();
}

bool BodyParser::try_declaration_statement(std::uint32_t first, Site site) {
    // What cannot begin a declaration: not a name, not a type keyword, not `::`.
    if (eof()) return false;
    const bool name_start { ident() || is(Kind::coloncolon) };
    const bool keyword_start { tok().kind == Kind::raw_identifier && (at_type_start(false) || word("decltype") || word("template")) };
    if (!name_start && !keyword_start) return false;
    // A first name the scopes know as a value begins an expression.
    if (ident() && !is(Kind::coloncolon, 1) && !is(Kind::less, 1)) {
        if (const Entry* e { scope_entry(spelled(here())) }; e != nullptr && e->what == NameClass::value) {
            note(Ambiguity::declaration_or_expression, Resolution::expression, ast::Basis::scope, first);
            return false;
        }
    }
    StmtId result;
    last_type_class_ = NameClass::unknown;
    last_type_basis_ = ast::Basis::syntax;
    const bool saved { std::exchange(tentative_declaration_, true) };
    const bool ok { attempt([&] {
        bool definition { false };
        const std::vector<LocalId> locals { simple_declaration(site, &definition) };
        if (failed_) return;
        Stmt s;
        s.kind = StmtKind::declaration;
        s.first = first;
        s.last = prev();
        std::vector<Handle> handles;
        for (const auto l : locals) handles.push_back(l.handle());
        s.list = emit(handles);
        result = add(s);
    }) };
    tentative_declaration_ = saved;
    if (!ok) return false;
    last_declaration_ = result;
    note(Ambiguity::declaration_or_expression, Resolution::declaration, keyword_start ? ast::Basis::syntax : last_type_basis_, first);
    return true;
}

StmtId BodyParser::expression_statement() {
    const std::uint32_t first { here() };
    const ExprId e { expression() };
    if (failed_) return error_stmt(first, prev());
    expect(Kind::semi, "`;`");
    Stmt s;
    s.kind = StmtKind::expression;
    s.first = first;
    s.last = prev();
    s.e1 = e;
    return add(s);
}

StmtId BodyParser::asm_statement(std::uint32_t first) {
    Stmt s;
    s.kind = StmtKind::asm_;
    s.first = first;
    next();
    while (word("volatile") || word("__volatile__") || word("goto") || word("inline")) {
        if (word("volatile") || word("__volatile__")) s.flags |= Stmt::volatile_;
        next();
    }
    if (is(Kind::l_paren)) skip_balanced();
    else fail("expected `(` after asm");
    expect(Kind::semi, "`;`");
    s.last = prev();
    return add(s);
}

// ---- control statements ----

// A condition: a declaration with an initializer (`T x = e`, `auto [a, b] = e`) or an expression. The cursor at its start.
bool BodyParser::condition(ExprId& expr, LocalId& local) {
    if (try_condition_declaration(local)) return true;
    if (failed_) return false;
    Nested nest { *this };
    expr = expression();
    return !failed_;
}

bool BodyParser::try_condition_declaration(LocalId& local) {
    if (eof()) return false;
    const std::uint32_t first { here() };
    const bool name_start { ident() || is(Kind::coloncolon) };
    const bool keyword_start { tok().kind == Kind::raw_identifier && (at_type_start(false) || word("decltype")) };
    if (!name_start && !keyword_start) return false;
    if (ident() && !is(Kind::coloncolon, 1) && !is(Kind::less, 1)) {
        if (const Entry* e { scope_entry(spelled(here())) }; e != nullptr && e->what == NameClass::value) return false;
    }
    LocalId made;
    const bool saved { std::exchange(tentative_declaration_, true) };
    last_type_basis_ = ast::Basis::syntax;
    const bool ok { attempt([&] {
        made = condition_declaration(Site::condition);
        if (failed_) return;
        // A condition's declaration has an initializer, braced or after `=`.
        if (!made || !tree_.locals[made.index].init) fail("a condition's declaration needs an initializer");
        // and the condition ends with it: `a && g(x) == n` is not `a&& g(x)` followed by more.
        else if (!is(Kind::r_paren) && !is(Kind::semi)) fail("a condition's declaration ends the condition");
    }) };
    tentative_declaration_ = saved;
    if (!ok) return false;
    local = made;
    note(Ambiguity::declaration_or_expression, Resolution::declaration, keyword_start ? ast::Basis::syntax : last_type_basis_, first);
    return true;
}

// `if ( [init-statement] condition )`: the init-statement when a `;` closes something before the `)`.
bool BodyParser::init_before_condition(StmtId& init) {
    // A `;` at depth 0 inside the parentheses (the cursor after the `(`).
    int depth { 0 };
    bool has_init { false };
    for (std::size_t k { i_ }; k < end_; ++k) {
        const Kind kd { t_[k].kind };
        if (kd == Kind::l_paren || kd == Kind::l_square || kd == Kind::l_brace) ++depth;
        else if (kd == Kind::r_paren || kd == Kind::r_square || kd == Kind::r_brace) {
            if (depth == 0) break;
            --depth;
        } else if (kd == Kind::semi && depth == 0) {
            has_init = true;
            break;
        }
    }
    if (!has_init) return true;
    const std::uint32_t first { here() };
    if (is_declaration_start(Site::condition) || !try_declaration_statement(first, Site::block)) {
        if (is_declaration_start(Site::condition)) init = declaration_statement();
        else init = expression_statement();
    } else {
        init = last_declaration_;
    }
    return !failed_;
}

StmtId BodyParser::if_statement(std::uint32_t first) {
    next();   // if
    Stmt s;
    s.kind = StmtKind::if_;
    s.first = first;
    push_scope();
    struct Pop {
        BodyParser& p;
        ~Pop() { p.pop_scope(); }
    } pop { *this };
    bool negated { false };
    if (is(Kind::exclaim) && word("consteval", 1)) {
        negated = true;
        next();
    }
    if (word("constexpr")) {
        s.flags |= Stmt::constexpr_;
        next();
    } else if (word("consteval")) {
        s.flags |= Stmt::consteval_;
        if (negated) s.flags |= Stmt::negated;
        next();
        if (!is(Kind::l_brace)) fail("expected `{` after `if consteval`");
        s.b = failed_ ? StmtId {} : statement();
        if (word("else")) {
            next();
            s.c = statement();
        }
        s.last = prev();
        return add(s);
    }
    if (!expect(Kind::l_paren, "`(`")) return error_stmt(first, prev());
    Nested nest { *this };
    if (!init_before_condition(s.a)) return error_stmt(first, prev());
    if (!condition(s.e1, s.local)) return error_stmt(first, prev());
    nest.restore();
    if (!expect(Kind::r_paren, "`)`")) return error_stmt(first, prev());
    s.b = statement();
    if (word("else")) {
        next();
        s.c = statement();
    }
    s.last = prev();
    return add(s);
}

StmtId BodyParser::switch_statement(std::uint32_t first) {
    next();
    Stmt s;
    s.kind = StmtKind::switch_;
    s.first = first;
    push_scope();
    struct Pop {
        BodyParser& p;
        ~Pop() { p.pop_scope(); }
    } pop { *this };
    if (!expect(Kind::l_paren, "`(`")) return error_stmt(first, prev());
    Nested nest { *this };
    if (!init_before_condition(s.a) || !condition(s.e1, s.local)) return error_stmt(first, prev());
    nest.restore();
    if (!expect(Kind::r_paren, "`)`")) return error_stmt(first, prev());
    s.b = statement();
    s.last = prev();
    return add(s);
}

StmtId BodyParser::while_statement(std::uint32_t first) {
    next();
    Stmt s;
    s.kind = StmtKind::while_;
    s.first = first;
    push_scope();
    struct Pop {
        BodyParser& p;
        ~Pop() { p.pop_scope(); }
    } pop { *this };
    if (!expect(Kind::l_paren, "`(`")) return error_stmt(first, prev());
    Nested nest { *this };
    if (!condition(s.e1, s.local)) return error_stmt(first, prev());
    nest.restore();
    if (!expect(Kind::r_paren, "`)`")) return error_stmt(first, prev());
    s.b = statement();
    s.last = prev();
    return add(s);
}

StmtId BodyParser::do_statement(std::uint32_t first) {
    next();
    Stmt s;
    s.kind = StmtKind::do_;
    s.first = first;
    s.a = statement();
    if (failed_) return error_stmt(first, prev());
    if (!accept_word("while")) {
        fail("expected `while` after the body of `do`");
        return error_stmt(first, prev());
    }
    if (!expect(Kind::l_paren, "`(`")) return error_stmt(first, prev());
    {
        Nested nest { *this };
        s.e1 = expression();
    }
    expect(Kind::r_paren, "`)`");
    expect(Kind::semi, "`;`");
    s.last = prev();
    return add(s);
}

StmtId BodyParser::return_statement(std::uint32_t first) {
    const bool coroutine { word("co_return") };
    next();
    Stmt s;
    s.kind = coroutine ? StmtKind::co_return_ : StmtKind::return_;
    s.first = first;
    if (!is(Kind::semi) && !eof()) {
        Nested nest { *this };
        s.e1 = initializer_clause();
        // The comma operator is part of the operand: `return a, b;`.
        while (!failed_ && is(Kind::comma)) {
            next();
            const ExprId rhs { assignment_expression() };
            Expr e;
            e.kind = ExprKind::binary;
            e.op = Op::comma;
            e.first = tree_.expressions[s.e1.index].first;
            e.last = prev();
            e.a = s.e1;
            e.b = rhs;
            s.e1 = add(e);
        }
    }
    expect(Kind::semi, "`;`");
    s.last = prev();
    return add(s);
}

StmtId BodyParser::try_statement(std::uint32_t first) {
    next();
    Stmt s;
    s.kind = StmtKind::try_;
    s.first = first;
    s.a = compound_statement();
    std::vector<Handle> handlers;
    while (!failed_ && word("catch")) handlers.push_back(handler(here()).handle());
    if (handlers.empty() && !failed_) fail("expected a handler after a try block");
    s.list = emit(handlers);
    s.last = prev();
    return add(s);
}

StmtId BodyParser::handler(std::uint32_t first) {
    next();   // catch
    Stmt s;
    s.kind = StmtKind::handler;
    s.first = first;
    push_scope();
    struct Pop {
        BodyParser& p;
        ~Pop() { p.pop_scope(); }
    } pop { *this };
    if (!expect(Kind::l_paren, "`(`")) return error_stmt(first, prev());
    if (is(Kind::ellipsis)) next();
    else {
        Nested nest { *this };
        const DeclSpec spec { decl_specifiers(Site::exception) };
        if (!spec.has_type) {
            fail("expected an exception declaration");
            return error_stmt(first, prev());
        }
        const Declarator d { declarator(spec.type, Mode::either, Site::exception) };
        Local l;
        l.kind = LocalKind::variable;
        l.entity = msa::Kind::variable;
        l.first = spec.first;
        l.type = d.type ? d.type : spec.type;
        l.name = d.name;
        l.name_token = d.name_token;
        l.last = prev();
        l.flags = spec.flags;
        if (l.name_token != NONE) declare_name(l.name_token, NameClass::value);
        s.local = link_outline(add(l));
    }
    if (!expect(Kind::r_paren, "`)`")) return error_stmt(first, prev());
    s.a = compound_statement();
    s.last = prev();
    return add(s);
}

// for (init; cond; inc) body, and for (init? decl : range) body.
StmtId BodyParser::for_statement(bool expansion, std::uint32_t first) { return for_loop(first, expansion); }

StmtId BodyParser::for_loop(std::uint32_t first, bool expansion) {
    next();   // for
    Stmt s;
    s.first = first;
    push_scope();
    struct Pop {
        BodyParser& p;
        ~Pop() { p.pop_scope(); }
    } pop { *this };
    if (!expect(Kind::l_paren, "`(`")) return error_stmt(first, prev());
    // What is in the parentheses: two `;` make a classic for; a `:` that is not a conditional's (after at most
    // one `;`) a range-based one.
    int depth { 0 }, question { 0 }, semicolons { 0 };
    bool range { false };
    for (std::size_t k { i_ }; k < end_; ++k) {
        const Kind kd { t_[k].kind };
        if (kd == Kind::l_paren || kd == Kind::l_square || kd == Kind::l_brace) ++depth;
        else if (kd == Kind::r_paren || kd == Kind::r_square || kd == Kind::r_brace) {
            if (depth == 0) break;
            --depth;
        } else if (depth == 0) {
            if (kd == Kind::semi) {
                if (++semicolons == 2) break;
            } else if (kd == Kind::question) ++question;
            else if (kd == Kind::colon) {
                if (question > 0) --question;
                else {
                    range = true;
                    break;
                }
            }
        }
    }
    Nested nest { *this };
    if (range) {
        s.kind = StmtKind::range_for;
        if (expansion) s.flags |= Stmt::expansion;
        if (semicolons == 1) {   // C++20: for (init; decl : range)
            const std::uint32_t at { here() };
            if (is_declaration_start(Site::for_init)) s.a = declaration_statement();
            else if (!try_declaration_statement(at, Site::block)) s.a = expression_statement();
            else s.a = last_declaration_;
            if (failed_) return error_stmt(first, prev());
        }
        const DeclSpec spec { decl_specifiers(Site::for_init) };
        if (failed_ || !spec.has_type) {
            if (!failed_) fail("expected a declaration in a range-based for");
            return error_stmt(first, prev());
        }
        if (spec.placeholder && (is(Kind::l_square) || ((is(Kind::amp) || is(Kind::ampamp)) && is(Kind::l_square, 1)))) {
            s.local = structured_binding(spec, Site::for_init);
        } else {
            const Declarator d { declarator(spec.type, Mode::named, Site::for_init) };
            Local l;
            l.kind = LocalKind::variable;
            l.entity = msa::Kind::variable;
            l.first = spec.first;
            l.type = d.type ? d.type : spec.type;
            l.name = d.name;
            l.name_token = d.name_token;
            l.flags = spec.flags;
            l.last = prev();
            if (l.name_token != NONE) declare_name(l.name_token, NameClass::value);
            s.local = link_outline(add(l));
        }
        if (!expect(Kind::colon, "`:` in a range-based for")) return error_stmt(first, prev());
        s.e1 = is(Kind::l_brace) ? braced_init_list() : expression();
        nest.restore();
        if (!expect(Kind::r_paren, "`)`")) return error_stmt(first, prev());
        s.b = statement();
        s.last = prev();
        return add(s);
    }
    s.kind = StmtKind::for_;
    // init-statement (it ends with its `;`)
    if (is(Kind::semi)) {
        const std::uint32_t at { here() };
        next();
        s.a = add(Stmt { .first = at, .last = at, .kind = StmtKind::null });
    } else {
        const std::uint32_t at { here() };
        if (is_declaration_start(Site::for_init)) s.a = declaration_statement();
        else if (try_declaration_statement(at, Site::block)) s.a = last_declaration_;
        else s.a = expression_statement();
    }
    if (failed_) return error_stmt(first, prev());
    if (!is(Kind::semi)) {
        if (!condition(s.e1, s.local)) return error_stmt(first, prev());
    }
    if (!expect(Kind::semi, "`;`")) return error_stmt(first, prev());
    if (!is(Kind::r_paren)) s.e2 = expression();
    nest.restore();
    if (!expect(Kind::r_paren, "`)`")) return error_stmt(first, prev());
    s.b = statement();
    s.last = prev();
    return add(s);
}

} // namespace mcxx::frontend::bodies
