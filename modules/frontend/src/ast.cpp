// :ast's definitions: names of kinds, handles back to nodes, ranges, and the dump of a tree as text.
module mcxx.frontend;

import std;
import mcxx.msa;

namespace mcxx::frontend::ast {

namespace {

template <class E, std::size_t N>
std::string_view lookup(const std::array<std::string_view, N>& names, E e) {
    const auto i { static_cast<std::size_t>(e) };
    return i < N ? names[i] : std::string_view { "?" };
}

constexpr std::array<std::string_view, 58> OPS {
    "", "+", "-", "!", "~", "*", "&", "++", "--", "++", "--",
    "*", "/", "%", "+", "-", "<<", ">>", "<", ">", "<=", ">=", "<=>", "==", "!=", "&", "^", "|", "&&", "||", ",", ".*", "->*",
    "=", "*=", "/=", "%=", "+=", "-=", "<<=", ">>=", "&=", "^=", "|=",
    "static_cast", "dynamic_cast", "const_cast", "reinterpret_cast",
    "()", "[]", "new", "delete", "new[]", "delete[]", "co_await", "->", "__real__", "__imag__",
};
static_assert(OPS.size() == static_cast<std::size_t>(Op::imag_part) + 1);

constexpr std::array<std::string_view, 24> BUILTINS {
    "", "void", "bool", "char", "signed char", "unsigned char", "wchar_t", "char8_t", "char16_t", "char32_t", "short", "unsigned short", "int",
    "unsigned int", "long", "unsigned long", "long long", "unsigned long long", "float", "double", "long double", "__int128", "unsigned __int128",
    "other",
};
static_assert(BUILTINS.size() == static_cast<std::size_t>(Builtin::other) + 1);

constexpr std::array<std::string_view, 5> BASES { "syntax", "scope", "lookup", "feedback", "shape" };
constexpr std::array<std::string_view, 6> AMBIGUITIES { "name", "template-arguments", "declaration-or-expression", "cast-or-parenthesized",
                                                        "parameters-or-initializer", "type-or-expression-argument" };
constexpr std::array<std::string_view, 19> RESOLUTIONS { "type", "class-template", "alias-template", "function-template", "variable-template",
                                                         "concept", "value", "namespace", "unknown", "arguments", "less-than", "declaration",
                                                         "expression", "cast", "parenthesized", "parameters", "initializer", "type-argument",
                                                         "expression-argument" };
static_assert(RESOLUTIONS.size() == static_cast<std::size_t>(Resolution::expression_argument) + 1);

constexpr std::array<std::string_view, 37> EXPR_KINDS {
    "error", "integer", "floating", "character", "string", "boolean", "null-pointer", "this", "id", "paren", "unary", "binary", "assign",
    "conditional", "call", "subscript", "member", "cast-named", "cast-c", "cast-functional", "sizeof", "sizeof-pack", "noexcept", "typeid",
    "new", "delete", "throw", "co-await", "co-yield", "lambda", "requires", "fold", "pack-expansion", "init-list", "designated",
    "statement-expression", "builtin",
};
constexpr std::array<std::string_view, 6> EXPR_KINDS_TAIL { "label-address", "reflect", "splice", "pack-index", "member-init", "paren-list" };

constexpr std::array<std::string_view, 27> STMT_KINDS {
    "error", "null", "compound", "expression", "declaration", "if", "switch", "while", "do", "for", "range-for", "return", "break",
    "continue", "goto", "labeled", "case", "default", "try", "handler", "co-return", "asm", "static-assert", "contract-assert",
    "consteval-block", "attributed", "function-body",
};

constexpr std::array<std::string_view, 16> TYPE_KINDS {
    "error", "builtin", "named", "elaborated", "decltype", "auto", "pointer", "lvalue-ref", "rvalue-ref", "member-pointer", "array", "function",
    "pack-expansion", "pack-index", "splice", "atomic",
};

constexpr std::array<std::string_view, 29> LOCAL_KINDS {
    "variable", "parameter", "field", "function", "type-alias", "class", "enum", "enumerator", "structured-binding", "binding", "capture",
    "template-parameter", "using-declaration", "using-directive", "using-enum", "namespace", "namespace-alias", "static-assert", "access",
    "friend", "asm", "empty", "concept", "consteval-block", "deduction-guide", "linkage", "export-block", "template", "error",
};

} // namespace

std::string_view spelling(Op op) { return lookup(OPS, op); }
std::string_view spelling(Builtin b) { return lookup(BUILTINS, b); }
std::string_view to_string(Basis b) { return lookup(BASES, b); }
std::string_view to_string(Ambiguity a) { return lookup(AMBIGUITIES, a); }
std::string_view to_string(Resolution r) { return lookup(RESOLUTIONS, r); }

std::string_view to_string(ExprKind k) {
    const auto i { static_cast<std::size_t>(k) };
    if (i < EXPR_KINDS.size()) return EXPR_KINDS[i];
    return i - EXPR_KINDS.size() < EXPR_KINDS_TAIL.size() ? EXPR_KINDS_TAIL[i - EXPR_KINDS.size()] : std::string_view { "?" };
}
std::string_view to_string(StmtKind k) { return lookup(STMT_KINDS, k); }
std::string_view to_string(TypeKind k) {
    const auto i { static_cast<std::size_t>(k) };
    if (i < TYPE_KINDS.size()) return TYPE_KINDS[i];
    return k == TypeKind::typeof_ ? "typeof" : "?";
}
std::string_view to_string(LocalKind k) { return lookup(LOCAL_KINDS, k); }

View Tree::get(Handle h) const {
    View v;
    switch (h.sort) {
    case Sort::declaration: v.declaration = syntax != nullptr && h.index < syntax->declarations.size() ? &syntax->declarations[h.index] : nullptr; break;
    case Sort::local: v.local = local(h); break;
    case Sort::statement: v.statement = statement(h); break;
    case Sort::expression: v.expression = expression(h); break;
    case Sort::type: v.type = type(h); break;
    case Sort::name: v.name = name(h); break;
    }
    return v;
}

std::pair<std::uint32_t, std::uint32_t> Tree::tokens(Handle h) const {
    const View v { get(h) };
    if (v.expression != nullptr) return { v.expression->first, v.expression->last };
    if (v.statement != nullptr) return { v.statement->first, v.statement->last };
    if (v.type != nullptr) return { v.type->first, v.type->last };
    if (v.local != nullptr) return { v.local->first, v.local->last };
    if (v.name != nullptr) return { v.name->first, v.name->last };
    if (v.declaration != nullptr) return { v.declaration->first_token, v.declaration->last_token };
    return { 0, 0 };
}

msa::Range Tree::range(Handle h) const {
    if (syntax == nullptr || !h) return {};
    const auto [first, last] = tokens(h);
    return token_range(*syntax, first, last);
}

std::string Tree::text(std::uint32_t first, std::uint32_t last) const {
    std::string out;
    if (syntax == nullptr) return out;
    const auto& t = syntax->pp.tokens;
    for (std::uint32_t k { first }; k <= last && k < t.size(); ++k) {
        if (k > first && t[k].leading_space) out += ' ';
        out += t[k].spelling;
    }
    return out;
}

std::string Tree::text(Handle h) const {
    const auto [first, last] = tokens(h);
    return text(first, last);
}

namespace {

class Dumper {
public:
    explicit Dumper(const Tree& t) : t_ { t } {}

