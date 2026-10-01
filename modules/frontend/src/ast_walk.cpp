// Walking the tree: what a node's children are, whatever its kind. One place says which slots of which kinds
// hold nodes, so a pass that visits every node does not repeat it (and a new slot is added here once).
module mcxx.frontend;

import std;
import mcxx.msa;

namespace mcxx::frontend::ast {

namespace {

struct Collector {
    const Tree& t;
    std::vector<Handle> out;

    template <Sort S>
    void one(Id<S> id) {
        if (id) out.push_back(id.handle());
    }
    void list(Span32 s) {
        for (const auto h : t.list(s)) out.push_back(h);
    }

    void expression(const Expr& e) {
        switch (e.kind) {
        case ExprKind::id: case ExprKind::sizeof_pack: one(e.name); break;
        case ExprKind::paren: case ExprKind::unary: case ExprKind::noexcept_: case ExprKind::throw_: case ExprKind::co_await_: case ExprKind::co_yield_:
        case ExprKind::delete_: case ExprKind::pack_expansion: case ExprKind::splice:
            one(e.a);
            break;
        case ExprKind::binary: case ExprKind::assign: case ExprKind::pack_index: one(e.a), one(e.b); break;
        case ExprKind::conditional: one(e.a), one(e.b), one(e.c); break;
        case ExprKind::call: case ExprKind::subscript: one(e.a), list(e.list); break;
        case ExprKind::member: one(e.a), one(e.name); break;
        case ExprKind::cast_named: case ExprKind::cast_c: one(e.type), one(e.a); break;
        case ExprKind::cast_functional: one(e.type), list(e.list); break;
        case ExprKind::sizeof_: case ExprKind::typeid_: e.type ? one(e.type) : one(e.a); break;
        case ExprKind::new_: {
            const News& n { t.news[e.aux] };
            list(n.placement);
            one(n.type), one(n.bound), one(n.init);
            break;
        }
        case ExprKind::lambda: {
            const LambdaInfo& l { t.lambdas[e.aux] };
            list(l.captures), list(l.template_params), one(l.requires_), one(l.function), one(l.trailing_requires), one(l.body);
            break;
        }
        case ExprKind::requires_: {
            const RequiresInfo& r { t.requires_infos[e.aux] };
            list(r.params);
            for (std::uint32_t i { 0 }; i < r.requirements.count; ++i) {
                const Requirement& q { t.requirements[r.requirements.begin + i] };
                one(q.expr), one(q.type), one(q.constraint);
            }
            break;
        }
        case ExprKind::fold: {
            const FoldInfo& f { t.folds[e.aux] };
            one(f.pattern), one(f.init);
            break;
        }
        case ExprKind::init_list: case ExprKind::builtin: case ExprKind::reflect: list(e.list); break;
        case ExprKind::designated:
            for (std::uint32_t i { 0 }; i < e.list.count; ++i) {
                const Designator& d { t.designators[e.aux + i] };
                one(d.index), one(d.end);
            }
            one(e.a);
            break;
        case ExprKind::statement_expression: one(e.stmt); break;
        case ExprKind::member_init: one(e.name), list(e.list); break;
        default: break;
        }
    }

    void statement(const Stmt& s) {
        switch (s.kind) {
        case StmtKind::compound: case StmtKind::declaration: list(s.list); break;
        case StmtKind::expression: case StmtKind::return_: case StmtKind::co_return_: case StmtKind::contract_assert: one(s.e1); break;
        case StmtKind::if_: case StmtKind::switch_: one(s.a), s.local ? one(s.local) : one(s.e1), one(s.b), one(s.c); break;
        case StmtKind::while_: s.local ? one(s.local) : one(s.e1), one(s.b); break;
        case StmtKind::do_: one(s.a), one(s.e1); break;
        case StmtKind::for_: one(s.a), s.local ? one(s.local) : one(s.e1), one(s.e2), one(s.b); break;
        case StmtKind::range_for: one(s.a), one(s.local), one(s.e1), one(s.b); break;
        case StmtKind::goto_: one(s.e1); break;
        case StmtKind::labeled: case StmtKind::default_: case StmtKind::consteval_block: case StmtKind::attributed: one(s.a); break;
        case StmtKind::case_: one(s.e1), one(s.e2), one(s.a); break;
        case StmtKind::try_: one(s.a), list(s.list); break;
        case StmtKind::handler: one(s.local), one(s.a); break;
        case StmtKind::static_assert_: one(s.e1), one(s.e2); break;
        case StmtKind::function_body: list(s.list), one(s.e1), one(s.a); break;
        default: break;
        }
    }

    void type(const TypeNode& n) {
        switch (n.kind) {
        case TypeKind::elaborated: one(n.name), one(n.local); break;
        case TypeKind::named: one(n.name); break;
        case TypeKind::decltype_: one(n.expr); break;
        case TypeKind::auto_: one(n.name); break;
        case TypeKind::pointer: case TypeKind::lvalue_ref: case TypeKind::rvalue_ref: case TypeKind::pack_expansion: case TypeKind::atomic: one(n.inner); break;
        case TypeKind::member_pointer: one(n.name), one(n.inner); break;
        case TypeKind::array: one(n.inner), one(n.expr); break;
        case TypeKind::function: one(n.inner), list(n.list), one(n.expr); break;
        case TypeKind::pack_index: one(n.inner), one(n.name), one(n.expr); break;
        case TypeKind::splice: one(n.expr); break;
        case TypeKind::typeof_: one(n.inner), one(n.expr); break;
        default: break;
        }
    }

