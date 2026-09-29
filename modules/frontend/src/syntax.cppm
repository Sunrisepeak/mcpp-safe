// mcxx.frontend:syntax -- the declarations of a file, from its preprocessed tokens: namespaces,
// classes, enums and their enumerators, functions (bodies skipped by their braces), variables,
// fields, aliases, concepts, templates, with each one's name and whole range, and the tree they make.
// No types are known: where C++ needs them to tell a declaration's parts apart, the parser decides as
// the code's shape says (a name followed by a declarator is a type; a parenthesized list after a
// declarator is a parameter list unless it holds what only an expression can) -- checked against
// Clang's outline (tools/checks/syntaxdiff.py).
//
//   const auto syntax = mcxx::frontend::parse(text, { .file = path });
//   for (const auto& s : mcxx::frontend::symbols(syntax)) ... s.kind, s.name, s.range, s.selection
//
// It always gives a tree: what it cannot parse it skips (to the next `;` at its depth, or past a
// balanced block), says where (diagnostics), and goes on.
export module mcxx.frontend:syntax;

import std;
import mcxx.msa;
import :lex;
import :preprocess;

export namespace mcxx::frontend {

struct Declaration {
    msa::Kind kind { msa::Kind::unknown };
    std::string name;             // as an outline names it: "f", "operator==", "~S", "operator bool"
    Where name_at;                // the name's first token
    Where at;                     // the whole declaration, from its first token to its last
    std::int32_t parent { -1 };   // index into Syntax::declarations; -1 at file scope
    bool exported { false };      // inside `export`
    bool definition { false };    // a body, a class's members, an enum's enumerators, a variable's initializer
    bool listed { true };         // in an outline: not in an unnamed namespace, class or enum, not a friend
    std::string qualifier;        // an out-of-line member's "S::", as written
    std::uint32_t name_token { 0 };    // indices into Syntax::pp.tokens
    std::uint32_t first_token { 0 };
    std::uint32_t last_token { 0 };
};

struct Syntax {
    Preprocessed pp;
    std::vector<std::uint32_t> line_starts;
    std::vector<Declaration> declarations;   // in the order they are written
    std::vector<Diagnostic> diagnostics;     // where the parser could not follow the code, and skipped
};

// The file's declarations. The text must outlive the result (tokens view it).
Syntax parse(std::string_view text, const PreprocessOptions& options = {});

// The outline, as MSA's Unit::symbols() has it: the listed declarations, each namespace's, class's and
// enum's own inside it.
std::vector<msa::Symbol> symbols(const Syntax& syntax);

// A position (MC3: 0-based line, UTF-8 bytes) of a byte of the text.
msa::Position position(const Syntax& syntax, std::uint32_t offset);

} // namespace mcxx::frontend

namespace mcxx::frontend {

namespace {

constexpr std::string_view SPECIFIERS[] {
    "const", "volatile", "static", "extern", "inline", "constexpr", "consteval", "constinit", "virtual", "mutable", "thread_local",
    "register", "typedef", "friend", "__inline", "__inline__", "_Thread_local", "__restrict", "__restrict__", "restrict", "__thread",
    "_Noreturn", "__extension__",
};
constexpr std::string_view TYPE_KEYWORDS[] {
    "void", "bool", "char", "char8_t", "char16_t", "char32_t", "wchar_t", "short", "int", "long", "signed", "unsigned", "float",
    "double", "auto", "__int128", "__int128_t", "__uint128_t", "_Float16", "__fp16", "__bf16", "_Complex", "_Bool", "__signed", "__signed__",
    "__unsigned",
};
constexpr std::string_view OPERATORS_WORDS[] { "new", "delete", "co_await" };

class Parser {
public:
    Parser(Syntax& out) : out_ { out }, t_ { out.pp.tokens } {}

    void run() {
        Scope file { Context::file, {}, -1, true, false };
        sequence(file, false);
        while (i_ < t_.size()) {   // a stray `}` at file scope
            diagnose("unmatched `}`");
            ++i_;
            sequence(file, false);
        }
    }

private:
    enum class Context : std::uint8_t { file, name_space, class_, enum_ };
    struct Scope {
        Context context;
        std::string class_name;   // the class a member belongs to
        std::int32_t parent;
        bool listed;
        bool exported;
        std::string template_arguments {};   // a class template's: "<T, N>", for its constructors' names
    };
    struct Name {
        std::size_t begin { 0 };   // its first token (a leading `::` included)
        std::size_t end { 0 };     // past its last token
        std::size_t last { 0 };    // the token that names it: an identifier, `~`, `operator`
        std::string spelled;       // "f", "~S", "operator==", "operator bool"
        std::vector<std::string> qualifiers;
        bool destructor { false };
        bool conversion { false };
        bool op { false };
        bool ok { false };
    };

    Syntax& out_;
    const std::vector<PpToken>& t_;
    std::size_t i_ { 0 };
    std::set<std::string, std::less<>> namespaces_;   // names of the file's namespaces: `ns::f` is not a member
    // The parameters of the template header just read, as its arguments ("<T, N>"), and where it began.
    std::string template_arguments_;
    std::size_t template_begin_ { static_cast<std::size_t>(-1) };

    // ---- tokens ----

    const PpToken& tok(std::size_t k) const {
        static const PpToken end {};
        return k < t_.size() ? t_[k] : end;
    }
    bool at_end(std::size_t k) const { return k >= t_.size(); }
    bool is(std::size_t k, Kind kind) const { return k < t_.size() && t_[k].kind == kind; }
    bool word(std::size_t k, std::string_view w) const { return k < t_.size() && t_[k].kind == Kind::raw_identifier && t_[k].spelling == w; }
    bool identifier(std::size_t k) const { return k < t_.size() && t_[k].kind == Kind::raw_identifier; }
    bool any_word(std::size_t k, std::span<const std::string_view> words) const {
        return identifier(k) && std::ranges::contains(words, t_[k].spelling);
    }

    void diagnose(std::string message) {
        const auto& t = tok(std::min(i_, t_.empty() ? 0 : t_.size() - 1));
        out_.diagnostics.push_back({ Diagnostic::Severity::error, std::move(message), t.at });
    }