    std::string node(Handle h) {
        if (!h) return "_";
        switch (h.sort) {
        case Sort::expression: return expression(t_.expressions[h.index]);
        case Sort::statement: return statement(t_.statements[h.index]);
        case Sort::type: return type(t_.types[h.index]);
        case Sort::name: return name(t_.names[h.index]);
        case Sort::local: return local(t_.locals[h.index]);
        case Sort::declaration:
            return t_.syntax != nullptr && h.index < t_.syntax->declarations.size() ? std::format("(declaration {})", t_.syntax->declarations[h.index].name) : "(declaration)";
        }
        return "?";
    }

private:
    const Tree& t_;

    std::string tok(std::uint32_t k) const { return k < t_.syntax->pp.tokens.size() ? std::string { t_.syntax->pp.tokens[k].spelling } : std::string {}; }
    template <Sort S>
    std::string id(Id<S> i) { return i ? node(i.handle()) : "_"; }
    std::string list(Span32 s) {
        std::string out;
        for (const auto h : t_.list(s)) out += " " + node(h);
        return out;
    }

    std::string name(const Name& n) {
        std::string out;
        if (n.global) out += "::";
        bool first { true };
        for (const auto& c : t_.parts(n)) {
            if (!first) out += "::";
            first = false;
            if (c.has(NameComponent::template_keyword)) out += "template ";
            switch (c.form) {
            case NameComponent::Form::identifier: out += tok(c.token); break;
            case NameComponent::Form::operator_function: {
                const std::string_view op { spelling(c.op) };
                out += "operator" + std::string { !op.empty() && std::isalpha(static_cast<unsigned char>(op[0])) != 0 ? " " : "" } + std::string { op };
                break;
            }
            case NameComponent::Form::conversion_function: out += "operator " + id(c.type); break;
            case NameComponent::Form::literal_operator: out += "operator\"\"" + tok(c.token); break;
            case NameComponent::Form::destructor: out += "~" + (c.type ? id(c.type) : tok(c.token)); break;
            case NameComponent::Form::decltype_: out += "decltype(" + id(c.expr) + ")"; break;
            case NameComponent::Form::splice: out += "[:" + id(c.expr) + ":]"; break;
            }
            if (c.has(NameComponent::has_arguments)) {
                out += "<";
                bool again { false };
                for (const auto h : t_.list(c.arguments)) out += (std::exchange(again, true) ? ", " : "") + node(h);
                out += ">";
            }
        }
        return out;
    }