    void local(const Local& l) {
        one(l.name), one(l.type), one(l.init), one(l.width);
        list(l.params);
        for (std::uint32_t i { 0 }; i < l.bases.count; ++i) one(t.bases[l.bases.begin + i].type);
        one(l.constraint);
        if (l.kind == LocalKind::template_) one(l.target);
        else list(l.members);
        for (std::uint32_t i { 0 }; i < l.contracts.count; ++i) one(t.contracts[l.contracts.begin + i].condition);
        one(l.body);
    }

    void name(const Name& n) {
        for (const auto& c : t.parts(n)) {
            list(c.arguments);
            one(c.type), one(c.expr);
        }
    }
};

} // namespace

std::vector<Handle> children(const Tree& tree, Handle h) {
    Collector c { tree, {} };
    switch (h.sort) {
    case Sort::expression: if (const auto* e { tree.expression(h) }) c.expression(*e); break;
    case Sort::statement: if (const auto* s { tree.statement(h) }) c.statement(*s); break;
    case Sort::type: if (const auto* t { tree.type(h) }) c.type(*t); break;
    case Sort::local: if (const auto* l { tree.local(h) }) c.local(*l); break;
    case Sort::name: if (const auto* n { tree.name(h) }) c.name(*n); break;
    case Sort::declaration: break;
    }
    return std::move(c.out);
}

std::vector<std::string> validate(const Tree& tree) {
    std::vector<std::string> out;
    const auto kind_of = [&](Handle h) -> std::string {
        if (const auto* e { tree.expression(h) }) return "expression " + std::string { to_string(e->kind) };
        if (const auto* s { tree.statement(h) }) return "statement " + std::string { to_string(s->kind) };
        if (const auto* t { tree.type(h) }) return "type " + std::string { to_string(t->kind) };
        if (const auto* l { tree.local(h) }) return "local " + std::string { to_string(l->kind) };
        if (tree.name(h) != nullptr) return "name";
        return "?";
    };
    const auto bad = [&](Handle h, std::string what) {
        if (out.size() < 50) out.push_back(std::format("{} #{} (tokens {}..{}): {}", kind_of(h), h.index, tree.tokens(h).first, tree.tokens(h).second, std::move(what)));
    };
    const auto exists = [&](Handle h) {
        switch (h.sort) {
        case Sort::expression: return h.index < tree.expressions.size();
        case Sort::statement: return h.index < tree.statements.size();
        case Sort::type: return h.index < tree.types.size();
        case Sort::local: return h.index < tree.locals.size();
        case Sort::name: return h.index < tree.names.size();
        case Sort::declaration: return tree.syntax != nullptr && h.index < tree.syntax->declarations.size();
        }
        return false;
    };
    const auto derived_type = [&](Handle h) {
        const TypeNode* t { tree.type(h) };
        return t != nullptr && (t->kind == TypeKind::pointer || t->kind == TypeKind::lvalue_ref || t->kind == TypeKind::rvalue_ref ||
                                t->kind == TypeKind::array || t->kind == TypeKind::function || t->kind == TypeKind::member_pointer);
    };
    const auto check = [&](Handle h) {
        const auto [first, last] = tree.tokens(h);
        if (first > last) bad(h, "its first token is after its last");
        // A derived type (a pointer to what the specifiers wrote) is written around its inner type's tokens, not over them.
        if (derived_type(h)) return;
        for (const auto child : children(tree, h)) {
            if (!exists(child)) {
                bad(h, "a child that is not in the tree");
                continue;
            }
            const auto [cf, cl] = tree.tokens(child);
            const bool error_node { (tree.expression(child) != nullptr && tree.expression(child)->kind == ExprKind::error) ||
                                    (tree.statement(child) != nullptr && tree.statement(child)->kind == StmtKind::error) ||
                                    (tree.type(child) != nullptr && tree.type(child)->kind == TypeKind::error) ||
                                    (tree.local(child) != nullptr && tree.local(child)->kind == LocalKind::error) };
            if (!derived_type(child) && !error_node && (cf < first || cl > last))
                bad(h, std::format("a child ({} {}) spans tokens {}..{} outside its {}..{}", static_cast<int>(child.sort), child.index, cf, cl, first, last));
        }
    };
    for (std::uint32_t i { 0 }; i < tree.expressions.size(); ++i) check({ Sort::expression, i });
    for (std::uint32_t i { 0 }; i < tree.statements.size(); ++i) check({ Sort::statement, i });
    for (std::uint32_t i { 0 }; i < tree.types.size(); ++i) check({ Sort::type, i });
    for (std::uint32_t i { 0 }; i < tree.locals.size(); ++i) check({ Sort::local, i });
    for (std::uint32_t i { 0 }; i < tree.names.size(); ++i) check({ Sort::name, i });
    for (const auto& r : tree.roots) {
        if (!r.node) {
            bad(r.node, "a root without a node");
            continue;
        }
        const auto [first, last] = tree.tokens(r.node);
        if (first < r.first || last > r.last) bad(r.node, std::format("a root spans tokens {}..{} outside its part's {}..{}", first, last, r.first, r.last));
    }
    return out;
}

} // namespace mcxx::frontend::ast