    // At an opening ( [ {: past its match (or the end). In broken text: a `}` whose `{` is not open
    // here, or a `;` in parentheses no brace holds, ends the scan before it.
    std::size_t balanced(std::size_t k) const {
        std::vector<Kind> stack;
        std::size_t braces { 0 };
        for (; k < t_.size(); ++k) {
            const Kind kind { t_[k].kind };
            if (kind == Kind::l_paren || kind == Kind::l_square || kind == Kind::l_brace) {
                stack.push_back(kind);
                braces += kind == Kind::l_brace ? 1 : 0;
            } else if (kind == Kind::r_paren || kind == Kind::r_square) {
                const Kind open { kind == Kind::r_paren ? Kind::l_paren : Kind::l_square };
                if (stack.back() == open) stack.pop_back();   // a stray one is skipped
                if (stack.empty()) return k + 1;
            } else if (kind == Kind::r_brace) {
                if (braces == 0) return k;
                while (stack.back() != Kind::l_brace) stack.pop_back();
                stack.pop_back();
                --braces;
                if (stack.empty()) return k + 1;
            } else if (kind == Kind::semi && braces == 0) {
                return k;
            }
        }
        return k;
    }

    // At `<` after a name: past the matching `>` of template arguments, or `k` when there is none
    // before a `;`, `{` or `}`. `>>` closes two lists; `extra` says it closed the caller's as well.
    std::size_t angle(std::size_t k, int* extra = nullptr) const {
        std::size_t j { k + 1 };
        while (j < t_.size()) {
            const Kind kind { t_[j].kind };
            if (kind == Kind::greater) return j + 1;
            if (kind == Kind::greatergreater) {
                if (extra != nullptr) *extra = 1;
                return j + 1;
            }
            if (kind == Kind::less) {
                int inner { 0 };
                const std::size_t after { angle(j, &inner) };
                if (after == j) return k;
                if (inner != 0) return after;   // the `>>` closed this list too
                j = after;
                continue;
            }
            if (kind == Kind::l_paren || kind == Kind::l_square) {
                j = balanced(j);
                continue;
            }
            if (kind == Kind::l_brace) {   // T{} in an argument
                j = balanced(j);
                continue;
            }
            if (kind == Kind::semi || kind == Kind::r_brace || kind == Kind::r_paren || kind == Kind::r_square) return k;
            ++j;
        }
        return k;
    }

    // Attributes and their kin: [[...]], alignas(...), __attribute__((...)), __declspec(...).
    std::size_t attributes(std::size_t k) const {
        for (;;) {
            if (is(k, Kind::l_square) && is(k + 1, Kind::l_square)) k = balanced(k);
            else if ((word(k, "alignas") || word(k, "__attribute__") || word(k, "__declspec") || word(k, "_Alignas")) && is(k + 1, Kind::l_paren))
                k = balanced(k + 1);
            else return k;
        }
    }

    // Past the next `;` at this depth (balanced blocks skipped), or before a `}` that closes the scope.
    void skip_statement() {
        while (i_ < t_.size()) {
            const Kind kind { t_[i_].kind };
            if (kind == Kind::semi) {
                ++i_;
                return;
            }
            if (kind == Kind::r_brace) return;
            if (kind == Kind::l_paren || kind == Kind::l_square || kind == Kind::l_brace) {
                i_ = balanced(i_);
                continue;
            }
            ++i_;
        }
    }

    Where span(std::size_t first, std::size_t last) const {
        const auto& a = tok(first).at;
        const auto& b = tok(last).at;
        return { a.begin, std::max(a.end, b.end), a.line, a.column };
    }

    // A declaration: its name's token, its first and last tokens.
    std::int32_t record(msa::Kind kind, std::string name, std::size_t name_token, std::size_t first, std::size_t last, const Scope& scope,
                        bool definition, bool listed, std::string qualifier) {
        Declaration d { kind, std::move(name), tok(name_token).at, span(first, last), scope.parent, scope.exported, definition, listed, std::move(qualifier) };
        d.name_token = static_cast<std::uint32_t>(name_token);
        d.first_token = static_cast<std::uint32_t>(first);
        d.last_token = static_cast<std::uint32_t>(last);
        out_.declarations.push_back(std::move(d));
        return static_cast<std::int32_t>(out_.declarations.size() - 1);
    }
    // Its last token, once its body is read.
    void close(std::int32_t index, std::size_t last) {
        auto& d = out_.declarations[static_cast<std::size_t>(index)];
        d.at = span(d.first_token, last);
        d.last_token = static_cast<std::uint32_t>(last);
    }

    // ---- names ----

    // A (qualified) name at k: [::] (id [<...>] ::)* (id [<...>] | ~id | operator ...).
    Name name(std::size_t k) const {
        Name n;
        n.begin = k;
        if (is(k, Kind::coloncolon)) ++k;
        for (;;) {
            if (word(k, "template")) ++k;
            if (is(k, Kind::tilde) && identifier(k + 1)) {
                n.destructor = true;
                n.last = k;
                n.spelled = "~" + std::string { tok(k + 1).spelling };
                k += 2;
                if (is(k, Kind::less)) k = std::max(k, angle(k));
                n.end = k;
                n.ok = true;
                return n;
            }
            if (word(k, "operator")) return operator_name(n, k);
            if (word(k, "decltype") && is(k + 1, Kind::l_paren)) {
                k = balanced(k + 1);
                if (is(k, Kind::coloncolon)) {
                    ++k;
                    continue;
                }
                n.end = k;
                n.last = n.begin;
                n.ok = false;
                return n;
            }
            if (!identifier(k)) return n;
            const std::size_t id { k };
            ++k;
            if (is(k, Kind::less)) {
                const std::size_t after { angle(k) };
                if (after != k) k = after;
            }
            if (is(k, Kind::coloncolon) && (identifier(k + 1) || is(k + 1, Kind::tilde))) {
                n.qualifiers.emplace_back(tok(id).spelling);
                ++k;
                continue;
            }
            n.last = id;
            n.spelled = std::string { tok(id).spelling };
            n.end = k;
            n.ok = true;
            return n;
        }
    }

