// mcxx.frontend:bodies -- the full parse (M3.0): what is inside declarations, read into :ast's tree, over
// the declarations :syntax found.
//
//   const auto syntax = mcxx::frontend::parse(text, { .file = path });
//   const auto tree = mcxx::frontend::parse_bodies(syntax, { .imported = &imported });
//   for (const auto& root : tree.roots) ... root.role, tree.statement(root.node)
//
// It is a mode, not the outline's way: parse() and symbols() read no body (mcppls's fast path pays
// nothing for this), and what this reads they found -- each function's body, each initializer, default
// argument, enumerator value and static_assert is a root (Syntax::parts), read as the grammar says.
//
// Two interfaces keep the parser from deciding by itself what C++ makes depend on the rest of the program:
//
//  - NameOracle (semantic feedback): is this name a type, a template, a value? The parser asks when
//    code reads two ways -- `a < b > (c)`, `T * x;`, `(T) - x`, `f<int>(x)`. Its own scopes answer first
//    (what the body declared, the parameters, the template parameters), then the oracle; the file's lookup
//    and imports (mcxx.frontend:lookup) are the default one. Where none knows, the shape of the code
//    decides, by the rules the declaration-level parser uses. Which one decided is recorded
//    (ast::Decision, Tree::stats).
//  - SyntaxExtension (MC4's language extension point, plan §4): syntax a generation of the language
//    adds -- C++26's reflection, contracts, expansion statements, pack indexing -- is read by an extension,
//    not by the core, which reads C++23. cpp26_syntax() is the one the front end carries until
//    plugins/lang/cpp26 supplies its own; BodyOptions::extensions { } is C++23 alone.
export module mcxx.frontend:bodies;

import std;
import mcxx.msa;
import :lex;
import :preprocess;
import :syntax;
import :ast;
import :lookup;