    std::string expression(const Expr& e) {
        const std::string kind { to_string(e.kind) };
        switch (e.kind) {
        case ExprKind::error: return "(error)";
        case ExprKind::integer: case ExprKind::floating: case ExprKind::character: case ExprKind::boolean:
            return std::format("({} {})", kind, t_.text(e.first, e.last));
        case ExprKind::string: return std::format("(string {})", t_.text(e.first, e.last));
        case ExprKind::null_pointer: case ExprKind::this_: return "(" + kind + ")";
        case ExprKind::id: return "(id " + id(e.name) + ")";
        case ExprKind::paren: return "(paren " + id(e.a) + ")";
        case ExprKind::unary: return std::format("(unary {}{} {})", e.has(Expr::postfix) ? "post" : "", spelling(e.op), id(e.a));
        case ExprKind::binary: return std::format("(binary {} {} {})", spelling(e.op), id(e.a), id(e.b));
        case ExprKind::assign: return std::format("(assign {} {} {})", spelling(e.op), id(e.a), id(e.b));
        case ExprKind::conditional: return std::format("(conditional {} {} {})", id(e.a), id(e.b), id(e.c));
        case ExprKind::call: return "(call " + id(e.a) + list(e.list) + ")";
        case ExprKind::subscript: return "(subscript " + id(e.a) + list(e.list) + ")";
        case ExprKind::member: return std::format("(member {} {} {})", e.has(Expr::arrow) ? "->" : ".", id(e.a), id(e.name));
        case ExprKind::cast_named: return std::format("(cast {} {} {})", spelling(e.op), id(e.type), id(e.a));
        case ExprKind::cast_c: return std::format("(cast-c {} {})", id(e.type), id(e.a));
        case ExprKind::cast_functional: return std::format("(cast-functional{} {}{})", e.has(Expr::brace) ? "-brace" : "", id(e.type), list(e.list));
        case ExprKind::sizeof_: return std::format("({} {})", e.has(Expr::alignof_) ? "alignof" : "sizeof", e.type ? id(e.type) : id(e.a));
        case ExprKind::sizeof_pack: return "(sizeof-pack " + id(e.name) + ")";
        case ExprKind::noexcept_: return "(noexcept " + id(e.a) + ")";
        case ExprKind::typeid_: return "(typeid " + (e.type ? id(e.type) : id(e.a)) + ")";
        case ExprKind::new_: {
            const News& n { t_.news[e.aux] };
            std::string out { std::format("(new{}{} {}", e.has(Expr::global) ? " global" : "", e.has(Expr::array) ? " array" : "", id(n.type)) };
            if (n.bound) out += " [" + id(n.bound) + "]";
            for (const auto h : t_.list(n.placement)) out += " @" + node(h);
            if (n.initialized) out += (n.brace_init ? " {" : " (") + id(n.init) + (n.brace_init ? "}" : ")");
            return out + ")";
        }
        case ExprKind::delete_: return std::format("(delete{}{} {})", e.has(Expr::global) ? " global" : "", e.has(Expr::array) ? " array" : "", id(e.a));
        case ExprKind::throw_: return "(throw " + id(e.a) + ")";
        case ExprKind::co_await_: case ExprKind::co_yield_: return "(" + kind + " " + id(e.a) + ")";
        case ExprKind::lambda: {
            const LambdaInfo& l { t_.lambdas[e.aux] };
            std::string out { "(lambda [" };
            bool again { false };
            for (const auto h : t_.list(l.captures)) out += (std::exchange(again, true) ? ", " : "") + node(h);
            out += "]";
            if (l.template_params.count != 0) out += " <" + list(l.template_params).substr(1) + ">";
            if (l.has_params) out += " (" + (l.params.count != 0 ? list(l.params).substr(1) : std::string {}) + ")";
            if (l.function) out += " " + id(l.function);
            if (l.mutable_) out += " mutable";
            if (l.constexpr_) out += " constexpr";
            if (l.consteval_) out += " consteval";
            if (l.static_) out += " static";
            if (l.requires_) out += " requires " + id(l.requires_);
            if (l.trailing_requires) out += " requires " + id(l.trailing_requires);
            return out + " " + id(l.body) + ")";
        }
        case ExprKind::requires_: {
            const RequiresInfo& r { t_.requires_infos[e.aux] };
            std::string out { "(requires" };
            if (r.has_params) out += " (" + (r.params.count != 0 ? list(r.params).substr(1) : std::string {}) + ")";
            for (std::uint32_t i { 0 }; i < r.requirements.count; ++i) {
                const Requirement& q { t_.requirements[r.requirements.begin + i] };
                switch (q.form) {
                case Requirement::Form::simple: out += " (simple " + id(q.expr) + ")"; break;
                case Requirement::Form::type: out += " (type " + id(q.type) + ")"; break;
                case Requirement::Form::compound:
                    out += " (compound " + id(q.expr) + (q.noexcept_ ? " noexcept" : "") + (q.type ? " -> " + id(q.type) : q.constraint ? " -> " + id(q.constraint) : std::string {}) + ")";
                    break;
                case Requirement::Form::nested: out += " (nested " + id(q.expr) + ")"; break;
                }
            }
            return out + ")";
        }
        case ExprKind::fold: {
            const FoldInfo& f { t_.folds[e.aux] };
            return std::format("(fold {} {} {}{})", spelling(f.op), f.left ? "left" : "right", id(f.pattern), f.binary ? " " + id(f.init) : std::string {});
        }
        case ExprKind::pack_expansion: return "(pack-expansion " + id(e.a) + ")";
        case ExprKind::init_list: return "(init-list" + list(e.list) + ")";
        case ExprKind::paren_list: return "(paren-list" + list(e.list) + ")";
        case ExprKind::designated: {
            std::string out { "(designated" };
            for (std::uint32_t i { 0 }; i < e.list.count; ++i) {
                const Designator& d { t_.designators[e.aux + i] };
                out += d.field ? " ." + tok(d.token) : " [" + id(d.index) + "]";
            }
            return out + " = " + id(e.a) + ")";
        }
        case ExprKind::statement_expression: return "(statement-expression " + id(e.stmt) + ")";
        case ExprKind::builtin: return "(builtin " + tok(e.token) + list(e.list) + ")";
        case ExprKind::label_address: return "(label-address " + tok(e.token) + ")";
        case ExprKind::reflect: return "(reflect" + list(e.list) + ")";
        case ExprKind::splice: return "(splice " + id(e.a) + ")";
        case ExprKind::pack_index: return std::format("(pack-index {} {})", id(e.a), id(e.b));
        case ExprKind::member_init: return std::format("(member-init{} {}{})", e.has(Expr::brace) ? "-brace" : "", id(e.name), list(e.list));
        }
        return "?";
    }

