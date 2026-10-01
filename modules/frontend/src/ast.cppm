// mcxx.frontend:ast -- the syntax tree of what is inside declarations: statements, expressions, types,
// names and the declarations a block or a class holds (M3.0). Declaration-level structure stays in
// :syntax (Syntax::declarations, the outline); this is what bodies, initializers and the rest are made of.
//
// One tree per file (Tree), nodes in arenas by sort, each reached by a handle:
//
//   ast::Handle h = tree.roots[0].node;                       // a function body: a statement
//   if (const auto* s = tree.statement(h)) ... s->kind, s->first, s->last
//   tree.get(h)                                               // any handle back to its node (A3.0.4)
//
// Handles are indices, stable for the tree's life and the same for the same file and options: a
// declaration, a type, an expression is named by (sort, index), which is what reflection (`^^x`) and a
// contract assertion need to refer to an entity, and what a later pass keeps instead of a pointer.
// Every node has the tokens it spans ([first, last], inclusive indices of Syntax::pp.tokens), so a node's
// range is as Clang gives it (token_range()).
//
// Nodes are plain structs of a kind and a few slots (operands, a name, a type, a list); what a slot
// means is given with each kind below. Lists are spans of Tree::links.
//
// Where C++ needs to know what a name is to read it (a type or a value; `<` an argument list or less-
// than), the parser asks (:feedback) and records which decided: Tree::decisions, Tree::stats.
// Each construct that has a feature id in MC1 says it (:features, feature_of()).
export module mcxx.frontend:ast;

import std;
import mcxx.msa;
import :lex;
import :preprocess;
import :syntax;