export namespace mcxx::frontend {

// What a name is, as far as it is known.
enum class NameClass : std::uint8_t {
    unknown,
    type,               // a class, enum, alias, typedef, template parameter that is a type
    class_template,
    alias_template,
    function_template,
    variable_template,
    concept_,
    value,              // a variable, function, enumerator, parameter, non-type template parameter
    namespace_,
};
inline bool type_like(NameClass c) { return c == NameClass::type; }
inline bool template_like(NameClass c) {
    return c == NameClass::class_template || c == NameClass::alias_template || c == NameClass::function_template ||
           c == NameClass::variable_template || c == NameClass::concept_;
}
// A template that names a type once its arguments are given.
inline bool type_template(NameClass c) { return c == NameClass::class_template || c == NameClass::alias_template; }

struct NameQuery {
    std::string_view name;       // as written: "x", "std::vector", "::f", "ns::T" (template arguments left out)
    std::uint32_t at { 0 };      // the token the name begins at: where it is looked up from
};

struct NameAnswer {
    NameClass what { NameClass::unknown };
    ast::Basis basis { ast::Basis::feedback };   // lookup for the file's own, feedback for a host's
};

// Semantic feedback to the parser. Answers must be sure: unknown when not (the parser then decides by
// the code's shape and says so).
class NameOracle {
public:
    virtual ~NameOracle() = default;
    virtual NameAnswer classify(const NameQuery& query) = 0;
};

// The oracle over the file's own declarations and its imports (name lookup, :lookup); the one used when
// BodyOptions::oracle is none. It outlives nothing: `syntax` and `imported` must outlive it.
std::unique_ptr<NameOracle> lookup_oracle(const Syntax& syntax, const Imported& imported);

// ---- the language extension point ----

// What an extension may do at the parser's cursor. Everything it makes goes into the tree by make().
class SyntaxContext {
public:
    virtual ~SyntaxContext() = default;
    // Tokens: `ahead` counts from the cursor.
    virtual const PpToken& token(std::size_t ahead = 0) const = 0;
    virtual std::uint32_t position() const = 0;                       // the cursor's token index
    virtual bool adjacent(std::size_t ahead) const = 0;               // no space between token ahead - 1 and token ahead
    virtual void advance(std::size_t count = 1) = 0;
    virtual bool accept(Kind kind) = 0;
    virtual bool accept_word(std::string_view word) = 0;
    virtual bool expect(Kind kind, std::string_view what) = 0;        // false, and a failure, when it is not there
    virtual void fail(std::string message) = 0;                       // the construct cannot be read: the parser recovers
    virtual bool failed() const = 0;
    // The grammar's own productions.
    virtual ast::ExprId assignment_expression() = 0;
    virtual ast::ExprId constant_expression() = 0;                    // a conditional-expression
    virtual ast::Handle type_or_expression() = 0;                     // a type-id or an expression, as the grammar tells them apart
    virtual ast::TypeId type_id() = 0;
    virtual ast::StmtId statement() = 0;
    virtual ast::StmtId compound_statement() = 0;
    virtual ast::StmtId for_statement(bool expansion, std::uint32_t first) = 0;   // from `for` (the cursor), `expansion`: `template for`
    virtual ast::NameId qualified_name() = 0;
    // The tree.
    virtual ast::ExprId make(ast::Expr node) = 0;
    virtual ast::StmtId make(ast::Stmt node) = 0;
    virtual ast::TypeId make(ast::TypeNode node) = 0;
    virtual ast::LocalId make(ast::Local node) = 0;
    virtual std::uint32_t first_token(ast::Handle node) const = 0;     // the first token of a node made
    virtual ast::Span32 list(std::span<const ast::Handle> handles) = 0;
    virtual std::uint32_t contract(ast::Contract c) = 0;              // Tree::contracts' index
    // A name for a while: the result name of `post(r: ...)`.
    virtual void push_scope() = 0;
    virtual void pop_scope() = 0;
    virtual void declare(std::uint32_t name_token, NameClass what) = 0;
};

// A function's specifiers an extension read: contracts, and the reason of `= delete("why")`.
struct FunctionSpecifiers {
    std::vector<std::uint32_t> contracts;   // Tree::contracts indices
    ast::ExprId delete_reason;
};

// Syntax a generation of the language adds. Each hook is asked at the place it names, with the cursor at
// the construct's first token; true when it read one (the cursor past it, what it made in the out
// parameter). A failure inside is reported with ctx.fail().
class SyntaxExtension {
public:
    virtual ~SyntaxExtension() = default;
    virtual std::string_view name() const = 0;
    // The MC1 feature ids it provides: a profile that denies one gets a gate diagnostic where it is used.
    virtual std::span<const std::string_view> features() const = 0;
    virtual bool expression_primary(SyntaxContext&, ast::ExprId&) const { return false; }
    // After a postfix-expression `lhs`: `lhs...[n]`.
    virtual bool expression_postfix(SyntaxContext&, ast::ExprId& lhs) const { return false; }
    virtual bool statement(SyntaxContext&, ast::StmtId&) const { return false; }
    // A type-specifier: `typename [: r :]`, `[: r :]`.
    virtual bool type_specifier(SyntaxContext&, ast::TypeId&) const { return false; }
    // After a type-name `T` (a name): `T...[n]`.
    virtual bool type_postfix(SyntaxContext&, ast::TypeId& lhs) const { return false; }
    // A nested-name-specifier's first component: `[: r :]::`.
    virtual bool name_component(SyntaxContext&, ast::NameComponent&) const { return false; }
    // After a function's declarator and its qualifiers: `pre(...)`, `post(r: ...)`.
    virtual bool function_specifier(SyntaxContext&, FunctionSpecifiers&) const { return false; }
    // A declaration (a member's, a namespace's, a block's): `consteval { ... }`.
    virtual bool declaration(SyntaxContext&, ast::LocalId&) const { return false; }
    // A function body's `= delete(` ... `)`: the cursor after `delete`, at `(`.
    virtual bool delete_reason(SyntaxContext&, ast::ExprId&) const { return false; }
};

// C++26's syntax -- `^^`, `[: :]`, `template for`, `pre`/`post`, `contract_assert`, `consteval { }`, pack
// indexing, `= delete("why")` -- as an extension: what plugins/lang/cpp26 will supply (A3.0.3). Its features
// are those it has in the C++26 plugin's catalog.
std::shared_ptr<const SyntaxExtension> cpp26_syntax();

struct BodyOptions {
    const Imported* imported { nullptr };       // what the file's imports bring in, for the default oracle
    NameOracle* oracle { nullptr };             // feedback: none, the default (lookup_oracle over `imported`)
    // The syntax beyond C++23 that is read; by default C++26's.
    std::vector<std::shared_ptr<const SyntaxExtension>> extensions { cpp26_syntax() };
    bool record_decisions { false };            // keep every Decision in Tree::decisions (the counts are always kept)
    bool roots_only_bodies { false };           // read function bodies only (not initializers, defaults, enumerators)
};

// The tree of every function body and initializer of `syntax`. The syntax must outlive the tree.
ast::Tree parse_bodies(const Syntax& syntax, const BodyOptions& options = {});

// One root's text read as `role` wants (a body: a statement; others an expression): for tools and tests
// that have a snippet and no file. The text is wrapped as the part of a function `void f()` etc.
// Returns a tree with one root and its own Syntax in `owned`.
struct Fragment {
    std::shared_ptr<Syntax> owned;
    ast::Tree tree;
};
Fragment parse_fragment(std::string_view text, const BodyOptions& options = {});

} // namespace mcxx::frontend