    Name operator_name(Name n, std::size_t k) const {
        n.last = k;
        n.op = true;
        std::size_t j { k + 1 };
        std::string op;
        if (is(j, Kind::l_paren) && is(j + 1, Kind::r_paren)) {
            op = "()";
            j += 2;
        } else if (is(j, Kind::l_square) && is(j + 1, Kind::r_square)) {
            op = "[]";
            j += 2;
        } else if (any_word(j, OPERATORS_WORDS)) {
            op = " " + std::string { tok(j).spelling };
            ++j;
            if (is(j, Kind::l_square) && is(j + 1, Kind::r_square)) {
                op += "[]";
                j += 2;
            }
        } else if (is_string(tok(j).kind)) {
            // operator "" _x, operator ""_x
            op = std::string { tok(j).spelling };
            ++j;
            if (identifier(j) && op == "\"\"") op += std::string { tok(j++).spelling };
        } else if (!at_end(j) && tok(j).kind != Kind::raw_identifier && tok(j).kind != Kind::l_paren && tok(j).kind != Kind::coloncolon) {
            op = std::string { tok(j).spelling };
            ++j;
        } else {
            // A conversion function: the type, up to its parameter list.
            n.conversion = true;
            std::string type;
            while (!at_end(j) && !is(j, Kind::l_paren) && !is(j, Kind::semi) && !is(j, Kind::l_brace)) {
                if (is(j, Kind::less)) {
                    const std::size_t after { angle(j) };
                    if (after == j) break;
                    for (std::size_t x { j }; x < after; ++x) type += tok(x).spelling;
                    j = after;
                    continue;
                }
                if (!type.empty() && (identifier(j) || is(j, Kind::star) || is(j, Kind::amp) || is(j, Kind::ampamp))) type += ' ';
                type += tok(j).spelling;
                ++j;
            }
            op = " " + type;
        }
        n.spelled = "operator" + op;
        if (is(j, Kind::less)) {   // operator<< <T>, a specialization
            const std::size_t after { angle(j) };
            if (after != j && !n.conversion) j = after;
        }
        n.end = j;
        n.ok = true;
        return n;
    }

    static bool is_string(Kind k) {
        return k == Kind::string_literal || k == Kind::wide_string_literal || k == Kind::utf8_string_literal || k == Kind::utf16_string_literal ||
               k == Kind::utf32_string_literal;
    }

    // ---- declarations ----

    void sequence(const Scope& scope, bool braced) {
        while (i_ < t_.size()) {
            if (is(i_, Kind::r_brace)) {
                if (braced) return;
                return;
            }
            const std::size_t before { i_ };
            declaration(scope);
            if (i_ == before) {
                diagnose(std::format("unexpected `{}`", tok(i_).spelling));
                ++i_;
            }
        }
    }

    // A `{ ... }` block of declarations: its members parsed in `inner`; past the `}`.
    void block(const Scope& inner) {
        ++i_;   // {
        sequence(inner, true);
        if (is(i_, Kind::r_brace)) ++i_;
        else diagnose("missing `}`");
    }

    void declaration(const Scope& scope, std::optional<std::size_t> first = std::nullopt) {
        const std::size_t start { attributes(i_) };
        i_ = start;
        const std::size_t begin { first.value_or(start) };
        if (at_end(i_) || is(i_, Kind::r_brace)) return;
        if (is(i_, Kind::semi)) {
            ++i_;
            return;
        }
        const std::string_view w { identifier(i_) ? tok(i_).spelling : std::string_view {} };
        if (w == "export") {
            if (word(i_ + 1, "module") || word(i_ + 1, "import")) return skip_statement();
            ++i_;
            Scope inner { scope };
            inner.exported = true;
            if (is(i_, Kind::l_brace)) return block(inner);
            return declaration(inner);
        }
        if ((w == "module" || w == "import") && tok(i_).start_of_line) return skip_statement();
        if (w == "namespace" || (w == "inline" && word(i_ + 1, "namespace"))) return name_space(scope, begin);
        if (w == "using") return using_declaration(scope, begin);
        if (w == "template") return template_declaration(scope, begin);
        if (w == "extern" && is_string(tok(i_ + 1).kind)) {
            i_ += 2;
            if (is(i_, Kind::l_brace)) return block(scope);
            return declaration(scope);
        }
        if (w == "extern" && word(i_ + 1, "template")) return skip_statement();
        if (w == "static_assert" || w == "_Static_assert" || w == "asm" || w == "__asm__") return skip_statement();
        if ((w == "public" || w == "private" || w == "protected") && is(i_ + 1, Kind::colon)) {
            i_ += 2;
            return;
        }
        simple_declaration(scope, begin);
    }

    void name_space(const Scope& scope, std::size_t begin) {
        if (word(i_, "inline")) ++i_;
        ++i_;   // namespace
        i_ = attributes(i_);
        // namespace name = qualified-name;
        if (identifier(i_) && is(i_ + 1, Kind::equal)) {
            const std::size_t name_at { i_ };
            i_ += 2;
            const Name target { name(i_) };
            i_ = target.ok ? target.end : i_;
            record(msa::Kind::namespace_alias, std::string { tok(name_at).spelling }, name_at, begin, i_ - 1, scope, true, scope.listed, {});
            if (is(i_, Kind::semi)) ++i_;
            else skip_statement();
            return;
        }
        // namespace a::b::inline c { ... }: one namespace in the other; each starts where its name's
        // part starts (its `::`), all end at the `}`.
        std::vector<std::pair<std::size_t, std::size_t>> parts;   // (first token, name token)
        std::size_t part_begin { begin };
        while (identifier(i_)) {
            parts.emplace_back(part_begin, i_);
            ++i_;
            if (!is(i_, Kind::coloncolon)) break;
            part_begin = i_;
            ++i_;
            if (word(i_, "inline")) ++i_;
        }
        i_ = attributes(i_);
        if (!is(i_, Kind::l_brace)) {
            diagnose("expected `{` after the namespace's name");
            return skip_statement();
        }
        std::vector<std::int32_t> made;
        Scope inner { scope };
        inner.context = Context::name_space;
        if (parts.empty()) {
            // An unnamed namespace: in the tree, not in an outline (nor what it holds).
            inner.listed = false;
            made.push_back(record(msa::Kind::namespace_, {}, begin, begin, begin, scope, true, false, {}));
            inner.parent = made.back();
        }
        for (const auto& [first, id] : parts) {
            namespaces_.emplace(tok(id).spelling);
            made.push_back(record(msa::Kind::namespace_, std::string { tok(id).spelling }, id, first, first, inner, true, inner.listed, {}));
            inner.parent = made.back();
        }
        block(inner);
        for (const auto index : made) close(index, i_ - 1);
    }