export namespace mcxx::frontend::ast {

inline constexpr std::uint32_t NONE { 0xffffffffu };

// What a handle names. `declaration` is an index of Syntax::declarations (the outline's, by shape);
// `local` a declaration the full parse read (a block's, a class's members, a parameter, a lambda's
// capture, ...), with `Local::outline` where the outline has the same one.
enum class Sort : std::uint8_t { declaration, local, statement, expression, type, name };

struct Handle {
    Sort sort { Sort::expression };
    std::uint32_t index { NONE };
    explicit operator bool() const { return index != NONE; }
    bool operator==(const Handle&) const = default;
    auto operator<=>(const Handle&) const = default;
};

template <Sort S>
struct Id {
    std::uint32_t index { NONE };
    static constexpr Sort sort { S };
    explicit operator bool() const { return index != NONE; }
    Handle handle() const { return { S, index }; }
    bool operator==(const Id&) const = default;
};
using DeclId = Id<Sort::declaration>;
using LocalId = Id<Sort::local>;
using StmtId = Id<Sort::statement>;
using ExprId = Id<Sort::expression>;
using TypeId = Id<Sort::type>;
using NameId = Id<Sort::name>;

// A list: [begin, begin + count) of Tree::links.
struct Span32 {
    std::uint32_t begin { 0 };
    std::uint32_t count { 0 };
};

// The operator an expression applies; for a named cast, which one.
enum class Op : std::uint8_t {
    none,
    // unary
    plus, minus, not_, complement, deref, address_of, pre_increment, pre_decrement, post_increment, post_decrement,
    // binary
    multiply, divide, remainder, add, subtract, shift_left, shift_right, less, greater, less_equal, greater_equal, spaceship, equal, not_equal,
    bit_and, bit_xor, bit_or, logical_and, logical_or, comma, member_pointer_dot, member_pointer_arrow,
    // assignment
    assign, multiply_assign, divide_assign, remainder_assign, add_assign, subtract_assign, shift_left_assign, shift_right_assign, and_assign,
    xor_assign, or_assign,
    // named casts
    static_cast_, dynamic_cast_, const_cast_, reinterpret_cast_,
    // operator functions that are not one of the above (`operator()`, `operator[]`, `operator new`, ...)
    call, subscript, new_, delete_, new_array, delete_array, co_await_, arrow,
    // GNU unary
    real_part, imag_part,
};
std::string_view spelling(Op op);

enum class ExprKind : std::uint8_t {
    error,              // tokens [first, last] that could not be read
    // literals: `token` the (first) token; a string's [first, last] are its concatenated pieces
    integer, floating, character, string, boolean, null_pointer,
    this_,
    id,                 // name: an id-expression; what it names is for lookup
    paren,              // a: ( a )
    unary,              // op, a; flag postfix
    binary,             // op, a, b (including `,` and `.*`, `->*`)
    assign,             // op (assign or compound), a, b
    conditional,        // a ? b : c; b none for GNU `a ?: c`
    call,               // a(list...)
    subscript,          // a[list...] (C++23: any number, also none)
    member,             // a.name, a->name; flag arrow; `template` is in the name
    cast_named,         // op, type, a: static_cast<type>(a) ...
    cast_c,             // type, a: (type) a; a an init_list for a compound literal
    cast_functional,    // type, list: type(list...), type{list...} (flag brace); a name's call is `call`
    sizeof_,            // type or a; flag `alignof` for alignof
    sizeof_pack,        // name: sizeof...(P)
    noexcept_,          // a
    typeid_,            // type or a
    new_,               // aux: News; flag global, array
    delete_,            // a; flag global, array
    throw_,             // a, or none
    co_await_, co_yield_,   // a
    lambda,             // aux: Lambdas
    requires_,          // aux: Requireses
    fold,               // aux: Folds
    pack_expansion,     // a...
    init_list,          // list: its elements, each an expression (designated for `.x = e`); a braced-init-list
    designated,         // aux, list.count: its designators (Tree::designators[aux, aux + count)), a: the value
    statement_expression,   // stmt: GNU ({ ... })
    builtin,            // token: its name (a __builtin_ or a type trait); list: its arguments, each a type or an expression
    label_address,      // token: GNU &&label
    reflect,            // C++26 `^^ operand`: list[0] a type, an expression or a name
    splice,             // C++26 `[: a :]`; name: a template-id's arguments follow, if any
    pack_index,         // C++26 `a...[b]`
    member_init,        // a constructor's mem-initializer: name (the member or base), list: its arguments (flag brace: `{}`; pack: `...`)
};

struct Expr {
    std::uint32_t first { 0 }, last { 0 };
    ExprKind kind { ExprKind::error };
    Op op { Op::none };
    std::uint16_t flags { 0 };
    ExprId a, b, c;
    TypeId type;
    NameId name;
    StmtId stmt;
    Span32 list;
    std::uint32_t token { 0 };   // the literal, the operator, the builtin's name
    std::uint32_t aux { NONE };

    enum Flag : std::uint16_t {
        postfix = 1,
        arrow = 2,
        global = 4,        // a leading `::` (new, delete)
        array = 8,         // new[], delete[]
        brace = 16,        // T{...}
        user_defined = 32, // a literal with a suffix
        alignof_ = 64,
        parenthesized = 128,   // written in parentheses (not a `paren` node: that one is kept)
        pack = 256,        // written with a trailing `...` that is part of the node (a fold's, a type pack's)
        implicit_template_args = 512,
    };
    bool has(Flag f) const { return (flags & f) != 0; }
};

enum class StmtKind : std::uint8_t {
    error,              // tokens [first, last] skipped
    null,               // ;
    compound,           // list: statements
    expression,         // e1
    declaration,        // list: LocalIds (one declaration's declarators)
    if_,                // init (a), condition (e1 or local), then (b), else (c); flags constexpr, consteval, negated
    switch_,            // init (a), condition (e1 or local), body (b)
    while_,             // condition (e1 or local), body (b)
    do_,                // body (a), condition (e1)
    for_,               // init (a), condition (e1 or local), increment (e2), body (b)
    range_for,          // init (a), the variable (local), range (e1), body (b); flag expansion: `template for`
    return_,            // e1
    break_, continue_,
    goto_,              // token: the label; or e1 (computed `goto *e1`)
    labeled,            // token: the label, a
    case_,              // e1 (e2: GNU range `a ... b`), a
    default_,           // a
    try_,               // a: the block, list: handlers
    handler,            // local: the exception declaration (none for `...`), a: the block
    co_return_,         // e1
    asm_,               // tokens [first, last]
    static_assert_,     // e1: the condition, e2: the message
    contract_assert,    // e1
    consteval_block,    // a
    attributed,         // a: the statement; attributes: Tree::attributes of this node
    function_body,      // a function's body: list: its mem-initializers (member_init), a: the compound or try; flags defaulted, deleted, pure; e1: delete's reason
};

struct Stmt {
    std::uint32_t first { 0 }, last { 0 };
    StmtKind kind { StmtKind::error };
    std::uint16_t flags { 0 };
    StmtId a, b, c;
    ExprId e1, e2, e3;
    LocalId local;
    Span32 list;
    std::uint32_t token { NONE };

