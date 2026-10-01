// mcxx.frontend:bodyparser (not exported) -- the full parse's working state: one recursive-descent parser
// over the tokens of a part, with the tentative parsing the grammar's ambiguities need. Declarations only;
// the definitions are by concern: bodies.cpp (the cursor, the tree, the oracle's use, the driver),
// parse_name.cpp (names and template arguments), parse_expr.cpp (expressions), parse_lambda.cpp (lambdas,
// requires), parse_type.cpp (decl-specifiers, declarators, type-ids), parse_decl.cpp (block and class
// declarations), parse_stmt.cpp (statements), syntax26.cpp (the C++26 extension). MC5 §8.
//
// Failure: no exceptions. A production that cannot go on calls fail(); the parser's failed_ then makes every
// production return at once, and the nearest recovery point -- a statement, a part -- turns it into a
// diagnostic and an error node, skipping to a sync point. A tentative parse (attempt) rewinds the cursor,
// the tree and the scopes to where it began when it fails or is not wanted.
module mcxx.frontend:bodyparser;

import std;
import mcxx.msa;
import mcxx.base;
import :lex;
import :preprocess;
import :syntax;
import :ast;
import :bodies;

namespace mcxx::frontend::bodies {

using namespace ast;

constexpr std::string_view TRACE { "frontend.syntax" };
constexpr std::size_t NPOS { static_cast<std::size_t>(-1) };

inline bool is_string(Kind k) {
    return k == Kind::string_literal || k == Kind::wide_string_literal || k == Kind::utf8_string_literal || k == Kind::utf16_string_literal ||
           k == Kind::utf32_string_literal;
}
inline bool at_gt_kind(Kind k) { return k == Kind::greater || k == Kind::greatergreater || k == Kind::greaterequal || k == Kind::greatergreaterequal; }
inline std::string_view last_component(std::string_view q) {
    const auto at { q.rfind("::") };
    return at == std::string_view::npos ? q : q.substr(at + 2);
}
// The words that are a simple-type-specifier by themselves or in a sequence.
inline bool is_type_keyword(std::string_view w) {
    static constexpr std::string_view WORDS[] { "void", "bool", "char", "char8_t", "char16_t", "char32_t", "wchar_t", "short", "int", "long", "signed",
                                                "unsigned", "float", "double", "__int128", "__int128_t", "__uint128_t", "_Float16", "__fp16", "__bf16",
                                                "_Complex", "_Bool", "__signed", "__signed__", "__unsigned", "_Float32", "_Float64", "_Float128",
                                                "__float128", "_Float32x", "_Float64x", "_Float128x", "__ibm128", "_BitInt", "_ExtInt" };
    return std::ranges::contains(WORDS, w);
}

// Where a declaration is read: what may follow its specifiers, what its declarator may be.
enum class Site : std::uint8_t { block, class_, namespace_, parameter, condition, for_init, template_parameter, exception, lambda_capture };

// The decl-specifier-seq of a declaration.
struct DeclSpec {
    std::uint32_t first { 0 }, last { 0 };
    std::uint32_t flags { 0 };           // Local::Flag bits: storage class, constexpr, inline, virtual, explicit, friend, typedef, ...
    TypeId type;                         // none when no type is written
    LocalId defined;                     // a class or enum this declaration defines
    msa::Kind defined_kind { msa::Kind::unknown };
    bool placeholder { false };          // auto, decltype(auto), a constrained placeholder
    bool has_type { false };
    std::uint32_t attributes_first { NONE }, attributes_last { NONE };
};

enum class Mode : std::uint8_t { named, abstract, either, new_declarator };

struct Declarator {
    std::uint32_t first { 0 }, last { 0 };
    NameId name;
    std::uint32_t name_token { NONE };
    TypeId type;                          // the declared type, the specifiers' type applied
    bool ok { false };
    bool function { false };              // its innermost suffix is a parameter list
    bool pack { false };                  // `T... x`
    bool parenthesized_name { false };    // `T (x)`
    Span32 params;                        // the function's parameters (Local handles), when function
    std::vector<std::uint32_t> contracts; // Tree::contracts indices
};

struct NameInfo {
    NameClass what { NameClass::unknown };
    ast::Basis basis { ast::Basis::shape };
    bool dependent { false };             // qualified by something that depends on a template parameter
};

// What a name stands for in the scopes the parse itself keeps.
struct Entry {
    std::string_view name;
    NameClass what { NameClass::unknown };
    bool dependent { false };             // a template parameter, or a name that depends on one
};

class Nested;

class BodyParser final : public SyntaxContext {
public:
    BodyParser(const Syntax& syntax, Tree& tree, NameOracle& oracle, std::span<const std::shared_ptr<const SyntaxExtension>> extensions);