    void using_declaration(const Scope& scope, std::size_t begin) {
        ++i_;   // using
        // using X [[attrs]] = type;
        if (identifier(i_) && (is(attributes(i_ + 1), Kind::equal))) {
            const std::size_t id { i_ };
            i_ = attributes(i_ + 1) + 1;
            skip_statement_end();
            // An alias template's name, as Clang places it, is its `using`.
            const std::size_t selection { template_begin_ == begin ? id - 1 : id };
            record(msa::Kind::type_alias, std::string { tok(id).spelling }, selection, begin, i_ - 1, scope, true, scope.listed, {});
            if (is(i_, Kind::semi)) ++i_;
            return;
        }
        skip_statement();   // using namespace, using enum, a using-declaration
    }

    // Up to (not past) the `;` that ends this declaration, or a `}` or `,` at this depth.
    void skip_statement_end(bool stop_at_comma = false) {
        while (i_ < t_.size()) {
            const Kind kind { t_[i_].kind };
            if (kind == Kind::semi || kind == Kind::r_brace || (stop_at_comma && kind == Kind::comma)) return;
            if (kind == Kind::l_paren || kind == Kind::l_square || kind == Kind::l_brace) {
                i_ = balanced(i_);
                continue;
            }
            ++i_;
        }
    }

    void template_declaration(const Scope& scope, std::size_t begin) {
        while (word(i_, "template")) {
            ++i_;
            if (!is(i_, Kind::less)) return skip_statement();   // an explicit instantiation
            const std::size_t after { angle(i_) };
            if (after == i_) {
                diagnose("unbalanced template parameter list");
                return skip_statement();
            }
            template_arguments_ = parameter_names(i_ + 1, after - 1);
            template_begin_ = begin;
            i_ = after;
        }
        if (word(i_, "requires")) constraint();
        i_ = attributes(i_);
        if (word(i_, "concept") && identifier(i_ + 1)) {
            const std::size_t id { i_ + 1 };
            i_ += 2;
            skip_statement_end();
            record(msa::Kind::concept_, std::string { tok(id).spelling }, id, begin, i_ - 1, scope, true, scope.listed, {});
            if (is(i_, Kind::semi)) ++i_;
            return;
        }
        declaration(scope, begin);
    }

    // A template parameter list's names as an argument list: "<T, N, Ts...>"; "" for `<>`.
    std::string parameter_names(std::size_t from, std::size_t to) const {
        std::vector<std::string> names;
        std::string current;
        bool pack { false };
        bool in_default { false };
        for (std::size_t k { from }; k <= to && k < t_.size(); ++k) {
            if (is(k, Kind::comma) || k == to) {
                if (!current.empty()) names.push_back(current + (pack ? "..." : ""));
                current.clear();
                pack = in_default = false;
                continue;
            }
            if (in_default) continue;
            if (is(k, Kind::equal)) {
                in_default = true;
                continue;
            }
            if (is(k, Kind::less)) {   // template <class> class TT: its own parameters
                const std::size_t after { angle(k) };
                if (after != k) k = after - 1;
                continue;
            }
            if (is(k, Kind::l_paren) || is(k, Kind::l_square)) {
                k = balanced(k) - 1;
                continue;
            }
            if (is(k, Kind::ellipsis)) pack = true;
            if (identifier(k) && !word(k, "typename") && !word(k, "class") && !any_word(k, TYPE_KEYWORDS) && !word(k, "const"))
                current = std::string { tok(k).spelling };
        }
        if (names.empty()) return {};
        std::string out { "<" };
        for (std::size_t n { 0 }; n < names.size(); ++n) out += (n ? ", " : "") + names[n];
        return out + ">";
    }

    // requires constraint-expression: primaries joined by && and ||.
    void constraint() {
        ++i_;   // requires
        for (;;) {
            if (is(i_, Kind::exclaim)) ++i_;
            if (is(i_, Kind::l_paren)) i_ = balanced(i_);
            else if (word(i_, "requires")) {
                ++i_;
                if (is(i_, Kind::l_paren)) i_ = balanced(i_);
                if (is(i_, Kind::l_brace)) i_ = balanced(i_);
            } else if (word(i_, "true") || word(i_, "false") || is(i_, Kind::numeric_constant)) {
                ++i_;
            } else {
                const Name n { name(i_) };
                if (!n.ok) return;
                i_ = n.end;
                if (is(i_, Kind::l_paren)) i_ = balanced(i_);   // a function-style call
            }
            if (is(i_, Kind::ampamp) || is(i_, Kind::pipepipe)) {
                ++i_;
                continue;
            }
            return;
        }
    }