    enum Flag : std::uint16_t {
        constexpr_ = 1, consteval_ = 2, negated = 4, expansion = 8, volatile_ = 16, function_try = 32, defaulted = 64, deleted = 128, pure = 256,
    };
    bool has(Flag f) const { return (flags & f) != 0; }
};

enum class Builtin : std::uint8_t {
    none, void_, bool_, char_, signed_char, unsigned_char, wchar, char8, char16, char32, short_, unsigned_short, int_, unsigned_int, long_,
    unsigned_long, long_long, unsigned_long_long, float_, double_, long_double, int128, unsigned_int128, other,   // other: _Float16, __bf16, ... a spelling
};
std::string_view spelling(Builtin b);

enum class TypeKind : std::uint8_t {
    error,
    builtin,            // builtin; flags complex; token: its first specifier
    named,              // name: a type-name (class, enum, alias, typedef, template parameter, a template-id)
    elaborated,         // key (token): class, struct, union, enum, typename; name; local: the definition when one is written here
    decltype_,          // expr; flag `auto`: decltype(auto)
    auto_,              // a placeholder; name: its type-constraint (`C auto`) when it has one
    pointer,            // inner
    lvalue_ref, rvalue_ref,   // inner
    member_pointer,     // inner, name: the class (`C::*`)
    array,              // inner: the element, expr: the bound (none: `[]`); flag vla `[*]`
    function,           // inner: the return type, list: parameters (LocalIds), expr: noexcept(expr), contracts
    pack_expansion,     // inner...
    pack_index,         // C++26 `name...[expr]` as a type: name, expr
    splice,             // C++26 `[: expr :]` as a type (typename allowed)
    atomic,             // _Atomic(inner)
    typeof_,            // __typeof__(expr or type): type or expr
};

struct TypeNode {
    std::uint32_t first { 0 }, last { 0 };
    TypeKind kind { TypeKind::error };
    Builtin builtin { Builtin::none };
    std::uint16_t flags { 0 };
    TypeId inner;
    NameId name;
    ExprId expr;
    LocalId local;
    Span32 list;
    std::uint32_t token { NONE };