    // ---- the driver (bodies.cpp) ----
    void set_outline(const std::unordered_map<std::uint32_t, std::int32_t>* outline) { outline_ = outline; }
    // Names the part's declaration makes visible: a function's parameters, a template's parameters.
    void declare_entry(std::string_view name, NameClass what, bool dependent = false);
    // `open_ended`: the outline's end of the part is a guess (an initializer it cut at a comma inside template
    // arguments): the parse reads where the grammar ends; position() says where that was.
    void begin_part(std::int32_t declaration, std::size_t from, std::size_t to, bool open_ended = false);
    std::size_t position_after() const { return i_; }
    // Roots: what a part holds.
    StmtId read_function_body();
    StmtId read_static_assert();
    ExprId read_initializer();            // `= e`, `{...}`, `(...)`
    ExprId read_expression_part(Part::Role role);
    ExprId read_noexcept_part();
    ExprId read_contract_part();
    // After a part: whether it read all its tokens; if not, the rest is one diagnostic.
    void finish_part();
    bool failed_part() const { return part_failed_; }

    // ---- SyntaxContext ----
    const PpToken& token(std::size_t ahead = 0) const override { return tok(ahead); }
    std::uint32_t position() const override { return static_cast<std::uint32_t>(i_); }
    bool adjacent(std::size_t ahead) const override;
    void advance(std::size_t count = 1) override { for (std::size_t n { 0 }; n < count; ++n) next(); }
    bool accept(Kind kind) override;
    bool accept_word(std::string_view word) override;
    bool expect(Kind kind, std::string_view what) override;
    void fail(std::string message) override;
    bool failed() const override { return failed_; }
    ExprId assignment_expression() override;
    ExprId constant_expression() override { return conditional_expression(); }
    Handle type_or_expression() override;
    Handle type_or_expression_biased(bool unknown_is_expression);
    TypeId type_id() override;
    StmtId statement() override;
    StmtId compound_statement() override;
    StmtId committed_compound();
    StmtId for_statement(bool expansion, std::uint32_t first) override;
    NameId qualified_name() override { return qualified_name(NameUse::expression); }
    ExprId make(Expr node) override { return add(node); }
    StmtId make(Stmt node) override { return add(node); }
    TypeId make(TypeNode node) override { return add(node); }
    LocalId make(Local node) override { return add(node); }
    std::uint32_t first_token(Handle h) const override { return tree_.tokens(h).first; }
    Span32 list(std::span<const Handle> handles) override;
    std::uint32_t contract(Contract c) override;
    void push_scope() override { scope_marks_.push_back(scope_.size()); }
    void pop_scope() override;
    void declare(std::uint32_t name_token, NameClass what) override;

    enum class NameUse : std::uint8_t { expression, type, member, declarator };

private:
    friend class Nested;

    const Syntax& s_;
    Tree& tree_;
    NameOracle& oracle_;
    std::span<const std::shared_ptr<const SyntaxExtension>> extensions_;
    const std::vector<PpToken>& t_;
    const bool tracing_ { base::trace::enabled(TRACE, base::trace::Level::debug) };
    const std::unordered_map<std::uint32_t, std::int32_t>* outline_ { nullptr };