    // class-key [attrs] [name [<args>]] [final] [: bases] { members }, or an elaborated name.
    // `i_` at the key. The declaration, when one is made; `definition` whether it has members.
    std::int32_t class_specifier(const Scope& scope, std::size_t begin, bool friend_, bool* definition) {
        const std::string_view key { tok(i_).spelling };
        const bool is_enum { key == "enum" };
        ++i_;
        if (is_enum && (word(i_, "class") || word(i_, "struct"))) ++i_;
        i_ = attributes(i_);
        Name n;
        if (identifier(i_) || is(i_, Kind::coloncolon)) {
            n = name(i_);
            if (n.ok) i_ = n.end;
        }
        i_ = attributes(i_);
        while (word(i_, "final") || word(i_, "sealed") || word(i_, "abstract")) ++i_;
        const msa::Kind kind { is_enum ? msa::Kind::enum_ : key == "union" ? msa::Kind::union_ : key == "struct" ? msa::Kind::struct_ : msa::Kind::class_ };
        const bool body_follows { is(i_, Kind::l_brace) || (is(i_, Kind::colon) && !(is_enum && false)) };
        if (!body_follows) {
            *definition = false;
            // A forward declaration (`class X;`) declares X; an elaborated name elsewhere does not.
            if (n.ok && is(i_, Kind::semi) && !friend_)
                return record(kind, n.spelled, n.last, begin, i_ - 1, scope, false, scope.listed, join(n.qualifiers));
            return -1;
        }
        if (is(i_, Kind::colon)) {   // bases, or an enum's underlying type
            ++i_;
            while (i_ < t_.size() && !is(i_, Kind::l_brace) && !is(i_, Kind::semi) && !is(i_, Kind::r_brace)) {
                if (is(i_, Kind::l_paren) || is(i_, Kind::l_square)) i_ = balanced(i_);
                else if (is(i_, Kind::less)) {
                    const std::size_t after { angle(i_) };
                    i_ = after == i_ ? i_ + 1 : after;
                } else ++i_;
            }
            if (!is(i_, Kind::l_brace)) {   // an opaque enum declaration: enum class E : int;
                *definition = false;
                if (n.ok && !friend_)
                    return record(kind, n.spelled, n.last, begin, i_ - 1, scope, false, scope.listed, {});
                return -1;
            }
        }
        *definition = true;
        const bool named { n.ok };
        const std::int32_t index { record(kind, named ? n.spelled : std::string {}, named ? n.last : begin, begin, begin, scope, true,
                                          scope.listed && named && !friend_, join(n.qualifiers)) };
        Scope inner { is_enum ? Context::enum_ : Context::class_, named ? n.spelled : std::string {}, index, scope.listed && named && !friend_, scope.exported };
        // A class template (not a specialization, whose arguments are written): its constructors are
        // named with its parameters.
        if (template_begin_ == begin && !template_arguments_.empty() && !is(n.last + 1, Kind::less)) inner.template_arguments = template_arguments_;
        if (is_enum) enumerators(inner);
        else block(inner);
        close(index, i_ - 1);
        return index;
    }

    static std::string join(const std::vector<std::string>& q) {
        std::string s;
        for (const auto& c : q) s += c + "::";
        return s;
    }

    void enumerators(const Scope& e) {
        ++i_;   // {
        while (i_ < t_.size() && !is(i_, Kind::r_brace)) {
            i_ = attributes(i_);
            if (!identifier(i_)) {
                diagnose("expected an enumerator");
                skip_statement_end(true);
                if (is(i_, Kind::comma)) ++i_;
                if (is(i_, Kind::semi)) ++i_;
                continue;
            }
            const std::size_t id { i_ };
            ++i_;
            i_ = attributes(i_);
            if (is(i_, Kind::equal)) {
                ++i_;
                skip_statement_end(true);
            }
            record(msa::Kind::enumerator, std::string { tok(id).spelling }, id, id, i_ - 1, e, true, e.listed, {});
            if (is(i_, Kind::comma)) ++i_;
            else if (!is(i_, Kind::r_brace)) {
                diagnose("expected `,` or `}` after an enumerator");
                skip_statement_end(true);
                if (is(i_, Kind::semi)) ++i_;
            }
        }
        if (is(i_, Kind::r_brace)) ++i_;
    }

    struct Declarator {
        Name id;
        bool ok { false };
        bool function { false };   // its (innermost) declarator-id takes a parameter list
        bool nested { false };
        bool pointer { false };
    };

    // What a parenthesized list after a declarator-id holds: parameters (a function) or an
    // expression (a variable's initializer)? The tokens decide; a type is not known.
    bool parameters(std::size_t open, const Scope& scope, const Name& id, bool void_type) const {
        const std::size_t close { balanced(open) - 1 };
        if (close == open + 1) return true;   // ()
        if (void_type || scope.context == Context::class_ || !id.qualifiers.empty() || id.op || id.destructor || id.conversion) return true;
        std::size_t k { attributes(open + 1) };
        if (any_word(k, SPECIFIERS) || any_word(k, TYPE_KEYWORDS) || word(k, "typename") || word(k, "decltype") || word(k, "struct") || word(k, "class") ||
            word(k, "enum") || word(k, "union") || is(k, Kind::ellipsis))
            return true;
        // The first parameter, up to its default argument: an expression's tokens say it is an
        // initializer (literals, operators a declaration does not hold).
        std::size_t end { k };
        while (end < close && !is(end, Kind::comma) && !is(end, Kind::equal)) {
            if (is(end, Kind::l_paren) || is(end, Kind::l_square) || is(end, Kind::l_brace)) {
                end = balanced(end);
                continue;
            }
            ++end;
        }
        for (std::size_t j { k }; j < end; ++j) {
            const Kind kind { t_[j].kind };
            if (kind == Kind::numeric_constant || is_string(kind) || kind == Kind::char_constant || kind == Kind::l_brace || kind == Kind::plus ||
                kind == Kind::minus || kind == Kind::exclaim || kind == Kind::question || kind == Kind::arrow || kind == Kind::period ||
                kind == Kind::slash || kind == Kind::percent || kind == Kind::equalequal || kind == Kind::pipepipe)
                return false;
            if (word(j, "nullptr") || word(j, "true") || word(j, "false") || word(j, "sizeof") || word(j, "new") || word(j, "this")) return false;
        }
        // name name, name *, name &, name<...> name: a parameter.
        const Name first { name(k) };
        if (!first.ok) return false;
        const std::size_t after { first.end };
        return identifier(after) || is(after, Kind::star) || is(after, Kind::amp) || is(after, Kind::ampamp) || is(after, Kind::comma) ||
               is(after, Kind::r_paren) || is(after, Kind::ellipsis) || is(after, Kind::l_square) || is(after, Kind::equal) || word(after, "const") ||
               word(after, "volatile") || is(after, Kind::coloncolon);
    }