    enum Flag : std::uint16_t {
        const_ = 1, volatile_ = 2, restrict_ = 4,
        c_variadic = 8,       // function: `...`
        const_method = 16, volatile_method = 32, lvalue_method = 64, rvalue_method = 128,   // function: cv and ref qualifiers
        trailing = 256,       // function: written with a trailing return type
        noexcept_ = 512,      // function: `noexcept` (expr: its condition, none: unconditional)
        decltype_auto = 1024,
        vla = 2048,
        complex = 4096,
        dynamic_throw = 8192,   // function: `throw(...)`
    };
    bool has(Flag f) const { return (flags & f) != 0; }
};

// A name as written: [::] (component ::)* component.
struct NameComponent {
    enum class Form : std::uint8_t {
        identifier,
        operator_function,    // op
        conversion_function,  // type
        literal_operator,     // `operator "" _x`: token the suffix
        destructor,           // ~ identifier (token the identifier) or ~decltype(...) (type)
        decltype_,            // decltype(expr) before `::`
        splice,               // [: expr :]
        template_keyword_only,   // never produced; keeps the enum's tail a place for extension forms
    };
    Form form { Form::identifier };
    Op op { Op::none };
    std::uint16_t flags { 0 };
    std::uint32_t token { 0 };       // the identifier, or `operator`, `~`
    std::uint32_t last { 0 };
    Span32 arguments;                // handles: type, expression or (a template template argument) name
    TypeId type;
    ExprId expr;

    enum Flag : std::uint16_t { template_keyword = 1, has_arguments = 2, pack = 4 };
    bool has(Flag f) const { return (flags & f) != 0; }
};

struct Name {
    std::uint32_t first { 0 }, last { 0 };
    bool global { false };           // a leading `::`
    Span32 components;               // into Tree::components
};

enum class LocalKind : std::uint8_t {
    variable,           // type, init, flags; a condition's, a for-init's, a catch's too
    parameter,          // type, init: the default argument; flag pack, explicit_object
    field,              // type, init: the default member initializer, width
    function,           // type: a function type; body; entity says: function, method, constructor, destructor, conversion
    type_alias,         // typedef or using: type
    class_,             // entity: class_, struct_, union_; name; members; bases
    enum_,              // name; type: the underlying type; members: enumerators; flag scoped
    enumerator,         // init: its value
    structured_binding, // the group `[a, b]`: members: the bindings; init; flags; ref kind
    binding,            // a name a structured binding introduces
    capture,            // a lambda's: `x`, `&x`, `x = e` (init), `this`, `*this`, `...x`; type none
    template_parameter, // entity template_parameter; form: type, value (type), template (params); init: the default
    using_declaration,  // name: what it names
    using_directive,    // name
    using_enum,         // name or type
    namespace_,         // name, members
    namespace_alias,    // name, target
    static_assert_,     // init: the condition, width: the message
    access,             // token: public, private, protected
    friend_,            // type (friend class X; friend T;) or a function (local)
    asm_,               // tokens
    empty,              // a stray `;`
    concept_,           // name, template parameters, init: the constraint
    consteval_block,    // body
    deduction_guide,    // name, type
    linkage,            // extern "C" { members }
    export_block,       // export { members }
    template_,          // wraps `local`: params and requires-clause
    error,
};

struct Contract {
    bool post { false };              // post(...); else pre(...)
    std::uint32_t result { NONE };    // post(r: ...): the identifier's token
    ExprId condition;
    std::uint32_t first { 0 }, last { 0 };
};

struct Base {
    TypeId type;
    std::uint8_t access { 0 };        // 0 default, 1 public, 2 protected, 3 private
    bool virtual_ { false };
    bool pack { false };
};

struct Local {
    std::uint32_t first { 0 }, last { 0 };
    LocalKind kind { LocalKind::error };
    msa::Kind entity { msa::Kind::unknown };
    std::uint32_t flags { 0 };
    NameId name;
    std::uint32_t name_token { NONE };      // the declared name's token (none: unnamed)
    TypeId type;
    ExprId init;                            // initializer: the expression, an init_list for braces, a paren's list is `init_list` with flag
    ExprId width;                           // a bit-field's width, a static_assert's message
    StmtId body;
    LocalId target;                         // template_: the declaration it templates
    Span32 members;                         // LocalIds: a class's, enum's, namespace's, a function's parameters, a binding group's names
    Span32 params;                          // LocalIds: a template's parameters
    Span32 bases;                           // Tree::bases indices
    Span32 contracts;                       // a function's pre and post: Tree::contracts[begin, begin + count)
    ExprId constraint;                      // a requires-clause (a template's, a function's trailing one)
    std::int32_t outline { -1 };            // the Syntax::declarations index with the same name token, where the outline has it
    std::uint8_t form { 0 };                // template_parameter: 0 type, 1 value, 2 template; capture: 0 a name, 1 `this`, 2 `*this`, 3 a default (`=`, `&`); access: 1 public, 2 protected, 3 private
    std::uint32_t token { NONE };

