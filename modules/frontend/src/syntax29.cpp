// C++29's syntax as a language extension, apart from C++26's (MC4, the plan's §4: each generation is its own package;
// this is what plugins/lang/cpp29 will supply). The core reads C++23; every construct below is read here, through the
// SyntaxContext the core offers an extension.
//
//   pack indexing of template names   Ts...[0]<int>, as an expression (a pack of templates, indexed, with arguments)
//   conditional noexcept in a         { e } noexcept(cond) -> C;
//   compound requirement
//
// Their feature ids are those of the catalog's C++29 package.
module mcxx.frontend;

import std;
import mcxx.msa;
import mcxx.base;

namespace mcxx::frontend {

namespace {

using namespace ast;

constexpr std::string_view IDS[] { "c++29:pack-indexing-template-names", "c++29:conditional-noexcept-specifiers-compound-requirements" };

class Cpp29Syntax final : public SyntaxExtension {
public:
    std::string_view name() const override { return "mcxx.plugins.lang.cpp29"; }
    std::span<const std::string_view> features() const override { return IDS; }

    // After `pack...[n]`: template arguments, when they read as such (`Ts...[0]<int>(x)`), else a comparison.
    bool expression_postfix(SyntaxContext& c, ExprId& lhs) const override {
        if (c.token().kind != Kind::less) return false;
        const Expr* node { c.expression_node(lhs) };
        if (node == nullptr || node->kind != ExprKind::pack_index || node->has(Expr::template_arguments)) return false;
        Span32 args;
        if (!c.try_template_arguments(args)) return false;
        Expr e { *node };
        e.list = args;
        e.flags |= Expr::template_arguments;
        e.last = c.position() - 1;
        lhs = c.make(e);
        return true;
    }

    // `noexcept ( cond )` of a compound requirement, the cursor at the `(`.
    bool noexcept_condition(SyntaxContext& c, ExprId& out) const override {
        if (c.token().kind != Kind::l_paren) return false;
        c.advance();
        out = c.constant_expression();
        c.expect(Kind::r_paren, "`)`");
        return !c.failed();
    }
};

} // namespace

std::shared_ptr<const SyntaxExtension> cpp29_syntax() {
    static const std::shared_ptr<const SyntaxExtension> instance { std::make_shared<Cpp29Syntax>() };
    return instance;
}

} // namespace mcxx::frontend