    std::size_t i_ { 0 }, end_ { 0 }, begin_ { 0 }, guess_ { 0 };   // guess_: where the outline said an open-ended part ends
    Kind split_ { Kind::unknown };        // a `>` taken off a `>>`, `>=`, `>>=`: what is left of the token at split_at_
    std::size_t split_at_ { NPOS };
    bool failed_ { false };
    std::size_t fail_at_ { 0 };
    std::string fail_message_;
    bool part_failed_ { false };
    int speculating_ { 0 };
    int no_gt_ { 0 };                     // inside template arguments: a `>` closes them
    bool fold_ok_ { false };              // directly in parentheses: `op ...` ends an expression
    bool no_brace_ { false };             // in a requires-clause: a `{` after a name begins the body, not a construction
    int depth_ { 0 };
    std::int32_t owner_ { -1 };
    std::vector<Entry> scope_;
    std::vector<std::size_t> scope_marks_;
    std::vector<std::string_view> class_names_;   // the classes being read (for constructors' names)
    std::unordered_set<std::size_t> not_arguments_;   // `<` tokens decided not to open template arguments
    bool tentative_declaration_ { false };   // a statement is being tried as a declaration
    StmtId last_declaration_;
    NameClass last_type_class_ { NameClass::unknown };
    ast::Basis last_type_basis_ { ast::Basis::syntax };

    // ---- cursor (bodies.cpp) ----
    const PpToken& tok(std::size_t ahead = 0) const {
        static const PpToken none {};
        return i_ + ahead < end_ ? t_[i_ + ahead] : none;
    }
    bool eof(std::size_t ahead = 0) const { return i_ + ahead >= end_; }
    Kind kind(std::size_t ahead = 0) const;
    bool is(Kind k, std::size_t ahead = 0) const { return !eof(ahead) && kind(ahead) == k; }
    bool word(std::string_view w, std::size_t ahead = 0) const { return !eof(ahead) && t_[i_ + ahead].kind == Kind::raw_identifier && t_[i_ + ahead].spelling == w; }
    bool ident(std::size_t ahead = 0) const;     // an identifier that is not a reserved word
    bool any_word(std::span<const std::string_view> words, std::size_t ahead = 0) const;
    void next();                                  // past the token (or the first half of a split one)
    std::uint32_t prev() const { return static_cast<std::uint32_t>(split_at_ == i_ ? i_ : (i_ > begin_ ? i_ - 1 : i_)); }
    std::uint32_t here() const { return static_cast<std::uint32_t>(i_ < end_ ? i_ : (end_ > 0 ? end_ - 1 : 0)); }
    bool expect_gt();                             // a `>`, taking it off `>>`, `>=`, `>>=` when need be
    bool at_gt() const;
    std::size_t balanced(std::size_t k) const;    // past the bracket pair opened at k (within the part)
    void skip_balanced();                         // the cursor past its bracket pair
    std::string_view spelled(std::size_t k) const { return k < t_.size() ? t_[k].spelling : std::string_view {}; }

    // ---- nodes (bodies.cpp) ----
    ExprId add(Expr e);
    StmtId add(Stmt s);
    TypeId add(TypeNode n);
    LocalId add(Local l);
    NameId add(Name n);
    ExprId error_expr(std::uint32_t first);
    StmtId error_stmt(std::uint32_t first, std::uint32_t last);
    TypeId error_type(std::uint32_t first);
    Span32 emit(const std::vector<Handle>& handles);
    ExprId make_expr(ExprKind kind, std::uint32_t first, ExprId a = {}, ExprId b = {}, ExprId c = {});
    std::string name_text(NameId n) const;        // the components' spellings, `::` between, template arguments left out