    enum Flag : std::uint32_t {
        static_ = 1u << 0, extern_ = 1u << 1, thread_local_ = 1u << 2, register_ = 1u << 3, mutable_ = 1u << 4, typedef_ = 1u << 5,
        inline_ = 1u << 6, constexpr_ = 1u << 7, consteval_ = 1u << 8, constinit_ = 1u << 9, virtual_ = 1u << 10, explicit_ = 1u << 11,
        friend_ = 1u << 12, pure = 1u << 13, defaulted = 1u << 14, deleted = 1u << 15, override_ = 1u << 16, final_ = 1u << 17,
        pack = 1u << 18,             // `T... x`; a binding pack `auto [...xs]`
        explicit_object = 1u << 19,  // C++23 `this T self`
        scoped = 1u << 20,           // enum class
        copy_init = 1u << 21,        // `= e`
        direct_init = 1u << 22,      // `(e...)`
        brace_init = 1u << 23,       // `{...}`
        exported = 1u << 24,
        init_capture_ref = 1u << 25, // `&x = e`
        by_ref = 1u << 26,           // a capture `&x`, `&`; a binding group `auto& [a]`
        by_rvalue_ref = 1u << 27,    // a binding group `auto&& [a]`
        default_capture = 1u << 28,  // `=` or `&` alone
        deleted_with_reason = 1u << 29,   // = delete("why"): the reason is `init`
        anonymous = 1u << 30,
        in_condition = 1u << 31,
    };
    bool has(Flag f) const { return (flags & f) != 0; }
};

struct Attribute {
    std::uint32_t first { 0 }, last { 0 };     // the whole `[[...]]`, alignas(...), __attribute__((...))
    Handle owner;
};

struct Designator {
    std::uint32_t first { 0 }, last { 0 };
    bool field { true };         // `.name`; else `[index]`
    std::uint32_t token { NONE };
    ExprId index;
    ExprId end;                  // GNU [a ... b]
};

struct News {                    // new_ (aux)
    Span32 placement;            // expressions
    TypeId type;
    ExprId bound;                // new T[bound]
    ExprId init;                 // an init_list: paren (flag brace unset) or braces
    bool paren_type { false };   // `new (T)`
    bool initialized { false };
    bool brace_init { false };
};

struct LambdaInfo {              // lambda (aux)
    Span32 captures;             // LocalIds
    Span32 template_params;      // LocalIds
    ExprId requires_;            // the clause after the template parameters
    Span32 params;               // LocalIds
    TypeId function;             // the function type, with the trailing return type, noexcept and qualifiers
    ExprId trailing_requires;
    StmtId body;
    bool has_params { false };
    bool mutable_ { false }, constexpr_ { false }, consteval_ { false }, static_ { false };
    std::uint8_t capture_default { 0 };   // 0 none, 1 `=`, 2 `&`
};

struct Requirement {
    enum class Form : std::uint8_t { simple, type, compound, nested };
    Form form { Form::simple };
    ExprId expr;                 // simple, compound, nested: the expression
    TypeId type;                 // type: the type; compound: the `-> constraint` type
    NameId constraint;           // compound: the type-constraint's concept name when `-> C<T>`
    bool noexcept_ { false };
    std::uint32_t first { 0 }, last { 0 };
};

struct RequiresInfo {            // requires_ (aux)
    Span32 params;               // LocalIds
    bool has_params { false };
    Span32 requirements;         // Tree::requirements indices
};

struct FoldInfo {                // fold (aux)
    ExprId pattern;              // the pack expression
    ExprId init;                 // the other operand of a binary fold
    Op op { Op::none };
    bool left { false };         // ( ... op e ) and ( init op ... op e ): left; else right
    bool binary { false };
};

// A decision between readings of the code that needs to know what a name is: what the parser asked
// (:feedback), what it chose, and who told it.
enum class Basis : std::uint8_t {
    syntax,    // the grammar alone: a keyword, a punctuator
    scope,     // a name the parse itself declared (a local, a parameter, a template parameter, a lambda's capture)
    lookup,    // name lookup (the file's declarations and its imports: mcxx.frontend:lookup)
    feedback,  // the host's oracle (BodyOptions::oracle), which answered
    shape,     // none knew: the code's shape decided (the same rules the declaration-level parser uses)
};
enum class Ambiguity : std::uint8_t {
    name,                    // what a name is: a type, a template, a value
    template_arguments,      // `<` after a name: arguments or less-than
    declaration_or_expression,
    cast_or_parenthesized,   // ( T ) x
    parameters_or_initializer,   // `T x(...)`
    type_or_expression_argument, // a template argument, sizeof(x), typeid(x)
};
enum class Resolution : std::uint8_t {
    type, class_template, alias_template, function_template, variable_template, concept_, value, namespace_, unknown,
    arguments, less_than, declaration, expression, cast, parenthesized, parameters, initializer, type_argument, expression_argument,
};
std::string_view to_string(Basis b);
std::string_view to_string(Ambiguity a);
std::string_view to_string(Resolution r);

struct Decision {
    std::uint32_t token { 0 };   // where: the name's first token, the `(`, the `<`
    Ambiguity what { Ambiguity::name };
    Resolution chose { Resolution::unknown };
    Basis basis { Basis::syntax };
};

// What a part of the file is for, to the full parse: a root of the tree.
struct Root {
    enum class Role : std::uint8_t {
        body,                // a function's body (Syntax::Part::Role::function_body): a statement
        initializer,         // a variable's or field's: an expression
        default_argument,
        enumerator_value,
        bit_width,
        static_assert_,      // a namespace- or class-scope static_assert: a statement
        constraint,          // a trailing requires-clause
        noexcept_spec,
        concept_definition,
        contract,            // a pre or post specifier: an expression
    };
    Role role { Role::body };
    std::int32_t declaration { -1 };   // the outline's declaration it belongs to
    Handle node;                       // a statement (body, static_assert_) or an expression
    std::uint32_t first { 0 }, last { 0 };
};

struct Stats {
    std::size_t parts { 0 };       // roots read
    std::size_t failed { 0 };      // roots that left a diagnostic
    std::size_t errors { 0 };      // error nodes made (statements and expressions that could not be read)
    std::size_t decisions { 0 };
    std::array<std::size_t, 5> by_basis {};   // decisions by Basis
    std::size_t speculations { 0 };           // tentative parses started
    std::size_t rewinds { 0 };                // of them abandoned
};

// What a handle gets back (A3.0.4): the node, by its sort; the others null.
struct View {
    const Expr* expression { nullptr };
    const Stmt* statement { nullptr };
    const TypeNode* type { nullptr };
    const Local* local { nullptr };
    const Name* name { nullptr };
    const Declaration* declaration { nullptr };
};

struct Tree {
    const Syntax* syntax { nullptr };      // whose tokens the nodes span; it must outlive the tree
    std::vector<Expr> expressions;
    std::vector<Stmt> statements;
    std::vector<TypeNode> types;
    std::vector<Local> locals;
    std::vector<Name> names;
    std::vector<NameComponent> components;
    std::vector<Handle> links;                  // every list's elements
    std::vector<News> news;
    std::vector<LambdaInfo> lambdas;
    std::vector<RequiresInfo> requires_infos;
    std::vector<Requirement> requirements;
    std::vector<FoldInfo> folds;
    std::vector<Designator> designators;
    std::vector<Base> bases;
    std::vector<Contract> contracts;
    std::vector<Attribute> attributes;
    std::vector<Root> roots;                    // in the order of the file
    std::vector<Diagnostic> diagnostics;        // where the full parse could not follow the code, and skipped
    std::vector<Decision> decisions;            // when BodyOptions::record_decisions
    Stats stats;