    Declarator declarator(const Scope& scope, bool void_type = false) {
        Declarator d;
        // ptr-operators: * & && ^, class::*, with their cv and attributes
        for (;;) {
            i_ = attributes(i_);
            if (is(i_, Kind::star) || is(i_, Kind::amp) || is(i_, Kind::ampamp) || is(i_, Kind::caret)) {
                d.pointer = true;
                ++i_;
                while (word(i_, "const") || word(i_, "volatile") || word(i_, "__restrict") || word(i_, "__restrict__") || word(i_, "restrict") ||
                       word(i_, "_Nonnull") || word(i_, "_Nullable"))
                    ++i_;
                continue;
            }
            // class::*
            if (identifier(i_) || is(i_, Kind::coloncolon)) {
                std::size_t k { i_ };
                if (is(k, Kind::coloncolon)) ++k;
                bool member { false };
                while (identifier(k)) {
                    ++k;
                    if (is(k, Kind::less)) {
                        const std::size_t after { angle(k) };
                        if (after == k) break;
                        k = after;
                    }
                    if (!is(k, Kind::coloncolon)) break;
                    ++k;
                    if (is(k, Kind::star)) {
                        member = true;
                        break;
                    }
                }
                if (member) {
                    d.pointer = true;
                    i_ = k + 1;
                    continue;
                }
            }
            break;
        }
        if (is(i_, Kind::l_paren) && !is(i_ + 1, Kind::r_paren)) {
            // ( declarator ): a nested one when what follows the `(` starts a declarator.
            const std::size_t k { attributes(i_ + 1) };
            const bool nested { is(k, Kind::star) || is(k, Kind::amp) || is(k, Kind::ampamp) || is(k, Kind::caret) ||
                                ((identifier(k) || is(k, Kind::coloncolon)) && [&] {
                                    const Name n { name(k) };
                                    return n.ok && is(n.end, Kind::r_paren);
                                }()) };
            if (nested) {
                ++i_;
                Declarator inner { declarator(scope, void_type) };
                if (is(i_, Kind::r_paren)) ++i_;
                inner.nested = true;
                const bool inner_function { inner.function };
                suffixes(scope, inner, true, void_type);
                inner.function = inner_function || (!inner.pointer && inner.function);
                return inner;
            }
        }
        if (is(i_, Kind::ellipsis)) ++i_;
        if (identifier(i_) || is(i_, Kind::coloncolon) || is(i_, Kind::tilde)) {
            d.id = name(i_);
            if (d.id.ok) {
                i_ = d.id.end;
                d.ok = true;
            }
        }
        i_ = attributes(i_);
        suffixes(scope, d, false, void_type);
        return d;
    }

    // ( parameters ) and [ bounds ] after a declarator-id; `outer` for those after a nested declarator.
    void suffixes(const Scope& scope, Declarator& d, bool outer, bool void_type) {
        bool first { true };
        for (;;) {
            if (is(i_, Kind::l_paren)) {
                if (first && !outer && !parameters(i_, scope, d.id, void_type)) return;   // an initializer: the caller reads it
                if (first && !outer) d.function = true;
                i_ = balanced(i_);
                first = false;
                continue;
            }
            if (is(i_, Kind::l_square) && !is(i_ + 1, Kind::l_square)) {
                i_ = balanced(i_);
                first = false;
                continue;
            }
            return;
        }
    }

    // What follows a function's declarator: cv, ref, noexcept, trailing return, virt-specifiers, a
    // requires-clause, = 0 / default / delete.
    void function_tail() {
        for (;;) {
            i_ = attributes(i_);
            if (word(i_, "const") || word(i_, "volatile") || is(i_, Kind::amp) || is(i_, Kind::ampamp) || word(i_, "override") || word(i_, "final") ||
                word(i_, "mutable")) {
                ++i_;
                continue;
            }
            if ((word(i_, "noexcept") || word(i_, "throw")) ) {
                ++i_;
                if (is(i_, Kind::l_paren)) i_ = balanced(i_);
                continue;
            }
            if (is(i_, Kind::arrow)) {
                ++i_;
                // The trailing return type, to what ends the declarator.
                while (i_ < t_.size()) {
                    if (is(i_, Kind::l_brace) || is(i_, Kind::semi) || is(i_, Kind::equal) || is(i_, Kind::comma) || is(i_, Kind::r_brace) ||
                        word(i_, "requires") || word(i_, "override") || word(i_, "final") || (is(i_, Kind::colon) && !is(i_ + 1, Kind::colon)))
                        break;
                    if (is(i_, Kind::l_paren) || is(i_, Kind::l_square)) {
                        i_ = balanced(i_);
                        continue;
                    }
                    if (is(i_, Kind::less)) {
                        const std::size_t after { angle(i_) };
                        i_ = after == i_ ? i_ + 1 : after;
                        continue;
                    }
                    ++i_;
                }
                continue;
            }
            if (word(i_, "requires")) {
                constraint();
                continue;
            }
            return;
        }
    }