    // ---- tentative parsing (bodies.cpp) ----
    struct Mark {
        std::size_t i, split_at;
        Kind split;
        std::size_t exprs, stmts, types, locals, names, components, links, news, lambdas, requires_infos, requirements, folds, designators, bases,
            contracts, attributes, diagnostics, decisions, scopes, marks;
        int no_gt;
        bool fold_ok;
        bool part_failed;
    };
    Mark mark() const;
    void rewind(const Mark& m);
    // Runs `f` where failure is not an error: false, and everything undone, when it fails.
    template <class F>
    bool attempt(F&& f) {
        const Mark m { mark() };
        ++speculating_;
        ++tree_.stats.speculations;
        const bool was_failed { std::exchange(failed_, false) };
        f();
        const bool ok { !failed_ };
        --speculating_;
        if (!ok) {
            if (tracing_) {
                const auto& at = t_[std::min(fail_at_, t_.size() - 1)].at;
                base::trace::debug(TRACE, "{}:{} a tentative parse (from {}:{}) failed: {}", at.line, at.column, t_[m.i < t_.size() ? m.i : t_.size() - 1].at.line,
                                   t_[m.i < t_.size() ? m.i : t_.size() - 1].at.column, fail_message_);
            }
            rewind(m);
            ++tree_.stats.rewinds;
        }
        failed_ = was_failed;
        return ok;
    }
    // Recovery: the failure becomes a diagnostic, the cursor goes to a sync point.
    void recover_to_statement_end();
    void report_failure();
    void diagnostic(std::string message, std::size_t at);

    // ---- decisions and names (bodies.cpp, parse_name.cpp) ----
    void note(Ambiguity what, Resolution chose, ast::Basis basis, std::size_t at);
    NameInfo classify(std::string_view written, std::size_t at, bool record = true);
    NameInfo classify_name(NameId n, std::size_t at, bool record = true);
    const Entry* scope_entry(std::string_view name) const;
    void declare_name(std::size_t token, NameClass what, bool dependent = false);
    NameId qualified_name(NameUse use);
    NameId qualified_name(NameUse use, bool* dependent, NameInfo* info);
    bool name_component(NameComponent& c, NameUse use, bool first, bool after_template_kw, std::string& written, bool* dependent, NameInfo* info,
                        bool* more);
    bool template_arguments(Span32& out);
    Handle template_argument();
    bool starts_name() const;
    bool at_splice() const;                       // `[:` with no space
    std::size_t splice_end(std::size_t k) const;  // past the `:]` closing the splice at k, or k
    bool operator_function_id(NameComponent& c);
    std::size_t angle_end(std::size_t at_less) const;   // past the `>` of `<` at at_less by bracket counting, or at_less when none

    // ---- expressions (parse_expr.cpp) ----
    ExprId expression();
    ExprId initializer_clause();
    ExprId conditional_expression();
    ExprId binary_expression(int min_precedence);
    ExprId cast_expression();
    ExprId unary_expression();
    ExprId postfix_expression();
    ExprId postfix_suffixes(ExprId lhs, std::uint32_t first);
    ExprId primary_expression();
    ExprId name_expression();
    ExprId functional_cast(TypeId type, std::uint32_t first);
    ExprId literal_expression();
    ExprId parenthesized_expression();
    ExprId braced_init_list();
    ExprId designator_or_element();
    Span32 argument_list(Kind close, bool* trailing_pack = nullptr);
    ExprId new_expression();
    ExprId delete_expression();
    ExprId sizeof_expression();
    ExprId named_cast();
    ExprId typeid_expression();
    ExprId builtin_expression();
    ExprId constraint_expression();
    ExprId constraint_primary_for_not();
    ExprId requires_expression();
    ExprId fold_or_paren(std::uint32_t first);
    ExprId lambda_expression();
    ExprId throw_expression();
    bool try_cast(std::uint32_t first, ExprId& out);
    static int precedence(Kind k, Op* op);
    static bool is_builtin_name(std::string_view w);
    bool at_binary_operator(Op* op, int* prec) const;
    ExprId member_access(ExprId lhs, std::uint32_t first);