    std::string statement(const Stmt& s) {
        const std::string kind { to_string(s.kind) };
        switch (s.kind) {
        case StmtKind::error: return "(error)";
        case StmtKind::null: return "(null)";
        case StmtKind::compound: return "(compound" + list(s.list) + ")";
        case StmtKind::expression: return "(expression " + id(s.e1) + ")";
        case StmtKind::declaration: return "(declaration" + list(s.list) + ")";
        case StmtKind::if_: {
            std::string out { "(if" };
            if (s.has(Stmt::constexpr_)) out += " constexpr";
            if (s.has(Stmt::consteval_)) out += s.has(Stmt::negated) ? " !consteval" : " consteval";
            if (s.a) out += " init " + id(s.a);
            if (!s.has(Stmt::consteval_)) out += " " + (s.local ? id(s.local) : id(s.e1));
            return out + " " + id(s.b) + (s.c ? " else " + id(s.c) : std::string {}) + ")";
        }
        case StmtKind::switch_: return std::format("(switch{} {} {})", s.a ? " init " + id(s.a) : std::string {}, s.local ? id(s.local) : id(s.e1), id(s.b));
        case StmtKind::while_: return std::format("(while {} {})", s.local ? id(s.local) : id(s.e1), id(s.b));
        case StmtKind::do_: return std::format("(do {} {})", id(s.a), id(s.e1));
        case StmtKind::for_: return std::format("(for {} {} {} {})", id(s.a), s.local ? id(s.local) : id(s.e1), id(s.e2), id(s.b));
        case StmtKind::range_for:
            return std::format("({}{} {} : {} {})", s.has(Stmt::expansion) ? "template-for" : "range-for", s.a ? " init " + id(s.a) : std::string {}, id(s.local), id(s.e1), id(s.b));
        case StmtKind::return_: return "(return " + id(s.e1) + ")";
        case StmtKind::break_: return "(break)";
        case StmtKind::continue_: return "(continue)";
        case StmtKind::goto_: return s.e1 ? "(goto * " + id(s.e1) + ")" : "(goto " + tok(s.token) + ")";
        case StmtKind::labeled: return std::format("(labeled {} {})", tok(s.token), id(s.a));
        case StmtKind::case_: return std::format("(case {}{} {})", id(s.e1), s.e2 ? " ... " + id(s.e2) : std::string {}, id(s.a));
        case StmtKind::default_: return "(default " + id(s.a) + ")";
        case StmtKind::try_: return "(try " + id(s.a) + list(s.list) + ")";
        case StmtKind::handler: return std::format("(handler {} {})", s.local ? id(s.local) : "...", id(s.a));
        case StmtKind::co_return_: return "(co-return " + id(s.e1) + ")";
        case StmtKind::asm_: return "(asm)";
        case StmtKind::static_assert_: return std::format("(static-assert {}{})", id(s.e1), s.e2 ? " " + id(s.e2) : std::string {});
        case StmtKind::contract_assert: return "(contract-assert " + id(s.e1) + ")";
        case StmtKind::consteval_block: return "(consteval-block " + id(s.a) + ")";
        case StmtKind::attributed: return "(attributed " + id(s.a) + ")";
        case StmtKind::function_body:
            return std::format("(function-body{}{}{}{}{}{})", s.has(Stmt::defaulted) ? " default" : "", s.has(Stmt::deleted) ? " delete" : "", s.has(Stmt::pure) ? " pure" : "",
                               s.e1 ? " reason " + id(s.e1) : std::string {}, list(s.list), s.a ? " " + id(s.a) : std::string {});
        }
        return "(" + kind + ")";
    }