    // decl-specifiers declarator [, declarator]... ; or a function definition.
    void simple_declaration(const Scope& scope, std::size_t begin) {
        bool type { false }, typedef_ { false }, static_ { false }, friend_ { false }, void_ { false };
        std::int32_t made_class { -1 };
        bool class_definition { false };
        // decl-specifiers
        for (;;) {
            i_ = attributes(i_);
            if (at_end(i_)) return;
            const std::string_view w { identifier(i_) ? tok(i_).spelling : std::string_view {} };
            if (any_word(i_, SPECIFIERS)) {
                typedef_ = typedef_ || w == "typedef";
                static_ = static_ || w == "static";
                friend_ = friend_ || w == "friend";
                ++i_;
                continue;
            }
            if (w == "explicit" || w == "__forceinline") {
                ++i_;
                if (is(i_, Kind::l_paren)) i_ = balanced(i_);
                continue;
            }
            if (any_word(i_, TYPE_KEYWORDS)) {
                type = true;
                void_ = void_ || w == "void";
                ++i_;
                continue;
            }
            if ((w == "decltype" || w == "__typeof__" || w == "typeof" || w == "_Atomic" || w == "__underlying_type") && is(i_ + 1, Kind::l_paren)) {
                type = true;
                i_ = balanced(i_ + 1);
                if (is(i_, Kind::coloncolon)) {   // decltype(x)::type
                    const Name n { name(i_ + 1) };
                    if (n.ok) i_ = n.end;
                }
                continue;
            }
            if (w == "class" || w == "struct" || w == "union" || w == "enum") {
                if (type) break;
                made_class = class_specifier(scope, begin, friend_, &class_definition);
                type = true;
                continue;
            }
            if (w == "typename") {
                ++i_;
                const Name n { name(i_) };
                i_ = n.ok ? n.end : i_ + 1;
                type = true;
                continue;
            }
            if (!type && (identifier(i_) || is(i_, Kind::coloncolon))) {
                const Name n { name(i_) };
                if (!n.ok) break;
                // No type yet: a name that is followed by a declarator is one; a name followed by `(`
                // is the declarator (a constructor, or a function a macro stands before).
                if (is(n.end, Kind::l_paren) || n.destructor || n.op) break;
                if (is(n.end, Kind::semi) || is(n.end, Kind::equal) || is(n.end, Kind::comma) || is(n.end, Kind::l_brace) || is(n.end, Kind::colon) ||
                    is(n.end, Kind::l_square)) {
                    break;   // `x;`, `x = 1;`: no type written (an error, or a macro's)
                }
                i_ = n.end;
                type = true;
                continue;
            }
            break;
        }
        if (made_class >= 0 && is(i_, Kind::semi)) {   // class S { ... };
            ++i_;
            return;
        }
        if (is(i_, Kind::semi)) {   // `struct S;` elaborated in a friend, or specifiers alone
            ++i_;
            return;
        }
        (void)class_definition;
        // declarators
        for (bool first_declarator { true };; first_declarator = false) {
            const std::size_t declarator_start { i_ };
            Declarator d { declarator(scope, void_) };
            if (!d.ok) {
                if (i_ == declarator_start) {
                    if (made_class >= 0 && (is(i_, Kind::r_brace) || at_end(i_))) return;
                    diagnose(std::format("expected a declaration, found `{}`", tok(i_).spelling));
                    recover(begin);
                    return;
                }
            }
            const bool function { d.function };
            bool body { false };
            std::optional<std::size_t> end_before;   // the declaration's last token, when not the last one read
            if (function) {
                function_tail();
                if (is(i_, Kind::equal)) {
                    // = 0, = default, = delete ["why"]: in a declaration's range, not in an out-of-line
                    // definition's (`S::S() = default;` ends at its `)`), as Clang has them.
                    if (!d.id.qualifiers.empty()) end_before = i_ - 1;
                    ++i_;
                    skip_statement_end(true);
                } else if (is(i_, Kind::colon) && !typedef_) {   // a constructor's initializers, then the body
                    ++i_;
                    initializers();
                    body = true;
                } else if (word(i_, "try")) {
                    ++i_;
                    if (is(i_, Kind::colon)) {
                        ++i_;
                        initializers();
                    } else if (is(i_, Kind::l_brace)) i_ = balanced(i_);
                    while (word(i_, "catch") && is(i_ + 1, Kind::l_paren)) {
                        i_ = balanced(i_ + 1);
                        if (is(i_, Kind::l_brace)) i_ = balanced(i_);
                    }
                    body = true;
                } else if (is(i_, Kind::l_brace)) {
                    i_ = balanced(i_);
                    body = true;
                }
            } else {
                // A bit-field's width, then an initializer: = e, { e }, ( e ).
                if (is(i_, Kind::colon) && scope.context == Context::class_) {
                    ++i_;
                    while (i_ < t_.size() && !is(i_, Kind::semi) && !is(i_, Kind::comma) && !is(i_, Kind::equal) && !is(i_, Kind::l_brace) &&
                           !is(i_, Kind::r_brace)) {
                        if (is(i_, Kind::l_paren)) i_ = balanced(i_);
                        else ++i_;
                    }
                }
                if (is(i_, Kind::equal)) {
                    ++i_;
                    initializer();
                } else if (is(i_, Kind::l_brace) || is(i_, Kind::l_paren)) {
                    i_ = balanced(i_);
                }
            }
            if (d.ok && !friend_) {
                msa::Kind kind { msa::Kind::variable };
                const bool member { scope.context == Context::class_ || (!d.id.qualifiers.empty() && !namespaces_.contains(d.id.qualifiers.back())) };
                if (typedef_) kind = msa::Kind::type_alias;
                else if (function) {
                    const std::string_view owner { !d.id.qualifiers.empty() ? std::string_view { d.id.qualifiers.back() } : std::string_view { scope.class_name } };
                    if (d.id.destructor) kind = msa::Kind::destructor;
                    else if (d.id.conversion) kind = msa::Kind::conversion;
                    else if (!owner.empty() && d.id.spelled == owner && (scope.context == Context::class_ || !d.id.qualifiers.empty())) kind = msa::Kind::constructor;
                    else kind = member ? msa::Kind::method : msa::Kind::function;
                } else if (scope.context == Context::class_ && !static_) kind = msa::Kind::field;
                const std::size_t last { end_before.value_or(i_ > 0 ? i_ - 1 : 0) };
                std::string spelled { d.id.spelled };
                // A class template's constructor and destructor are named with its parameters (`S<T>`).
                if ((kind == msa::Kind::constructor || kind == msa::Kind::destructor) && d.id.qualifiers.empty()) spelled += scope.template_arguments;
                record(kind, std::move(spelled), d.id.last, begin, last, scope, body || !function, scope.listed, join(d.id.qualifiers));
            }
            if (body) {
                // A function definition ends the declaration (a `;` after it is an empty one).
                return;
            }
            if (is(i_, Kind::comma)) {
                ++i_;
                continue;
            }
            if (is(i_, Kind::semi)) {
                ++i_;
                return;
            }
            (void)first_declarator;
            diagnose(std::format("expected `;` after a declaration, found `{}`", at_end(i_) ? std::string_view { "the end" } : tok(i_).spelling));
            recover(begin);
            return;
        }
    }