    // ---- types (parse_type.cpp) ----
    DeclSpec decl_specifiers(Site site);
    bool type_name_in(DeclSpec& spec, Site site, TypeId& named, std::uint32_t& type_first);
    TypeId decltype_type();
    TypeId simple_type();
    TypeId type_name_specifier(std::uint32_t first, bool allow_template_kw, Site site);
    void rewind_one_for_extension(std::uint32_t key);
    bool ident_or_type_keyword() const;
    bool at_type_start(bool unknown_names_are_types);
    bool decltype_followed_by_scope() const;
    Declarator declarator(TypeId base, Mode mode, Site site);
    TypeId pointer_operators(TypeId base);
    bool suffixes(TypeId& type, Mode mode, Site site, Declarator* d);
    Span32 parameter_declaration_clause(bool* c_variadic);
    LocalId parameter_declaration();
    TypeId function_type(TypeId return_type, Span32 params, bool variadic, std::uint32_t first);
    void function_qualifiers(TypeId fn, std::vector<std::uint32_t>* contracts);
    bool starts_nested_declarator() const;
    bool starts_parameter_clause(Mode mode, Site site);
    bool every_element_reads_as_parameter(std::size_t open);
    std::uint32_t cv_flags();
    void skip_attributes();
    void skip_attributes(std::uint32_t& first, std::uint32_t& last);
    TypeId apply_cv(TypeId t, std::uint32_t flags);
    static bool reserved_operator_word(std::string_view w);
    static bool is_unary_word(std::string_view w);

    // ---- declarations (parse_decl.cpp) ----
    StmtId declaration_statement();
    std::vector<LocalId> simple_declaration(Site site, bool* function_definition);
    LocalId init_declarator(const DeclSpec& spec, Site site, bool first, bool* definition);
    LocalId class_specifier(DeclSpec& spec, Site site);
    LocalId enum_specifier(DeclSpec& spec, Site site);
    LocalId using_declaration(Site site);
    LocalId alias_declaration(std::uint32_t first, bool template_);
    LocalId static_assert_declaration();
    LocalId namespace_alias();
    LocalId structured_binding(const DeclSpec& spec, Site site);
    LocalId member_declaration(std::vector<LocalId>& out, Site site);
    std::vector<LocalId> member_specification_until_brace(Site site);
    LocalId template_declaration(Site site);
    LocalId template_declaration_inner(Site site);
    LocalId asm_declaration();
    void initializer_of(Local& l, Site site);
    StmtId function_body(LocalId owner);
    Span32 template_parameter_list(bool* ok);
    LocalId template_parameter();
    bool is_known_type_name(const NameInfo& info) const;
    void read_base_clause(Local& l);
    LocalId condition_declaration(Site site);
    bool is_declaration_start(Site site);
    void bind_declared(const Local& l);
    LocalId link_outline(LocalId id);

    // ---- statements (parse_stmt.cpp) ----
    StmtId statement_inner();
    StmtId declaration_or_expression_statement();
    bool try_declaration_statement(std::uint32_t first, Site site);
    StmtId if_statement(std::uint32_t first);
    StmtId switch_statement(std::uint32_t first);
    StmtId while_statement(std::uint32_t first);
    StmtId do_statement(std::uint32_t first);
    StmtId return_statement(std::uint32_t first);
    StmtId try_statement(std::uint32_t first);
    StmtId expression_statement();
    StmtId asm_statement(std::uint32_t first);
    StmtId for_loop(std::uint32_t first, bool expansion);
    bool init_before_condition(StmtId& init);
    bool condition(ExprId& expr, LocalId& local);
    bool try_condition_declaration(LocalId& local);
    StmtId handler(std::uint32_t first);
    StmtId attributed(std::uint32_t first, StmtId inner, std::uint32_t attr_first, std::uint32_t attr_last);
};

// A nested region (parentheses, brackets, braces, a lambda's body): `>` is an operator again and a
// fold's `...` is not the enclosing parentheses'. Restores what it replaced.
class Nested {
public:
    explicit Nested(BodyParser& p, bool fold = false, int no_gt = 0) : p_ { p }, fold_ { p.fold_ok_ }, no_gt_ { p.no_gt_ }, no_brace_ { p.no_brace_ } {
        p.fold_ok_ = fold;
        p.no_gt_ = no_gt;
        p.no_brace_ = false;
    }
    ~Nested() { if (!restored_) restore(); }
    Nested(const Nested&) = delete;
    Nested& operator=(const Nested&) = delete;
    void restore() {
        p_.fold_ok_ = fold_;
        p_.no_gt_ = no_gt_;
        p_.no_brace_ = no_brace_;
        restored_ = true;
    }

private:
    BodyParser& p_;
    bool fold_;
    int no_gt_;
    bool no_brace_;
    bool restored_ { false };
};

} // namespace mcxx::frontend::bodies
