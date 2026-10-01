// The MC1 feature each syntax-tree node is (D2/C3): the id a profile denies it by, so the front end can
// report a gate diagnostic where the construct is read, not by a later walk. A node that is core C++ no
// profile asks about says "". Ids of mc++.iso and of the C++26 package are the catalog's own; the rest are
// proposed in its naming and flagged unregistered in syntax_features().
//
// What a profile asks of a type -- `uninitialized`, `raw-pointer-arithmetic`, `c-style-cast` of `T(x)` to a
// scalar -- needs types, not syntax: those wait for the type system (M3.1).
module mcxx.frontend;

import std;
import mcxx.msa;

namespace mcxx::frontend::ast {

namespace {

// The ids, once: a node's feature is one of these.
constexpr std::string_view NEW_DELETE { "new-delete" }, REINTERPRET { "reinterpret-cast" }, C_STYLE { "c-style-cast" }, CONST_CAST { "const-cast" },
    RTTI { "rtti" }, EXCEPTIONS { "exceptions" }, GOTO { "goto" }, ASM { "asm" }, UNION { "union" }, C_ARRAY { "c-array" }, C_VARARGS { "c-varargs" },
    REFLECTION { "c++26:reflection" }, PACK_INDEXING { "c++26:pack-indexing" }, CONTRACTS { "c++26:contracts" },
    DELETE_REASON { "c++26:delete-with-reason" }, EXPANSION { "c++26:expansion-statements" },
    BINDING_PACK { "c++26:structured-bindings-can-introduce-pack" }, BINDING_CONDITION { "c++26:structured-binding-declaration-as-condition" },
    VARIADIC_FRIEND { "c++26:variadic-friends" }, CONSTEVAL_BLOCK { "c++26:consteval-blocks" },
    LAMBDA { "c++11:lambda" }, RANGE_FOR { "c++11:range-for" }, CONCEPTS { "c++20:concepts" }, FOLD { "c++17:fold-expressions" },
    IF_CONSTEXPR { "c++17:if-constexpr" }, STRUCTURED_BINDINGS { "c++17:structured-bindings" }, COROUTINES { "c++20:coroutines" },
    SPACESHIP { "c++20:three-way-comparison" }, DESIGNATED { "c++20:designated-initializers" }, IF_CONSTEVAL { "c++23:if-consteval" },
    MULTI_SUBSCRIPT { "c++23:multidimensional-subscript" }, EXPLICIT_OBJECT { "c++23:explicit-object-parameter" },
    INIT_STATEMENT { "c++17:init-statements" }, CONSTEVAL { "c++20:consteval" }, CONSTINIT { "c++20:constinit" },
    PACK_EXPANSION { "c++11:variadic-templates" }, DECLTYPE { "c++11:decltype" }, NOEXCEPT_EXPR { "c++11:noexcept" },
    STATEMENT_EXPRESSION { "ext:gnu-statement-expression" }, LABEL_ADDRESS { "ext:gnu-address-of-label" },
    OMITTED_OPERAND { "ext:gnu-conditional-omitted-operand" }, BUILTIN { "ext:gnu-builtin" }, CASE_RANGE { "ext:gnu-case-range" },
    TYPEOF { "ext:gnu-typeof" }, COMPOUND_LITERAL { "ext:gnu-compound-literal" }, REAL_IMAG { "ext:gnu-complex-parts" };

constexpr FeatureInfo FEATURES[] {
    { NEW_DELETE, true, "a new or delete expression" },
    { REINTERPRET, true, "reinterpret_cast" },
    { C_STYLE, true, "a C-style cast" },
    { CONST_CAST, true, "const_cast" },
    { RTTI, true, "typeid, dynamic_cast" },
    { EXCEPTIONS, true, "throw, try" },
    { GOTO, true, "goto" },
    { ASM, true, "an asm statement or declaration" },
    { UNION, true, "a union" },
    { C_ARRAY, true, "an array type" },
    { C_VARARGS, true, "a C variadic function; va_arg" },
    { REFLECTION, true, "^^ and [: :]" },
    { PACK_INDEXING, true, "pack...[n]" },
    { CONTRACTS, true, "pre, post, contract_assert" },
    { DELETE_REASON, true, "= delete(\"why\")" },
    { EXPANSION, true, "template for" },
    { BINDING_PACK, true, "auto [...xs]" },
    { BINDING_CONDITION, true, "a structured binding as a condition" },
    { VARIADIC_FRIEND, true, "friend Ts...;" },
    { CONSTEVAL_BLOCK, false, "consteval { }" },
    { LAMBDA, false, "a lambda-expression" },
    { RANGE_FOR, false, "a range-based for" },
    { CONCEPTS, false, "a concept, a requires-clause, a requires-expression" },
    { FOLD, false, "a fold-expression" },
    { IF_CONSTEXPR, false, "if constexpr" },
    { STRUCTURED_BINDINGS, false, "a structured binding" },
    { COROUTINES, false, "co_await, co_yield, co_return" },
    { SPACESHIP, false, "<=>" },
    { DESIGNATED, false, "a designated initializer" },
    { IF_CONSTEVAL, false, "if consteval" },
    { MULTI_SUBSCRIPT, false, "a subscript of other than one operand" },
    { EXPLICIT_OBJECT, false, "an explicit object parameter" },
    { INIT_STATEMENT, false, "an init-statement in if, switch, or a range-for" },
    { CONSTEVAL, false, "consteval" },
    { CONSTINIT, false, "constinit" },
    { PACK_EXPANSION, false, "a pack expansion" },
    { DECLTYPE, false, "decltype" },
    { NOEXCEPT_EXPR, false, "a noexcept-expression" },
    { STATEMENT_EXPRESSION, false, "({ ... })" },
    { LABEL_ADDRESS, false, "&&label" },
    { OMITTED_OPERAND, false, "a ?: b" },
    { BUILTIN, false, "a __builtin_ or a type trait" },
    { CASE_RANGE, false, "case a ... b" },
    { TYPEOF, false, "__typeof__" },
    { COMPOUND_LITERAL, false, "(T){ ... }" },
    { REAL_IMAG, false, "__real__, __imag__" },
};

} // namespace

std::span<const FeatureInfo> syntax_features() { return FEATURES; }

std::string_view feature_of(const Expr& e) {
    switch (e.kind) {
    case ExprKind::new_: return NEW_DELETE;
    case ExprKind::delete_: return NEW_DELETE;
    case ExprKind::cast_c: return C_STYLE;
    case ExprKind::cast_named:
        if (e.op == Op::reinterpret_cast_) return REINTERPRET;
        if (e.op == Op::const_cast_) return CONST_CAST;
        if (e.op == Op::dynamic_cast_) return RTTI;
        return {};
    case ExprKind::typeid_: return RTTI;
    case ExprKind::throw_: return EXCEPTIONS;
    case ExprKind::label_address: return LABEL_ADDRESS;
    case ExprKind::unary: return e.op == Op::real_part || e.op == Op::imag_part ? REAL_IMAG : std::string_view {};
    case ExprKind::lambda: return LAMBDA;
    case ExprKind::requires_: return CONCEPTS;
    case ExprKind::fold: return FOLD;
    case ExprKind::pack_expansion: return PACK_EXPANSION;
    case ExprKind::co_await_: case ExprKind::co_yield_: return COROUTINES;
    case ExprKind::binary: return e.op == Op::spaceship ? SPACESHIP : std::string_view {};
    case ExprKind::designated: return DESIGNATED;
    case ExprKind::subscript: return e.list.count != 1 ? MULTI_SUBSCRIPT : std::string_view {};
    case ExprKind::statement_expression: return STATEMENT_EXPRESSION;
    case ExprKind::conditional: return e.b ? std::string_view {} : OMITTED_OPERAND;
    case ExprKind::noexcept_: return NOEXCEPT_EXPR;
    case ExprKind::builtin: return BUILTIN;
    case ExprKind::reflect: case ExprKind::splice: return REFLECTION;
    case ExprKind::pack_index: return PACK_INDEXING;
    default: return {};
    }
}

std::string_view feature_of(const Stmt& s) {
    switch (s.kind) {
    case StmtKind::goto_: return GOTO;
    case StmtKind::asm_: return ASM;
    case StmtKind::try_: case StmtKind::handler: return EXCEPTIONS;
    case StmtKind::co_return_: return COROUTINES;
    case StmtKind::range_for: return s.has(Stmt::expansion) ? EXPANSION : RANGE_FOR;
    case StmtKind::if_:
        if (s.has(Stmt::consteval_)) return IF_CONSTEVAL;
        return s.has(Stmt::constexpr_) ? IF_CONSTEXPR : std::string_view {};
    case StmtKind::contract_assert: return CONTRACTS;
    case StmtKind::consteval_block: return CONSTEVAL_BLOCK;
    case StmtKind::case_: return s.e2 ? CASE_RANGE : std::string_view {};
    default: return {};
    }
}

std::string_view feature_of(const TypeNode& t) {
    switch (t.kind) {
    case TypeKind::array: return C_ARRAY;
    case TypeKind::function: return t.has(TypeNode::c_variadic) ? C_VARARGS : std::string_view {};
    case TypeKind::decltype_: return DECLTYPE;
    case TypeKind::splice: return REFLECTION;
    case TypeKind::pack_index: return PACK_INDEXING;
    case TypeKind::pack_expansion: return PACK_EXPANSION;
    case TypeKind::typeof_: return TYPEOF;
    default: return {};
    }
}

std::string_view feature_of(const Local& l) {
    switch (l.kind) {
    case LocalKind::class_: return l.entity == msa::Kind::union_ ? UNION : std::string_view {};
    case LocalKind::asm_: return ASM;
    case LocalKind::concept_: return CONCEPTS;
    case LocalKind::structured_binding:
        if (l.has(Local::pack)) return BINDING_PACK;
        return l.has(Local::in_condition) ? BINDING_CONDITION : STRUCTURED_BINDINGS;
    case LocalKind::binding: return l.has(Local::pack) ? BINDING_PACK : std::string_view {};
    case LocalKind::consteval_block: return CONSTEVAL_BLOCK;
    case LocalKind::friend_: return l.has(Local::pack) ? VARIADIC_FRIEND : std::string_view {};
    case LocalKind::parameter: return l.has(Local::explicit_object) ? EXPLICIT_OBJECT : std::string_view {};
    case LocalKind::function:
        if (l.has(Local::deleted_with_reason)) return DELETE_REASON;
        if (l.has(Local::consteval_)) return CONSTEVAL;
        return l.contracts.count != 0 ? CONTRACTS : std::string_view {};
    case LocalKind::variable: return l.has(Local::constinit_) ? CONSTINIT : std::string_view {};
    default: return {};
    }
}

} // namespace mcxx::frontend::ast
