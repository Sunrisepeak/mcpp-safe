// C++26's syntax as a language extension (MC4, the plan's §4): what plugins/lang/cpp26 will supply to the
// front end. The core reads C++23; every construct below is read here, through the SyntaxContext the core
// offers an extension, and none of them is named in the core's parsing.
//
//   P2996 reflection      ^^ operand, [: expr :] (an expression, a type, a nested-name prefix)
//   P1306 expansion       template for (...) body
//   P2900 contracts       pre(cond), post(r: cond) after a declarator; contract_assert(cond);
//   P2662 pack indexing   pack...[n], as an expression and as a type
//   P2573 delete reason   = delete("why")
//   consteval blocks      consteval { ... } as a statement and a declaration
//
// Their feature ids are those of the catalog's C++26 package (the table of MC1's `c++26:` category).
module mcxx.frontend;

import std;
import mcxx.msa;
import mcxx.base;

namespace mcxx::frontend {

namespace {

using namespace ast;

constexpr std::string_view IDS[] {
    "c++26:reflection", "c++26:expansion-statements", "c++26:contracts", "c++26:pack-indexing", "c++26:delete-with-reason", "c++26:consteval-blocks",
};

class Cpp26Syntax final : public SyntaxExtension {
public:
    std::string_view name() const override { return "mcxx.plugins.lang.cpp26"; }
    std::span<const std::string_view> features() const override { return IDS; }

    // `^^ operand` and `[: constant-expression :]`.
    bool expression_primary(SyntaxContext& c, ExprId& out) const override {
        const std::uint32_t first { c.position() };
        if (c.token().kind == Kind::caret && c.token(1).kind == Kind::caret && c.adjacent(1)) {
            c.advance(2);
            Expr e;
            e.kind = ExprKind::reflect;
            e.first = first;
            std::vector<Handle> operand;
            // `^^::` is the global namespace: no operand.
            if (c.token().kind == Kind::coloncolon && !(c.token(1).kind == Kind::raw_identifier)) c.advance();
            else operand.push_back(c.type_or_expression());
            e.last = c.position() - 1;
            e.list = c.list(operand);
            out = c.make(e);
            return true;
        }
        if (is_splice_open(c)) {
            Expr e;
            e.kind = ExprKind::splice;
            e.first = first;
            e.a = splice_operand(c);
            e.last = c.position() - 1;
            out = c.make(e);
            return true;
        }
        return false;
    }

    // `lhs ... [ n ]`.
    bool expression_postfix(SyntaxContext& c, ExprId& lhs) const override {
        if (c.token().kind != Kind::ellipsis || c.token(1).kind != Kind::l_square) return false;
        const std::uint32_t first { c.position() };
        (void)first;
        c.advance(2);
        Expr e;
        e.kind = ExprKind::pack_index;
        e.a = lhs;
        e.b = c.constant_expression();
        c.expect(Kind::r_square, "`]`");
        e.last = c.position() - 1;
        e.first = c.first_token(lhs.handle());
        lhs = c.make(e);
        return true;
    }

    bool statement(SyntaxContext& c, StmtId& out) const override {
        const std::uint32_t first { c.position() };
        const PpToken& t { c.token() };
        if (t.kind != Kind::raw_identifier) return false;
        if (t.spelling == "template" && c.token(1).kind == Kind::raw_identifier && c.token(1).spelling == "for") {
            c.advance();   // template
            out = c.for_statement(true, first);
            return true;
        }
        if (t.spelling == "contract_assert" && c.token(1).kind == Kind::l_paren) {
            c.advance(2);
            Stmt s;
            s.kind = StmtKind::contract_assert;
            s.first = first;
            s.e1 = c.assignment_expression();
            c.expect(Kind::r_paren, "`)`");
            c.expect(Kind::semi, "`;`");
            s.last = c.position() - 1;
            out = c.make(s);
            return true;
        }
        if (t.spelling == "consteval" && c.token(1).kind == Kind::l_brace) {
            c.advance();
            Stmt s;
            s.kind = StmtKind::consteval_block;
            s.first = first;
            s.a = c.compound_statement();
            s.last = c.position() - 1;
            out = c.make(s);
            return true;
        }
        return false;
    }