    const Expr* expression(Handle h) const { return h.sort == Sort::expression && h.index < expressions.size() ? &expressions[h.index] : nullptr; }
    const Stmt* statement(Handle h) const { return h.sort == Sort::statement && h.index < statements.size() ? &statements[h.index] : nullptr; }
    const TypeNode* type(Handle h) const { return h.sort == Sort::type && h.index < types.size() ? &types[h.index] : nullptr; }
    const Local* local(Handle h) const { return h.sort == Sort::local && h.index < locals.size() ? &locals[h.index] : nullptr; }
    const Name* name(Handle h) const { return h.sort == Sort::name && h.index < names.size() ? &names[h.index] : nullptr; }
    const Expr& operator[](ExprId id) const { return expressions[id.index]; }
    const Stmt& operator[](StmtId id) const { return statements[id.index]; }
    const TypeNode& operator[](TypeId id) const { return types[id.index]; }
    const Local& operator[](LocalId id) const { return locals[id.index]; }
    const Name& operator[](NameId id) const { return names[id.index]; }

    // Any handle back to its node.
    View get(Handle h) const;
    std::span<const Handle> list(Span32 s) const { return std::span<const Handle> { links }.subspan(s.begin, s.count); }
    std::span<const NameComponent> parts(const Name& n) const { return std::span<const NameComponent> { components }.subspan(n.components.begin, n.components.count); }