    std::string type(const TypeNode& n) {
        std::string cv;
        if (n.has(TypeNode::const_)) cv += " const";
        if (n.has(TypeNode::volatile_)) cv += " volatile";
        if (n.has(TypeNode::restrict_)) cv += " restrict";
        switch (n.kind) {
        case TypeKind::error: return "(error-type)";
        case TypeKind::builtin: return std::format("(builtin {}{})", n.builtin == Builtin::other ? t_.text(n.first, n.last) : std::string { spelling(n.builtin) }, cv);
        case TypeKind::named: return "(named " + id(n.name) + cv + ")";
        case TypeKind::elaborated: return std::format("(elaborated {} {}{}{})", tok(n.token), id(n.name), n.local ? " " + id(n.local) : std::string {}, cv);
        case TypeKind::decltype_: return n.has(TypeNode::decltype_auto) ? "(decltype-auto)" : "(decltype " + id(n.expr) + ")";
        case TypeKind::auto_: return "(auto" + (n.name ? " " + id(n.name) : std::string {}) + cv + ")";
        case TypeKind::pointer: return "(pointer " + id(n.inner) + cv + ")";
        case TypeKind::lvalue_ref: return "(lvalue-ref " + id(n.inner) + ")";
        case TypeKind::rvalue_ref: return "(rvalue-ref " + id(n.inner) + ")";
        case TypeKind::member_pointer: return std::format("(member-pointer {} {}{})", id(n.name), id(n.inner), cv);
        case TypeKind::array: return std::format("(array {} {})", id(n.inner), n.expr ? id(n.expr) : n.has(TypeNode::vla) ? "*" : "_");
        case TypeKind::function: {
            std::string out { "(function " + id(n.inner) + " (" };
            bool again { false };
            for (const auto h : t_.list(n.list)) out += (std::exchange(again, true) ? " " : "") + node(h);
            out += n.has(TypeNode::c_variadic) ? (again ? " ...)" : "...)") : ")";
            if (n.has(TypeNode::const_method)) out += " const";
            if (n.has(TypeNode::volatile_method)) out += " volatile";
            if (n.has(TypeNode::lvalue_method)) out += " &";
            if (n.has(TypeNode::rvalue_method)) out += " &&";
            if (n.has(TypeNode::noexcept_)) out += n.expr ? " noexcept(" + id(n.expr) + ")" : " noexcept";
            return out + ")";
        }
        case TypeKind::pack_expansion: return "(pack " + id(n.inner) + ")";
        case TypeKind::pack_index: return std::format("(pack-index {} {})", n.inner ? id(n.inner) : id(n.name), id(n.expr));
        case TypeKind::splice: return "(splice " + id(n.expr) + ")";
        case TypeKind::atomic: return "(atomic " + id(n.inner) + ")";
        case TypeKind::typeof_: return "(typeof " + (n.inner ? id(n.inner) : id(n.expr)) + ")";
        }
        return "?";
    }