    // A variable's initializer after `=`: to the `,` or `;` that ends it (template arguments are not
    // told from comparisons here: a `,` inside `a<b, c>` at this depth ends it early).
    void initializer() { skip_statement_end(true); }

    // A constructor's member initializers, then its body.
    void initializers() {
        while (i_ < t_.size()) {
            if (is(i_, Kind::l_brace)) {
                // The body, unless this brace initializes the name before it.
                const bool initializes { i_ > 0 && (identifier(i_ - 1) || is(i_ - 1, Kind::greater) || is(i_ - 1, Kind::greatergreater)) };
                i_ = balanced(i_);
                if (!initializes) return;
                continue;
            }
            if (is(i_, Kind::l_paren)) {
                i_ = balanced(i_);
                continue;
            }
            if (is(i_, Kind::less)) {
                const std::size_t after { angle(i_) };
                i_ = after == i_ ? i_ + 1 : after;
                continue;
            }
            if (is(i_, Kind::semi) || is(i_, Kind::r_brace)) return;
            ++i_;
        }
    }

    // After what could not be parsed: past the next `;`, or a balanced block, at this depth -- but
    // not past a line that starts another declaration after an unterminated one (a macro invocation
    // without its `;`, say).
    void recover(std::size_t begin) {
        while (i_ < t_.size()) {
            if (i_ > begin && t_[i_].start_of_line && !t_[i_].expanded && starts_declaration(i_)) return;
            const Kind kind { t_[i_].kind };
            if (kind == Kind::semi) {
                ++i_;
                return;
            }
            if (kind == Kind::r_brace) return;
            if (kind == Kind::l_brace) {
                i_ = balanced(i_);
                return;
            }
            if (kind == Kind::l_paren || kind == Kind::l_square) {
                i_ = balanced(i_);
                continue;
            }
            ++i_;
        }
    }

    bool starts_declaration(std::size_t k) const {
        return any_word(k, SPECIFIERS) || any_word(k, TYPE_KEYWORDS) || word(k, "namespace") || word(k, "class") || word(k, "struct") ||
               word(k, "union") || word(k, "enum") || word(k, "template") || word(k, "using") || word(k, "export") || word(k, "typedef");
    }
};

} // namespace

Syntax parse(std::string_view text, const PreprocessOptions& options) {
    Syntax out;
    out.pp = preprocess(text, options);
    out.line_starts.push_back(0);
    for (std::uint32_t i { 0 }; i < text.size(); ++i)
        if (text[i] == '\n' || (text[i] == '\r' && (i + 1 == text.size() || text[i + 1] != '\n'))) out.line_starts.push_back(i + 1);
    Parser parser { out };
    parser.run();
    return out;
}

msa::Position position(const Syntax& syntax, std::uint32_t offset) {
    const auto it = std::ranges::upper_bound(syntax.line_starts, offset);
    const auto line = static_cast<std::uint32_t>(it - syntax.line_starts.begin() - 1);
    return { line, offset - *(it - 1) };
}

std::vector<msa::Symbol> symbols(const Syntax& syntax) {
    // Children in the order written: a declaration's index is after its parent's.
    std::vector<std::vector<std::size_t>> children(syntax.declarations.size() + 1);
    for (std::size_t i { 0 }; i < syntax.declarations.size(); ++i) {
        const auto& d = syntax.declarations[i];
        children[static_cast<std::size_t>(d.parent + 1)].push_back(i);
    }
    // Positions as Clang's outline gives them: a token in a macro's expansion is where the outermost
    // invocation starts; a name ends its spelled length after its start, a range the length of the
    // file's token at its last token's place (the macro's name, for one an expansion gave) -- on
    // that token's line, however many lines the token spans.
    const auto& tokens = syntax.pp.tokens;
    const auto after = [&](std::uint32_t begin, std::uint32_t length) {
        const auto p = position(syntax, begin);
        return msa::Position { p.line, p.column + length };
    };
    const auto selection = [&](const Declaration& d) {
        if (d.name_token >= tokens.size()) return msa::Range { position(syntax, d.name_at.begin), position(syntax, d.name_at.end) };
        const auto& t = tokens[d.name_token];
        const std::uint32_t length { t.expanded ? static_cast<std::uint32_t>(t.spelling.size()) : t.at.end - t.at.begin };
        return msa::Range { position(syntax, t.at.begin), after(t.at.begin, std::max(1u, length)) };
    };
    const auto whole = [&](const Declaration& d) {
        if (d.last_token >= tokens.size() || d.first_token >= tokens.size()) return msa::Range { position(syntax, d.at.begin), position(syntax, d.at.end) };
        const auto& t = tokens[d.last_token];
        const std::uint32_t length { t.expanded ? t.macro_end - t.at.begin : t.at.end - t.at.begin };
        return msa::Range { position(syntax, tokens[d.first_token].at.begin), after(t.at.begin, length) };
    };
    std::function<void(std::size_t, std::vector<msa::Symbol>&)> walk = [&](std::size_t slot, std::vector<msa::Symbol>& out) {
        for (const auto i : children[slot]) {
            const auto& d = syntax.declarations[i];
            if (!d.listed || d.name.empty() || d.kind == msa::Kind::unknown) continue;
            msa::Symbol s;
            s.name = d.name;
            s.kind = d.kind;
            s.range = whole(d);
            s.selection = selection(d);
            if (d.kind == msa::Kind::namespace_ || d.kind == msa::Kind::class_ || d.kind == msa::Kind::struct_ || d.kind == msa::Kind::union_ ||
                d.kind == msa::Kind::enum_)
                walk(i + 1, s.children);
            out.push_back(std::move(s));
        }
    };
    std::vector<msa::Symbol> out;
    walk(0, out);
    return out;
}

} // namespace mcxx::frontend
