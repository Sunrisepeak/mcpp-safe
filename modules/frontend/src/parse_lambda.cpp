// Lambda-expressions and requires-expressions: constructs that bring their own scope of names into an
// expression.
module mcxx.frontend;

import std;
import mcxx.msa;
import mcxx.base;
import :bodyparser;

namespace mcxx::frontend::bodies {

// [captures] <template-params> requires (params) specifiers -> ret requires { body }
ExprId BodyParser::lambda_expression() {
    const std::uint32_t first { here() };
    LambdaInfo info;
    next();   // [
    Nested nest { *this };
    push_scope();
    struct Pop {
        BodyParser& p;
        ~Pop() { p.pop_scope(); }
    } pop { *this };
    std::vector<Handle> captures;
    while (!failed_ && !is(Kind::r_square) && !eof()) {
        Local c;
        c.kind = LocalKind::capture;
        c.entity = msa::Kind::variable;
        c.first = here();
        if ((is(Kind::equal) || is(Kind::amp)) && (is(Kind::comma, 1) || is(Kind::r_square, 1))) {
            c.form = 3;
            c.flags |= Local::default_capture;
            if (is(Kind::amp)) c.flags |= Local::by_ref;
            info.capture_default = is(Kind::amp) ? 2 : 1;
            next();
        } else if (word("this")) {
            c.form = 1;
            next();
        } else if (is(Kind::star) && word("this", 1)) {
            c.form = 2;
            advance(2);
        } else {
            if (accept(Kind::amp)) c.flags |= Local::by_ref;
            if (accept(Kind::ellipsis)) c.flags |= Local::pack;   // `...x = e`
            if (!ident()) {
                fail("expected a capture");
                break;
            }
            c.name_token = here();
            next();
            if (accept(Kind::ellipsis)) c.flags |= Local::pack;
            // An init-capture: `x = e`, `x{e}`, `x(e)`: visible in the body, not in its own initializer.
            if (is(Kind::equal)) {
                next();
                c.flags |= Local::copy_init;
                c.init = assignment_expression();
            } else if (is(Kind::l_brace)) {
                c.flags |= Local::brace_init;
                c.init = braced_init_list();
            } else if (is(Kind::l_paren)) {
                const std::uint32_t at { here() };
                next();
                Expr list;
                list.kind = ExprKind::paren_list;
                list.first = at;
                list.list = argument_list(Kind::r_paren);
                list.last = prev();
                c.flags |= Local::direct_init;
                c.init = add(list);
            }
        }
        c.last = prev();
        captures.push_back(add(c).handle());
        if (!accept(Kind::comma)) break;
    }
    if (!failed_) expect(Kind::r_square, "`]`");
    // The captures are visible in the body: the init-captures' names, as values.
    for (const auto h : captures) {
        const Local& c { tree_.locals[h.index] };
        if (c.name_token != NONE) declare_name(c.name_token, NameClass::value);
    }
    info.captures = emit(captures);
    // Template parameters, a requires-clause after them.
    if (!failed_ && is(Kind::less)) {
        bool ok { false };
        info.template_params = template_parameter_list(&ok);
        if (!ok) return error_expr(first);
        if (word("requires")) {
            next();
            info.requires_ = constraint_expression();
        }
    }
    // The function type: parameters, specifiers, noexcept, trailing return.
    TypeNode fn;
    fn.kind = TypeKind::function;
    fn.first = here();
    fn.inner = {};
    TypeId function { add(fn) };
    std::vector<std::uint32_t> contracts;
    if (!failed_ && is(Kind::l_paren)) {
        info.has_params = true;
        next();
        bool variadic { false };
        // The parameters' names stay visible in the rest of the lambda.
        push_scope();
        std::vector<Handle> params;
        {
            Nested inner { *this };
            while (!failed_ && !is(Kind::r_paren) && !eof()) {
                if (is(Kind::ellipsis)) {
                    next();
                    variadic = true;
                    break;
                }
                const LocalId p { parameter_declaration() };
                if (failed_) break;
                if (tree_.locals[p.index].name_token != NONE) declare_name(tree_.locals[p.index].name_token, NameClass::value);
                params.push_back(p.handle());
                if (!accept(Kind::comma)) break;
            }
        }
        if (!failed_) expect(Kind::r_paren, "`)`");
        // Left in the scope pushed above: popped with the lambda's.
        scope_marks_.pop_back();   // the mark of the parameters' scope: its names stay in the lambda's
        info.params = emit(params);
        tree_.types[function.index].list = info.params;
        if (variadic) tree_.types[function.index].flags |= TypeNode::c_variadic;
    }
    // mutable, constexpr, consteval, static, attributes, noexcept, `-> type`, contracts
    for (;;) {
        if (failed_) break;
        skip_attributes();
        if (word("mutable")) {
            info.mutable_ = true;
            next();
        } else if (word("constexpr")) {
            info.constexpr_ = true;
            next();
        } else if (word("consteval")) {
            info.consteval_ = true;
            next();
        } else if (word("static") && !is(Kind::l_brace)) {
            info.static_ = true;
            next();
        } else break;
    }
    function_qualifiers(function, &contracts);
    // A lambda with no parameter list, specifiers or return type has no tokens of a function type: its range is the one before the body.
    tree_.types[function.index].last = prev();
    if (tree_.types[function.index].last < tree_.types[function.index].first) tree_.types[function.index].first = tree_.types[function.index].last;
    info.function = function;
    skip_attributes();
    if (!failed_ && word("requires")) {
        next();
        info.trailing_requires = constraint_expression();
    }
    if (failed_) return error_expr(first);
    if (!is(Kind::l_brace)) {
        fail("expected the lambda's body");
        return error_expr(first);
    }
    info.body = committed_compound();
    tree_.lambdas.push_back(info);
    Expr e;
    e.kind = ExprKind::lambda;
    e.first = first;
    e.last = prev();
    e.aux = static_cast<std::uint32_t>(tree_.lambdas.size() - 1);
    return add(e);
}

// requires [(params)] { requirement; ... }
ExprId BodyParser::requires_expression() {
    const std::uint32_t first { here() };
    next();   // requires
    RequiresInfo info;
    Nested nest { *this };
    push_scope();
    struct Pop {
        BodyParser& p;
        ~Pop() { p.pop_scope(); }
    } pop { *this };
    if (is(Kind::l_paren)) {
        next();
        info.has_params = true;
        bool variadic { false };
        std::vector<Handle> params;
        while (!failed_ && !is(Kind::r_paren) && !eof()) {
            if (is(Kind::ellipsis)) {
                next();
                variadic = true;
                break;
            }
            const LocalId p { parameter_declaration() };
            if (failed_) break;
            if (tree_.locals[p.index].name_token != NONE) declare_name(tree_.locals[p.index].name_token, NameClass::value);
            params.push_back(p.handle());
            if (!accept(Kind::comma)) break;
        }
        info.c_variadic = variadic;
        if (!failed_) expect(Kind::r_paren, "`)`");
        info.params = emit(params);
    }
    if (!expect(Kind::l_brace, "`{`")) return error_expr(first);
    std::vector<Requirement> requirements;
    while (!failed_ && !is(Kind::r_brace) && !eof()) {
        Requirement r;
        r.first = here();
        if (word("typename")) {
            r.form = Requirement::Form::type;
            const std::uint32_t at { here() };
            next();
            TypeNode t;
            t.kind = TypeKind::named;
            t.first = at;
            t.name = qualified_name(NameUse::type);
            t.last = prev();
            if (failed_) break;
            r.type = add(t);
        } else if (is(Kind::l_brace)) {
            r.form = Requirement::Form::compound;
            next();
            {
                Nested inner { *this };
                r.expr = expression();
            }
            if (!expect(Kind::r_brace, "`}`")) break;
            if (accept_word("noexcept")) r.noexcept_ = true;
            if (accept(Kind::arrow)) {
                // `-> type-constraint`: a concept (with arguments) naming the expression's type, or a type.
                const Mark m { mark() };
                bool as_constraint { false };
                {
                    ++speculating_;
                    const bool was_failed { std::exchange(failed_, false) };
                    const NameId n { qualified_name(NameUse::type) };
                    if (!failed_ && n && (is(Kind::semi))) {
                        r.constraint = n;
                        as_constraint = true;
                    }
                    failed_ = was_failed;
                    --speculating_;
                }
                if (!as_constraint) {
                    rewind(m);
                    r.type = type_id();
                }
            }
        } else if (word("requires")) {
            r.form = Requirement::Form::nested;
            next();
            r.expr = binary_expression(3);   // a constraint-expression: a logical-or-expression, `requires sizeof(T) > 1;`
        } else {
            r.form = Requirement::Form::simple;
            r.expr = expression();
        }
        if (failed_) break;
        if (!expect(Kind::semi, "`;`")) break;
        r.last = prev();
        requirements.push_back(r);
    }
    if (!failed_) expect(Kind::r_brace, "`}`");
    info.requirements = { static_cast<std::uint32_t>(tree_.requirements.size()), static_cast<std::uint32_t>(requirements.size()) };
    tree_.requirements.insert(tree_.requirements.end(), requirements.begin(), requirements.end());
    tree_.requires_infos.push_back(info);
    Expr e;
    e.kind = ExprKind::requires_;
    e.first = first;
    e.last = prev();
    e.aux = static_cast<std::uint32_t>(tree_.requires_infos.size() - 1);
    return add(e);
}

} // namespace mcxx::frontend::bodies