    std::string local(const Local& l) {
        const std::string name { l.name ? id(l.name) : l.name_token != NONE ? tok(l.name_token) : std::string { "_" } };
        std::string out { "(" + std::string { to_string(l.kind) } };
        if (l.kind == LocalKind::structured_binding || l.kind == LocalKind::namespace_ || l.kind == LocalKind::template_) {
        } else if (l.kind == LocalKind::access) out += l.form == 1 ? " public" : l.form == 2 ? " protected" : " private";
        else if (l.kind == LocalKind::capture && l.form != 0) {}
        else if (l.name || l.name_token != NONE) out += std::string { l.kind == LocalKind::capture && l.has(Local::by_ref) ? " &" : " " } + name;
        if (l.kind == LocalKind::structured_binding) out += l.has(Local::by_ref) ? " &" : l.has(Local::by_rvalue_ref) ? " &&" : "";
        if (l.kind == LocalKind::capture) {
            if (l.form == 1) out += " this";
            else if (l.form == 2) out += " *this";
            else if (l.form == 3) out += l.has(Local::by_ref) ? " &" : " =";
            else if (l.has(Local::by_ref)) out += "";
        }
        if (l.kind == LocalKind::static_assert_) return out + " " + id(l.init) + (l.width ? " " + id(l.width) : std::string {}) + ")";
        if (l.has(Local::static_)) out += " static";
        if (l.has(Local::constexpr_)) out += " constexpr";
        if (l.has(Local::typedef_)) out += " typedef";
        if (l.has(Local::friend_)) out += " friend";
        if (l.has(Local::virtual_)) out += " virtual";
        if (l.has(Local::pack)) out += " pack";
        if (l.has(Local::pure)) out += " =0";
        if (l.has(Local::defaulted)) out += " =default";
        if (l.has(Local::deleted)) out += " =delete";
        if (l.type) out += " " + id(l.type);
        switch (l.kind) {
        case LocalKind::class_:
            for (std::uint32_t i { 0 }; i < l.bases.count; ++i) out += " :" + id(t_.bases[l.bases.begin + i].type);
            out += list(l.members);
            break;
        case LocalKind::enum_: case LocalKind::namespace_: case LocalKind::structured_binding: case LocalKind::linkage: case LocalKind::export_block:
            out += list(l.members);
            break;
        case LocalKind::template_:
            out += " <" + (l.params.count != 0 ? list(l.params).substr(1) : std::string {}) + ">";
            if (l.constraint) out += " requires " + id(l.constraint);
            out += " " + id(l.target);
            return out + ")";
        case LocalKind::function:
            if (l.constraint) out += " requires " + id(l.constraint);
            break;
        default: break;
        }
        if (l.width) out += " :" + id(l.width);
        if (l.init) out += (l.has(Local::brace_init) ? " {" : l.has(Local::direct_init) ? " (" : " = ") + id(l.init) + (l.has(Local::brace_init) ? "}" : l.has(Local::direct_init) ? ")" : "");
        if (l.body) out += " " + id(l.body);
        return out + ")";
    }
};

} // namespace

std::string dump(const Tree& tree, Handle h) { return Dumper { tree }.node(h); }

} // namespace mcxx::frontend::ast