    // `[: r :]` and `typename [: r :]` as a type.
    bool type_specifier(SyntaxContext& c, TypeId& out) const override {
        const std::uint32_t first { c.position() };
        const bool keyword { c.token().kind == Kind::raw_identifier && c.token().spelling == "typename" };
        const std::size_t open { keyword ? 1u : 0u };
        if (!(c.token(open).kind == Kind::l_square && (c.token(open + 1).kind == Kind::colon || c.token(open + 1).kind == Kind::coloncolon) && c.adjacent(open + 1)))
            return false;
        if (keyword) c.advance();
        TypeNode t;
        t.kind = TypeKind::splice;
        t.first = first;
        t.expr = splice_operand(c);
        t.last = c.position() - 1;
        out = c.make(t);
        return true;
    }

    // `T ...[ n ]` as a type.
    bool type_postfix(SyntaxContext& c, TypeId& lhs) const override {
        if (c.token().kind != Kind::ellipsis || c.token(1).kind != Kind::l_square) return false;
        c.advance(2);
        TypeNode t;
        t.kind = TypeKind::pack_index;
        t.expr = c.constant_expression();
        c.expect(Kind::r_square, "`]`");
        t.last = c.position() - 1;
        t.first = c.first_token(lhs.handle());
        t.inner = lhs;
        lhs = c.make(t);
        return true;
    }

    // `[: r :] ::` as the start of a nested-name-specifier.
    bool name_component(SyntaxContext& c, NameComponent& out) const override {
        if (!is_splice_open(c)) return false;
        out.form = NameComponent::Form::splice;
        out.token = c.position();
        out.expr = splice_operand(c);
        out.last = c.position() - 1;
        return true;
    }

    // `pre ( cond )`, `post ( [r :] cond )`.
    bool function_specifier(SyntaxContext& c, FunctionSpecifiers& specs) const override {
        const PpToken& t { c.token() };
        if (t.kind != Kind::raw_identifier || (t.spelling != "pre" && t.spelling != "post") || c.token(1).kind != Kind::l_paren) return false;
        Contract contract;
        contract.first = c.position();
        contract.post = t.spelling == "post";
        c.advance(2);
        c.push_scope();
        // post(r: cond): `r` names the result, for the condition.
        if (contract.post && c.token().kind == Kind::raw_identifier && c.token(1).kind == Kind::colon) {
            contract.result = c.position();
            c.declare(contract.result, NameClass::value);
            c.advance(2);
        }
        contract.condition = c.assignment_expression();
        c.pop_scope();
        c.expect(Kind::r_paren, "`)`");
        contract.last = c.position() - 1;
        specs.contracts.push_back(c.contract(contract));
        return !c.failed();
    }

    // `consteval { ... }` as a declaration.
    bool declaration(SyntaxContext& c, LocalId& out) const override {
        if (!(c.token().kind == Kind::raw_identifier && c.token().spelling == "consteval" && c.token(1).kind == Kind::l_brace)) return false;
        Local l;
        l.kind = LocalKind::consteval_block;
        l.first = c.position();
        c.advance();
        l.body = c.compound_statement();
        l.last = c.position() - 1;
        out = c.make(l);
        return true;
    }

    // `= delete(` "why" `)`: the cursor at the `(`.
    bool delete_reason(SyntaxContext& c, ExprId& out) const override {
        if (c.token().kind != Kind::l_paren) return false;
        c.advance();
        out = c.assignment_expression();
        c.expect(Kind::r_paren, "`)`");
        return !c.failed();
    }

private:
    static bool is_splice_open(SyntaxContext& c) {
        return c.token().kind == Kind::l_square && (c.token(1).kind == Kind::colon || c.token(1).kind == Kind::coloncolon) && c.adjacent(1) &&
               !(c.token(1).kind == Kind::colon && c.token(2).kind == Kind::colon);
    }

    // `[:` constant-expression `:]`: the expression.
    static ExprId splice_operand(SyntaxContext& c) {
        c.advance(2);   // [ :
        const ExprId e { c.constant_expression() };
        if (c.failed()) return e;
        if (c.token().kind == Kind::colon && c.token(1).kind == Kind::r_square && c.adjacent(1)) c.advance(2);
        else c.fail("expected `:]` to close a splice");
        return e;
    }

};

} // namespace

std::shared_ptr<const SyntaxExtension> cpp26_syntax() {
    static const std::shared_ptr<const SyntaxExtension> instance { std::make_shared<Cpp26Syntax>() };
    return instance;
}

} // namespace mcxx::frontend