    // The first and last token of any node (a declaration's: the outline's), and its range as Clang gives it.
    std::pair<std::uint32_t, std::uint32_t> tokens(Handle h) const;
    msa::Range range(Handle h) const;
    // The source text of tokens [first, last], as the tokens spell it (a macro's expansion included).
    std::string text(std::uint32_t first, std::uint32_t last) const;
    std::string text(Handle h) const;
};

// The tree as one line of text, a node as `(kind detail child...)`: for tests and for looking at what a
// parse read. Expressions, statements, types, names and locals all.
std::string dump(const Tree& tree, Handle h);

// A node's children, in the order they are written (names and types of expressions among them): what a pass
// that visits every node needs, whatever the kind.
std::vector<Handle> children(const Tree& tree, Handle h);

// What is wrong with the tree's structure, as text; none when it is sound: every child exists, every node's
// tokens are its own and inside its parent's (a derived type, written around its inner type, aside), each root inside its part.
std::vector<std::string> validate(const Tree& tree);

std::string_view to_string(ExprKind k);
std::string_view to_string(StmtKind k);
std::string_view to_string(TypeKind k);
std::string_view to_string(LocalKind k);

// The MC1 feature a node is (an id the catalog has, or proposes), or "" when it is core C++ no profile
// asks about. See :features.
std::string_view feature_of(const Expr& e);
std::string_view feature_of(const Stmt& s);
std::string_view feature_of(const TypeNode& t);
std::string_view feature_of(const Local& l);

// A feature id the syntax tree can name. `registered`: the id is in MC1's catalog today (mc++.iso, the
// C++26 and C++29 packages); else it is proposed, in the catalog's own naming (`c++NN:` for a standard's
// feature by the standard that added it, `ext:` for an extension), for MC1 to take up.
struct FeatureInfo {
    std::string_view id;
    bool registered { false };
    std::string_view construct;   // what in the grammar it is
};
std::span<const FeatureInfo> syntax_features();

} // namespace mcxx::frontend::ast
