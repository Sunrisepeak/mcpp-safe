// The declaration-level parser behind mcxx.frontend:syntax (parse()): declarations by the code's
// shape, the constructs gates look at, the scopes name lookup needs, recovery that always gives a tree.
module mcxx.frontend;

import std;
import mcxx.msa;
import mcxx.base;

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
// Words that start a statement that is not a declaration.
constexpr std::string_view STATEMENTS[] {
    "return", "delete", "throw", "case", "default", "goto", "break", "continue", "co_return", "co_yield", "co_await", "new", "else", "do", "if",
    "for", "while", "switch", "try", "catch", "static_assert", "sizeof", "this", "using", "typedef", "asm", "__asm__", "__asm", "operator",
    "alignof", "noexcept", "requires", "true", "false", "nullptr", "typeid", "namespace",
};
constexpr std::string_view CASTS[] { "static_cast", "dynamic_cast", "const_cast", "reinterpret_cast" };

constexpr std::string_view TRACE { "frontend.syntax" };

class Parser {
public:
    Parser(Syntax& out, const Known& known) : out_ { out }, t_ { out.pp.tokens } {
        for (const auto& [name, a] : known.aliases) aliases_.insert_or_assign(name, std::pair { a.pointer, a.c_array });
    }

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
    enum class Context : std::uint8_t { file, name_space, class_, enum_, block };
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
    // Asked once per parse: a trace point is then a branch, its arguments not even evaluated when off.
    const bool tracing_ { base::trace::enabled(TRACE, base::trace::Level::debug) };
    std::size_t i_ { 0 };
    std::set<std::string, std::less<>> namespaces_;   // names of the file's namespaces: `ns::f` is not a member
    std::vector<std::pair<std::string, std::string>> pending_allows_;   // waivers read, for the next declaration
    std::map<std::string, std::pair<bool, bool>, std::less<>> aliases_;   // this file's aliases: (pointer, array)
    std::int32_t owner_ { -1 };   // the declaration whose body or initializer is being read
    std::size_t lambdas_ { 0 };   // how many lambdas' parameters, captures or bodies are being read
    std::vector<std::size_t> scope_ends_;   // the blocks being read: each one's `}` (what a local declared now is visible to)
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
        if (tracing_) base::trace::debug(TRACE, "{}:{} cannot follow: {}", t.at.line, t.at.column, message);
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

    // The `)` matching the `(` at `open`, a `;` inside it allowed (`for (;;)`, `if (init; cond)`, which
    // balanced() would end at); an unmatched closer (the enclosing block's `}`) where it is missing.
    std::size_t closing_paren(std::size_t open) const {
        std::size_t depth { 0 };
        for (std::size_t k { open }; k < t_.size(); ++k) {
            const Kind kind { t_[k].kind };
            if (kind == Kind::l_paren || kind == Kind::l_square || kind == Kind::l_brace) ++depth;
            else if ((kind == Kind::r_paren || kind == Kind::r_square || kind == Kind::r_brace) && --depth == 0) return k;
        }
        return t_.size();
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

    // In an expression, at k: past a name's template arguments when it has them -- `<...>` followed by
    // `(`, `{` or `::` (`std::make_shared<std::map<K, V>>()`, `T<A, B>::f`), what comparisons are
    // not -- so that a `,` among them does not end the expression; else k + 1.
    std::size_t past_template_arguments(std::size_t k) const {
        if (!identifier(k) || !is(k + 1, Kind::less)) return k + 1;
        const std::size_t after { angle(k + 1) };
        if (after == k + 1 || !(is(after, Kind::l_paren) || is(after, Kind::l_brace) || is(after, Kind::coloncolon))) return k + 1;
        return after;
    }

    // Attributes as attributes() skips them, their [[mcpp::allow(...)]] kept for the next declaration.
    std::size_t waivers(std::size_t k) {
        const std::size_t end { attributes(k) };
        for (std::size_t j { k }; j + 5 < end; ++j) {
            if (!(word(j, "mcpp") && is(j + 1, Kind::coloncolon) && word(j + 2, "allow") && is(j + 3, Kind::l_paren))) continue;
            std::vector<std::string> strings;
            for (std::size_t a { j + 4 }; a < end && !is(a, Kind::r_paren); ++a)
                if (is_string(tok(a).kind)) {
                    const std::string_view q { tok(a).spelling };
                    const auto open = q.find('"');
                    strings.emplace_back(q.substr(open + 1, q.rfind('"') - open - 1));
                }
            if (!strings.empty()) pending_allows_.emplace_back(strings[0], strings.size() > 1 ? strings[1] : std::string {});
        }
        return end;
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
        d.allows = std::exchange(pending_allows_, {});
        d.in_lambda = lambdas_ > 0;
        if (!scope_ends_.empty()) d.visible_end = static_cast<std::uint32_t>(scope_ends_.back());
        if (tracing_)
            base::trace::debug(TRACE, "{}:{} {} `{}` (declaration {}, in {})", d.name_at.line, d.name_at.column, msa::to_string(d.kind), d.name,
                               out_.declarations.size(), d.parent);
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
        const std::size_t start { waivers(i_) };
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
        if (w == "asm" || w == "__asm__") add_construct(Construct::What::asm_, i_, i_, scope.parent);
        if (w == "static_assert" || w == "_Static_assert" || w == "asm" || w == "__asm__") return skip_statement();
        if ((w == "public" || w == "private" || w == "protected") && is(i_ + 1, Kind::colon)) {
            i_ += 2;
            return;
        }
        simple_declaration(scope, begin);
    }

    void name_space(const Scope& scope, std::size_t begin) {
        const bool inline_first { word(i_, "inline") };
        if (inline_first) ++i_;
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
        std::vector<std::tuple<std::size_t, std::size_t, bool>> parts;   // (first token, name token, inline)
        std::size_t part_begin { begin };
        bool inline_part { inline_first };
        while (identifier(i_)) {
            parts.emplace_back(part_begin, i_, inline_part);
            ++i_;
            if (!is(i_, Kind::coloncolon)) break;
            part_begin = i_;
            ++i_;
            inline_part = word(i_, "inline");
            if (inline_part) ++i_;
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
            made.push_back(record(msa::Kind::namespace_, {}, i_, begin, begin, scope, true, false, {}));   // named at its `{`, as Clang has it
            inner.parent = made.back();
        }
        for (const auto& [first, id, inline_] : parts) {
            namespaces_.emplace(tok(id).spelling);
            made.push_back(record(msa::Kind::namespace_, std::string { tok(id).spelling }, id, first, first, inner, true, inner.listed, {}));
            out_.declarations.back().inline_namespace = inline_;
            inner.parent = made.back();
        }
        block(inner);
        for (const auto index : made) close(index, i_ - 1);
    }

    void using_declaration(const Scope& scope, std::size_t begin) {
        const std::size_t using_token { i_ };
        ++i_;   // using
        // using X [[attrs]] = type;
        if (identifier(i_) && (is(attributes(i_ + 1), Kind::equal))) {
            const std::size_t id { i_ };
            i_ = attributes(i_ + 1) + 1;
            // The type: specifiers and an abstract declarator (`void (*)(int)`, `int[16]`).
            const std::size_t type_start { i_ };
            const Scope type_scope { Context::block, {}, scope.parent, false, false };
            const Specifiers sp { specifiers(type_scope, type_start) };
            const std::size_t specifiers_end { i_ };
            const Declarator d { declarator(type_scope) };
            const std::size_t declarator_end { i_ };
            const std::pair flags { sp.pointer || d.star, sp.c_array || d.array };
            aliases_.insert_or_assign(std::string { tok(id).spelling }, flags);
            skip_statement_end();
            // An alias template's name, as Clang places it, is its `using`.
            const std::size_t selection { template_begin_ == begin ? id - 1 : id };
            const std::int32_t index { record(msa::Kind::type_alias, std::string { tok(id).spelling }, selection, begin, i_ - 1, scope, true, scope.listed, {}) };
            out_.declarations[static_cast<std::size_t>(index)].pointer = flags.first;
            out_.declarations[static_cast<std::size_t>(index)].c_array = flags.second;
            typed(index, type_start, specifiers_end, specifiers_end, declarator_end, d);
            if (is(i_, Kind::semi)) ++i_;
            return;
        }
        using_names(scope.parent, using_token, scope.exported);
        skip_statement();   // using namespace, using enum, a using-declaration
    }

    // `using namespace n;` and `using n::x;` at k (its `using`), recorded for name lookup.
    void using_names(std::int32_t parent, std::size_t k, bool exported = false) {
        std::size_t j { k + 1 };
        const bool directive { word(j, "namespace") };
        if (directive) ++j;
        if (word(j, "enum") || word(j, "typename")) return;
        std::string name;
        while (j < t_.size() && (identifier(j) || is(j, Kind::coloncolon))) name += tok(j++).spelling;
        if (name.empty() || !is(j, Kind::semi)) return;
        out_.usings.push_back({ directive, std::move(name), static_cast<std::uint32_t>(k), parent,
                                static_cast<std::uint32_t>(scope_ends_.empty() ? 0 : scope_ends_.back()), static_cast<std::uint32_t>(j - 1), exported });
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
        std::vector<std::int32_t> parameters;   // visible to the end of what the template declares
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
            for (const auto name : parameter_tokens(i_ + 1, after - 1))
                parameters.push_back(record(msa::Kind::template_parameter, std::string { tok(name).spelling }, name, name, name, scope, true, false, {}));
            i_ = after;
        }
        struct Visible {
            Parser& self;
            const std::vector<std::int32_t>& parameters;
            ~Visible() {
                for (const auto p : parameters) self.out_.declarations[static_cast<std::size_t>(p)].visible_end = static_cast<std::uint32_t>(self.i_ > 0 ? self.i_ - 1 : 0);
            }
        } visible { *this, parameters };
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

    // A template parameter list's names' tokens: each parameter's, where it has one.
    std::vector<std::size_t> parameter_tokens(std::size_t from, std::size_t to) const {
        std::vector<std::size_t> out;
        std::optional<std::size_t> current;
        bool in_default { false };
        for (std::size_t k { from }; k <= to && k < t_.size(); ++k) {
            if (is(k, Kind::comma) || k == to) {
                if (current) out.push_back(*current);
                current.reset();
                in_default = false;
                continue;
            }
            if (in_default) continue;
            if (is(k, Kind::equal)) {
                in_default = true;
                continue;
            }
            if (is(k, Kind::less)) {
                const std::size_t after { angle(k) };
                if (after != k) k = after - 1;
                continue;
            }
            if (is(k, Kind::l_paren) || is(k, Kind::l_square)) {
                k = balanced(k) - 1;
                continue;
            }
            if (identifier(k) && !word(k, "typename") && !word(k, "class") && !any_word(k, TYPE_KEYWORDS) && !word(k, "const")) current = k;
        }
        return out;
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
        const bool scoped { is_enum && (word(i_, "class") || word(i_, "struct")) };
        if (scoped) ++i_;
        i_ = waivers(i_);
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
        std::size_t bases_begin { 0 }, bases_end { 0 };
        if (is(i_, Kind::colon)) {   // bases, or an enum's underlying type
            ++i_;
            bases_begin = i_;
            while (i_ < t_.size() && !is(i_, Kind::l_brace) && !is(i_, Kind::semi) && !is(i_, Kind::r_brace)) {
                if (is(i_, Kind::l_paren) || is(i_, Kind::l_square)) i_ = balanced(i_);
                else if (is(i_, Kind::less)) {
                    const std::size_t after { angle(i_) };
                    i_ = after == i_ ? i_ + 1 : after;
                } else ++i_;
            }
            bases_end = i_;
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
        out_.declarations[static_cast<std::size_t>(index)].scoped_enum = scoped;
        if (!is_enum) {
            out_.declarations[static_cast<std::size_t>(index)].bases_begin = static_cast<std::uint32_t>(bases_begin);
            out_.declarations[static_cast<std::size_t>(index)].bases_end = static_cast<std::uint32_t>(bases_end);
        }
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
        bool pointer { false };    // a ptr-operator: * & && ^ or class::*
        bool star { false };       // a `*` (or `^`) among them: a pointer, not only a reference
        bool member { false };     // a `class::*` among them: a member pointer (no raw pointer, as Clang has it)
        bool array { false };      // its declarator-id is followed by [ ]
        std::size_t params { static_cast<std::size_t>(-1) };   // the `(` of its parameter list
    };

    // What a parenthesized list after a declarator-id holds: parameters (a function) or an
    // expression (a variable's initializer)? The tokens decide; a type is not known.
    bool parameters(std::size_t open, const Scope& scope, const Name& id, bool void_type) const {
        const std::size_t close { balanced(open) - 1 };
        if (scope.context == Context::block) return false;   // a local's direct initializer
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
                d.star = d.star || is(i_, Kind::star) || is(i_, Kind::caret);
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
                    d.member = true;
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
                const bool outer_star { d.star }, outer_member { d.member };
                suffixes(scope, inner, true, void_type);
                inner.function = inner_function || (!inner.pointer && inner.function);
                inner.star = inner.star || outer_star;
                inner.member = inner.member || outer_member;
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
                if (first && !outer) {
                    d.function = true;
                    d.params = i_;
                }
                i_ = balanced(i_);
                first = false;
                continue;
            }
            if (is(i_, Kind::l_square) && !is(i_ + 1, Kind::l_square)) {
                if (first && !outer) d.array = true;
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
    struct Specifiers {
        bool type { false }, typedef_ { false }, static_ { false }, friend_ { false }, void_ { false };
        bool auto_ { false };     // the type is `auto` (or `decltype(auto)`): deduced
        bool extern_ { false };   // `extern`: a declaration, which a reference need not initialize
        bool pointer { false };   // a `*` in a template argument of the type (outside parentheses), or the type an alias of one
        bool c_array { false };   // the type an alias (of this file) of an array
        bool va_list { false };   // the type is va_list: an array or a pointer, by the target
        std::int32_t made_class { -1 };
        bool class_definition { false };
    };

    // Whether tokens [from, to) hold a `*` outside parentheses: a pointer in template arguments.
    bool star_in(std::size_t from, std::size_t to) const {
        int parens { 0 };
        for (std::size_t k { from }; k < to && k < t_.size(); ++k) {
            if (is(k, Kind::l_paren)) ++parens;
            else if (is(k, Kind::r_paren)) --parens;
            else if (parens == 0 && is(k, Kind::star)) return true;
        }
        return false;
    }

    // decl-specifiers, from i_: the flags they set, i_ past them.
    Specifiers specifiers(const Scope& scope, std::size_t begin) {
        Specifiers sp;
        auto& type = sp.type;
        auto& typedef_ = sp.typedef_;
        auto& static_ = sp.static_;
        auto& friend_ = sp.friend_;
        auto& void_ = sp.void_;
        auto& made_class = sp.made_class;
        auto& class_definition = sp.class_definition;
        for (;;) {
            i_ = waivers(i_);
            if (at_end(i_)) return sp;
            const std::string_view w { identifier(i_) ? tok(i_).spelling : std::string_view {} };
            if (any_word(i_, SPECIFIERS)) {
                typedef_ = typedef_ || w == "typedef";
                static_ = static_ || w == "static";
                sp.extern_ = sp.extern_ || w == "extern";
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
                sp.auto_ = sp.auto_ || w == "auto";
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
                if (n.ok) sp.pointer = sp.pointer || star_in(n.begin, n.end);
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
                sp.pointer = sp.pointer || star_in(n.begin, n.end);
                // An alias this file declared: what its type holds, this one does.
                if (const auto a = aliases_.find(n.spelled); a != aliases_.end() && n.qualifiers.empty()) {
                    sp.pointer = sp.pointer || a->second.first;
                    sp.c_array = sp.c_array || a->second.second;
                }
                sp.va_list = sp.va_list || n.spelled == "__builtin_va_list" || n.spelled == "va_list" || n.spelled == "__gnuc_va_list";
                i_ = n.end;
                type = true;
                continue;
            }
            break;
        }
        return sp;
    }

    void simple_declaration(const Scope& scope, std::size_t begin) {
        const std::size_t specifiers_begin { i_ };
        const Specifiers sp { specifiers(scope, begin) };
        const std::size_t specifiers_end { i_ };
        const bool typedef_ { sp.typedef_ }, static_ { sp.static_ }, friend_ { sp.friend_ }, void_ { sp.void_ };
        const std::int32_t made_class { sp.made_class };
        if (at_end(i_)) return;
        if (made_class >= 0 && is(i_, Kind::semi)) {   // class S { ... };
            ++i_;
            return;
        }
        if (is(i_, Kind::semi)) {   // `struct S;` elaborated in a friend, or specifiers alone
            ++i_;
            return;
        }
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
            // The declaration first (what its body holds is its own), its last token once read.
            std::int32_t index { -1 };
            // A friend function is its class's enclosing namespace's, as Clang has it (not in the
            // outline); a friend variable or type is nothing here.
            const bool friend_function { friend_ && d.ok && d.function && scope.context == Context::class_ && scope.parent >= 0 };
            const bool recorded { d.ok && (!friend_ || friend_function) };
            if (recorded) {
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
                std::string spelled { d.id.spelled };
                // A class template's constructor and destructor are named with its parameters (`S<T>`).
                if ((kind == msa::Kind::constructor || kind == msa::Kind::destructor) && d.id.qualifiers.empty()) spelled += scope.template_arguments;
                if (friend_function) kind = msa::Kind::function;
                const Scope outer { friend_function ? Scope { Context::name_space, {}, out_.declarations[static_cast<std::size_t>(scope.parent)].parent, false, scope.exported }
                                                    : scope };
                index = record(kind, std::move(spelled), d.id.last, begin, i_ > 0 ? i_ - 1 : 0, outer, !function, outer.listed && !friend_function,
                               join(d.id.qualifiers));
                auto& made = out_.declarations[static_cast<std::size_t>(index)];
                made.pointer = sp.pointer || d.star;
                made.c_array = (d.array || sp.c_array) && !function;
                made.va_list = sp.va_list && !d.pointer && !function;
                if (typedef_) aliases_.insert_or_assign(d.id.spelled, std::pair { made.pointer, made.c_array });
                typed(index, specifiers_begin, specifiers_end, declarator_start, i_, d);
            }
            // What a body or an initializer holds belongs to this declaration, or (a friend's) to the scope.
            const std::int32_t owner { recorded ? index : scope.parent };
            if (function && d.params != static_cast<std::size_t>(-1)) parameters_of(d.params, owner, recorded ? index : -1);
            if (function) {
                const std::size_t tail { i_ };
                function_tail();
                if (recorded && star_in(tail, i_)) out_.declarations[static_cast<std::size_t>(index)].pointer = true;   // -> T*
                if (is(i_, Kind::equal)) {
                    // = 0, = default, = delete ["why"]: in a declaration's range, not in an out-of-line
                    // definition's (`S::S() = default;` ends at its `)`), as Clang has them.
                    if (!d.id.qualifiers.empty()) end_before = i_ - 1;
                    ++i_;
                    skip_statement_end(true);
                } else if (is(i_, Kind::colon) && !typedef_) {   // a constructor's initializers, then the body
                    ++i_;
                    initializers(owner);
                    body = true;
                } else if (word(i_, "try")) {
                    add_construct(Construct::What::try_, i_, i_, owner);
                    ++i_;
                    if (is(i_, Kind::colon)) {
                        ++i_;
                        initializers(owner);
                    } else if (is(i_, Kind::l_brace)) i_ = compound(i_, owner);
                    while (word(i_, "catch") && is(i_ + 1, Kind::l_paren)) {
                        const std::size_t close { balanced(i_ + 1) };
                        try_local(i_ + 2, owner, close - 1, Parens::catch_);
                        i_ = close;
                        if (is(i_, Kind::l_brace)) i_ = compound(i_, owner);
                    }
                    body = true;
                } else if (is(i_, Kind::l_brace)) {
                    i_ = compound(i_, owner);
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
                    const std::size_t from { i_ };
                    initializer();
                    scan(from, i_, owner, false);
                } else if (is(i_, Kind::l_brace) || is(i_, Kind::l_paren)) {
                    const std::size_t from { i_ };
                    i_ = balanced(i_);
                    scan(from, i_, owner, false);
                }
            }
            if (recorded) {
                auto& made = out_.declarations[static_cast<std::size_t>(index)];
                close(index, end_before.value_or(i_ > 0 ? i_ - 1 : 0));
                made.definition = body || !function;
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

    // A variable's initializer after `=`: to the `,` or `;` that ends it (a name's template arguments
    // skipped whole: past_template_arguments).
    void initializer() {
        while (i_ < t_.size()) {
            const Kind kind { t_[i_].kind };
            if (kind == Kind::semi || kind == Kind::r_brace || kind == Kind::comma) return;
            if (kind == Kind::l_paren || kind == Kind::l_square || kind == Kind::l_brace) {
                i_ = balanced(i_);
                continue;
            }
            i_ = past_template_arguments(i_);
        }
    }

    // A constructor's member initializers, then its body.
    void initializers(std::int32_t owner) {
        const std::size_t from { i_ };
        while (i_ < t_.size()) {
            if (is(i_, Kind::l_brace)) {
                // The body, unless this brace initializes the name before it.
                const bool initializes { i_ > 0 && (identifier(i_ - 1) || is(i_ - 1, Kind::greater) || is(i_ - 1, Kind::greatergreater)) };
                if (!initializes) {
                    scan(from, i_, owner, false);
                    i_ = compound(i_, owner);
                    return;
                }
                i_ = balanced(i_);
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
            if (is(i_, Kind::semi) || is(i_, Kind::r_brace)) break;
            ++i_;
        }
        scan(from, i_, owner, false);
    }

    void add_construct(Construct::What what, std::size_t first, std::size_t last, std::int32_t owner, std::string detail = {}, std::string to = {},
                       bool array = false) {
        out_.constructs.push_back({ what, static_cast<std::uint32_t>(first), static_cast<std::uint32_t>(last), owner, std::move(detail), std::move(to), array });
    }

    // Where a declaration's type is written: its decl-specifiers and its declarator.
    void typed(std::int32_t index, std::size_t specifiers_begin, std::size_t specifiers_end, std::size_t declarator_begin, std::size_t declarator_end,
               const Declarator& d) {
        if (index < 0) return;
        auto& made = out_.declarations[static_cast<std::size_t>(index)];
        made.specifiers_begin = static_cast<std::uint32_t>(specifiers_begin);
        made.specifiers_end = static_cast<std::uint32_t>(specifiers_end);
        made.declarator_begin = static_cast<std::uint32_t>(declarator_begin);
        made.declarator_end = static_cast<std::uint32_t>(declarator_end);
        if (d.ok) {
            made.id_begin = static_cast<std::uint32_t>(d.id.begin);
            made.id_end = static_cast<std::uint32_t>(d.id.end);
        }
    }

    std::string text_of(std::size_t from, std::size_t to) const {
        std::string out;
        for (std::size_t k { from }; k < to && k < t_.size(); ++k) {
            if (!out.empty() && t_[k].leading_space) out += ' ';
            out += t_[k].spelling;
        }
        return out;
    }

    // A function's parameters, (open) to its `)`: each a declaration of it; `...` makes it C variadic.
    void parameters_of(std::size_t open, std::int32_t owner, std::int32_t function) {
        const std::size_t saved { i_ };
        const std::size_t close { balanced(open) - 1 };
        std::size_t k { open + 1 };
        const Scope scope { Context::block, {}, owner, false, false };
        while (k < close) {
            // The parameter's tokens, to a top-level `,` (template arguments' commas are theirs).
            std::size_t end { k };
            while (end < close && !is(end, Kind::comma)) {
                if (is(end, Kind::l_paren) || is(end, Kind::l_square) || is(end, Kind::l_brace)) end = balanced(end);
                else if (is(end, Kind::less) && end > k && (identifier(end - 1) || is(end - 1, Kind::greater))) {
                    const std::size_t after { angle(end) };
                    end = after == end ? end + 1 : after;
                } else ++end;
            }
            end = std::min(end, close);
            if (end == k + 1 && is(k, Kind::ellipsis)) {
                if (function >= 0) out_.declarations[static_cast<std::size_t>(function)].c_variadic = true;
            } else if (!(end == k + 1 && word(k, "void")) && end > k) {
                i_ = k;
                const Specifiers sp { specifiers(scope, k) };
                const std::size_t specifiers_end { std::min(i_, end) };
                // An unnamed one is where its name would be, as Clang places it: the token after its
                // declarator (a `,`, the `)`, a default argument's `=`).
                std::size_t name_token { std::min(i_, end) };
                Declarator d;
                if (i_ < end) {
                    d = declarator(scope);
                    name_token = d.ok ? d.id.last : std::min(i_, end);
                }
                // Its default argument is not in its range.
                std::size_t last { std::min(i_, end) > k ? std::min(i_, end) - 1 : k };
                if (is(i_, Kind::equal) && i_ < end) scan(i_ + 1, end, owner, false);
                const std::int32_t index { record(msa::Kind::parameter, d.ok ? d.id.spelled : std::string {}, name_token, k, last, scope, true, false, {}) };
                typed(index, k, specifiers_end, specifiers_end, std::min(i_, end), d);
                auto& made = out_.declarations[static_cast<std::size_t>(index)];
                made.pointer = sp.pointer || d.star;
                made.c_array = d.array || sp.c_array;
                made.va_list = sp.va_list && !d.pointer;
            }
            k = end + 1;
        }
        i_ = saved;
    }

    // A compound statement at `open` (a function's body, a lambda's): its local declarations and the
    // constructs in it. Past its `}`.
    std::size_t compound(std::size_t open, std::int32_t owner) {
        const std::size_t close { balanced(open) };
        if (tracing_) base::trace::debug(TRACE, "{}:{} the body of declaration {}", tok(open).at.line, tok(open).at.column, owner);
        scan(open, close, owner, true);
        return close;
    }

    // The constructs in tokens [from, to), and, where statements are (`statements`, or inside a
    // lambda's body), the local declarations.
    void scan(std::size_t from, std::size_t to, std::int32_t owner, bool statements) {
        const std::size_t saved { i_ };
        std::vector<bool> contexts { statements };   // per brace: statements inside, or an initializer list
        std::vector<bool> lambda_bodies { false };   // per brace: a lambda's body
        bool boundary { statements };                // at a statement's start: a body's `{` is one, whatever precedes it
        std::size_t control_close { static_cast<std::size_t>(-1) };   // the `)` of if/for/while/switch: a statement follows
        bool lambda_body_next { false };
        for (std::size_t k { from }; k < to && k < t_.size();) {
            const PpToken& t = t_[k];
            // Local declarations, at a statement's start.
            if (boundary && contexts.back() && t.kind == Kind::raw_identifier && !any_word(k, STATEMENTS)) {
                if (is(k + 1, Kind::colon) && !is(k + 1, Kind::coloncolon)) {   // a label
                    k += 2;
                    continue;
                }
                if (try_local(k, owner, to)) {
                    k = i_;
                    boundary = is(k - 1, Kind::semi) || is(k - 1, Kind::r_brace);
                    if (is(k, Kind::semi)) {
                        ++k;
                        boundary = true;
                    }
                    continue;
                }
            }
            if (boundary && contexts.back() && word(k, "using") && identifier(k + 1) && is(attributes(k + 2), Kind::equal)) {
                // using X = type; -- a local alias, as a namespace-scope one is read
                i_ = k;
                using_declaration(Scope { Context::block, {}, owner, false, false }, k);
                k = i_;
                boundary = true;
                continue;
            }
            if (boundary && contexts.back() && word(k, "using")) {   // using namespace n; using n::x;
                using_names(owner, k);
            }
            if (boundary && contexts.back() && word(k, "namespace") && identifier(k + 1) && is(k + 2, Kind::equal)) {   // namespace a = b::c;
                std::size_t end { k + 3 };
                while (end < to && !is(end, Kind::semi)) ++end;
                record(msa::Kind::namespace_alias, std::string { tok(k + 1).spelling }, k + 1, k, end, Scope { Context::block, {}, owner, false, false },
                       true, false, {});
                k = end;
                continue;
            }
            if (boundary && (word(k, "case") || word(k, "default"))) {   // case e: / default:
                std::size_t j { k + 1 };
                while (j < to && !(is(j, Kind::colon))) j = is(j, Kind::l_paren) ? balanced(j) : j + 1;
                k = j + 1;
                continue;
            }
            // Constructs.
            if (t.kind == Kind::raw_identifier) {
                const std::string_view w { t.spelling };
                const bool after_operator { k > 0 && word(k - 1, "operator") };
                if (w == "goto") {
                    const bool computed { is(k + 1, Kind::star) };
                    add_construct(Construct::What::goto_, k, computed ? k + 2 : k + 1, owner, computed ? "*" : std::string { tok(k + 1).spelling });
                } else if (w == "new" && !after_operator) {
                    std::size_t j { k + 1 };
                    if (is(j, Kind::l_paren)) j = balanced(j);   // placement
                    const std::size_t type_from { j };
                    if (is(j, Kind::l_paren)) j = balanced(j);   // new (T)
                    else {
                        while (j < to && (identifier(j) || is(j, Kind::coloncolon))) {
                            ++j;
                            if (is(j, Kind::less)) {
                                const std::size_t after { angle(j) };
                                if (after == j) break;
                                j = after;
                            }
                        }
                        while (is(j, Kind::star) || is(j, Kind::amp)) ++j;
                    }
                    add_construct(Construct::What::new_, k, j > k + 1 ? j - 1 : k, owner, text_of(type_from, j), {}, is(j, Kind::l_square));
                } else if (w == "delete" && !after_operator && !(k > 0 && is(k - 1, Kind::equal))) {
                    const bool array { is(k + 1, Kind::l_square) && is(k + 2, Kind::r_square) };
                    add_construct(Construct::What::delete_, k, array ? k + 2 : k, owner, {}, {}, array);
                } else if (std::ranges::contains(CASTS, w) && is(k + 1, Kind::less)) {
                    const std::size_t after { angle(k + 1) };
                    const std::size_t close { is(after, Kind::l_paren) ? balanced(after) - 1 : after - 1 };
                    add_construct(Construct::What::cast, k, close, owner, std::string { w }, after > k + 1 ? text_of(k + 2, after - 1) : std::string {});
                } else if (w == "throw" && !(k > 0 && is(k - 1, Kind::r_paren) && false)) {
                    // `throw` in a dynamic exception specification (`throw()`) is not an expression.
                    if (!(is(k + 1, Kind::l_paren) && k > 0 && (is(k - 1, Kind::r_paren) || word(k - 1, "const") || word(k - 1, "noexcept"))))
                        add_construct(Construct::What::throw_, k, k, owner);
                } else if (w == "try" && is(k + 1, Kind::l_brace)) {
                    add_construct(Construct::What::try_, k, k, owner);
                } else if (w == "typeid" && is(k + 1, Kind::l_paren)) {
                    const std::size_t close { balanced(k + 1) };
                    add_construct(Construct::What::typeid_, k, close - 1, owner, text_of(k + 2, close - 1));
                } else if ((w == "asm" || w == "__asm__" || w == "__asm") && (is(k + 1, Kind::l_paren) || word(k + 1, "volatile") || word(k + 1, "__volatile__"))) {
                    add_construct(Construct::What::asm_, k, k, owner);
                } else if ((w == "va_arg" || w == "__builtin_va_arg") && is(k + 1, Kind::l_paren)) {
                    add_construct(Construct::What::va_arg, k, balanced(k + 1) - 1, owner);
                }
                // if (...), for (...), while (...), switch (...), catch (...): a declaration may start
                // right inside; a statement follows the `)`.
                if ((w == "if" || w == "for" || w == "while" || w == "switch" || w == "catch") && (is(k + 1, Kind::l_paren) || (w == "if" && word(k + 1, "constexpr")))) {
                    std::size_t open { k + 1 };
                    if (word(open, "constexpr")) ++open;
                    if (is(open, Kind::l_paren)) {
                        control_close = closing_paren(open);
                        if (tracing_)
                            base::trace::debug(TRACE, "{}:{} {} (...) to {}:{}", t.at.line, t.at.column, w, tok(control_close).at.line, tok(control_close).at.column);
                        // What it declares is visible to the end of the statement it controls.
                        std::size_t controlled { control_close + 1 };
                        if (is(controlled, Kind::l_brace)) controlled = balanced(controlled) - 1;
                        else
                            while (controlled < to && !is(controlled, Kind::semi)) controlled = is(controlled, Kind::l_paren) || is(controlled, Kind::l_brace) ? balanced(controlled) : controlled + 1;
                        scope_ends_.push_back(controlled);
                        try_local(open + 1, owner, control_close, w == "catch" ? Parens::catch_ : Parens::control);
                        scope_ends_.pop_back();
                        // What try_local did not take is read on, token by token.
                        k = std::max(open + 1, std::min(i_, control_close));
                        if (i_ <= open + 1) k = open + 1;
                        boundary = false;
                        continue;
                    }
                }
                if (w == "else" || w == "do") {
                    boundary = true;
                    ++k;
                    continue;
                }
            }
            // A lambda: [captures] <T> (parameters) specifiers { body }. Not a subscript's `[`: after a
            // name (a keyword that begins an expression aside: `return [x = f()] { ... }`), a `)`, ...
            const auto begins_expression = [&](std::size_t p) {
                return word(p, "return") || word(p, "co_return") || word(p, "co_yield") || word(p, "throw");
            };
            if (t.kind == Kind::l_square && !is(k + 1, Kind::l_square) &&
                !(k > from && ((identifier(k - 1) && !begins_expression(k - 1)) || is(k - 1, Kind::r_paren) || is(k - 1, Kind::r_square) ||
                               is(k - 1, Kind::greater) || is_string(tok(k - 1).kind)))) {
                ++lambdas_;
                const std::size_t lambda_declarations { out_.declarations.size() };
                captures(k, owner);
                std::size_t j { balanced(k) };
                if (is(j, Kind::less)) {
                    const std::size_t after { angle(j) };
                    if (after != j) j = after;
                }
                if (is(j, Kind::l_paren)) {
                    parameters_of(j, owner, -1);
                    j = balanced(j);
                }
                --lambdas_;
                // Up to the body: mutable, constexpr, noexcept(...), attributes, -> type, requires ...
                std::size_t b { j };
                while (b < to && !is(b, Kind::l_brace) && !is(b, Kind::semi) && !is(b, Kind::r_paren) && !is(b, Kind::comma)) {
                    if (is(b, Kind::l_paren) || is(b, Kind::l_square)) b = balanced(b);
                    // A return type's template arguments (`-> std::expected<void, E>`): their `,` is theirs.
                    else if (is(b, Kind::less) && b > j && identifier(b - 1) && angle(b) != b) b = angle(b);
                    else ++b;
                }
                if (is(b, Kind::l_brace)) {
                    // Its parameters and captures are visible in its body.
                    for (std::size_t i { lambda_declarations }; i < out_.declarations.size(); ++i) {
                        out_.declarations[i].visible_end = static_cast<std::uint32_t>(balanced(b) - 1);
                        // An init-capture is not visible in the captures: `[x = std::move(x)]` moves the outer x.
                        if (out_.declarations[i].kind == msa::Kind::variable) out_.declarations[i].visible_begin = static_cast<std::uint32_t>(b);
                    }
                    lambda_body_next = true;
                    k = b;
                    continue;
                }
                k = j;
                boundary = false;
                continue;
            }
            if (t.kind == Kind::l_brace) {
                // A block where statements are, or a lambda's body; otherwise a braced initializer.
                const bool block { lambda_body_next || (contexts.back() && (boundary || (k > 0 && (is(k - 1, Kind::r_paren) || word(k - 1, "else") ||
                                                                                                      word(k - 1, "do") || word(k - 1, "try"))))) };
                contexts.push_back(block);
                if (block) scope_ends_.push_back(balanced(k) - 1);
                lambda_bodies.push_back(lambda_body_next);
                if (lambda_body_next) ++lambdas_;
                if (tracing_)
                    base::trace::debug(TRACE, "{}:{} `{{` {} (owner {})", t.at.line, t.at.column,
                                       lambda_body_next ? "a lambda's body" : block ? "a block" : "an initializer", owner);
                lambda_body_next = false;
                boundary = block;
                ++k;
                continue;
            }
            if (t.kind == Kind::r_brace) {
                if (contexts.size() > 1) {
                    if (contexts.back() && !scope_ends_.empty()) scope_ends_.pop_back();
                    contexts.pop_back();
                    if (lambda_bodies.back()) --lambdas_;
                    lambda_bodies.pop_back();
                }
                boundary = contexts.back();
                ++k;
                continue;
            }
            if (t.kind == Kind::semi) {
                // Not inside a control statement's parentheses: what follows an init-statement is its
                // condition (`if (auto f = g(); f && ok(*f))`), not a statement.
                boundary = contexts.back() && !(control_close != static_cast<std::size_t>(-1) && k < control_close);
                ++k;
                continue;
            }
            if (k == control_close) {
                boundary = true;
                control_close = static_cast<std::size_t>(-1);
                ++k;
                continue;
            }
            boundary = false;
            ++k;
        }
        // A lambda's body, a block, the range ended inside (broken text) is no longer being read.
        for (std::size_t open { 1 }; open < lambda_bodies.size(); ++open) {
            if (lambda_bodies[open]) --lambdas_;
            if (contexts[open] && !scope_ends_.empty()) scope_ends_.pop_back();
        }
        i_ = saved;
    }

    // A local declaration at `k`: T x, T* p = e, auto [..] aside. Records it (and the constructs in
    // its initializers) and leaves i_ after its declarators (at the `;`, `)` or `:`); false, i_
    // unchanged, when the tokens are not one. `parens`: in the ( ) of if/for/while/switch, or of a
    // catch -- what may end a declaration there differs.
    enum class Parens : std::uint8_t { none, control, catch_ };
    bool try_local(std::size_t k, std::int32_t owner, std::size_t limit, Parens parens = Parens::none) {
        const bool in_parens { parens != Parens::none };
        const std::size_t saved { i_ };
        if (k >= limit || any_word(k, STATEMENTS)) return false;
        i_ = k;
        const Scope scope { Context::block, {}, owner, false, false };
        const Specifiers sp { specifiers(scope, k) };
        const std::size_t specifiers_end { i_ };
        if (!sp.type || sp.friend_ || i_ >= limit) {
            i_ = saved;
            return false;
        }
        bool any { false };
        // A structured binding, `auto [a, b] = e;` (`&` or `&&` first for a reference): one variable, as
        // Clang has it -- named `[a, b]`, at its `[`.
        if (std::size_t b { i_ }; sp.auto_ && (is(b, Kind::l_square) || ((is(b, Kind::amp) || is(b, Kind::ampamp)) && is(++b, Kind::l_square)))) {
            std::string names;
            std::size_t j { b + 1 };
            while (identifier(j)) {
                names += (names.empty() ? "" : ", ") + std::string { tok(j).spelling };
                if (!is(++j, Kind::comma)) break;
                ++j;
            }
            if (!names.empty() && is(j, Kind::r_square) && j < limit &&
                (is(j + 1, Kind::equal) || is(j + 1, Kind::l_brace) || is(j + 1, Kind::l_paren) || (in_parens && is(j + 1, Kind::colon)))) {
                i_ = j + 1;
                const std::int32_t index { record(msa::Kind::variable, "[" + names + "]", b, k, j, scope, true, false, {}) };
                for (std::size_t name { b + 1 }; name < j; ++name)
                    if (identifier(name)) out_.declarations[static_cast<std::size_t>(record(msa::Kind::variable, std::string { tok(name).spelling }, name, name, name, scope, true, false, {}))].binding = true;
                initializer(owner, limit, in_parens);
                close(index, i_ - 1);
                return true;
            }
        }
        for (;;) {
            const std::size_t start { i_ };
            const Declarator d { declarator(scope) };
            // What ends a declarator of a declaration: in a statement, an initializer or `;` `,`; in a
            // condition, an initializer (`if (T x = e)`, `{e}`), an init-statement's `;` (a `(e)` only
            // there), a range-for's `:`; in a catch, the `)`. A reference is initialized, except a
            // catch's. So `if (a && b)` and `a & b;` are expressions, not declarations of references.
            const bool reference { d.pointer && !d.star && !d.member };
            const bool initialized { is(i_, Kind::equal) || is(i_, Kind::l_brace) || is(i_, Kind::l_paren) || is(i_, Kind::colon) };
            bool ends { false };
            switch (parens) {
            case Parens::none: ends = is(i_, Kind::equal) || is(i_, Kind::semi) || is(i_, Kind::comma) || is(i_, Kind::l_brace) || is(i_, Kind::l_paren); break;
            case Parens::control:
                ends = is(i_, Kind::equal) || is(i_, Kind::l_brace) || is(i_, Kind::colon) || is(i_, Kind::semi) || is(i_, Kind::comma) ||
                       (is(i_, Kind::l_paren) && (is(balanced(i_), Kind::semi) || is(balanced(i_), Kind::comma)));
                break;
            case Parens::catch_: ends = is(i_, Kind::r_paren); break;
            }
            if (reference && parens != Parens::catch_ && !initialized && !sp.extern_) ends = false;
            if (!d.ok || !d.id.qualifiers.empty() || d.id.op || d.id.destructor || !ends || d.function || i_ > limit) {
                if (!any) {
                    i_ = saved;
                    return false;
                }
                i_ = start;
                return true;
            }
            const std::int32_t index { record(sp.typedef_ ? msa::Kind::type_alias : msa::Kind::variable, d.id.spelled, d.id.last, k, i_ - 1, scope, true, false, {}) };
            typed(index, k, specifiers_end, start, i_, d);
            out_.declarations[static_cast<std::size_t>(index)].pointer = sp.pointer || d.star;
            out_.declarations[static_cast<std::size_t>(index)].c_array = d.array || sp.c_array;
            out_.declarations[static_cast<std::size_t>(index)].va_list = sp.va_list && !d.pointer;
            any = true;
            initializer(owner, limit, in_parens);
            close(index, i_ - 1);
            if (is(i_, Kind::comma) && !in_parens) {
                ++i_;
                continue;
            }
            return true;
        }
    }

    // A local's initializer at i_ (`= e`, `{...}`, `(...)`), if it has one: past it, its constructs read.
    void initializer(std::int32_t owner, std::size_t limit, bool in_parens) {
        if (is(i_, Kind::equal)) {
            const std::size_t from { i_ + 1 };
            ++i_;
            while (i_ < limit && !is(i_, Kind::semi) && !is(i_, Kind::comma) && !(in_parens && (is(i_, Kind::r_paren) || is(i_, Kind::colon)))) {
                if (is(i_, Kind::l_paren) || is(i_, Kind::l_square) || is(i_, Kind::l_brace)) i_ = balanced(i_);
                else i_ = std::min(past_template_arguments(i_), limit);
            }
            scan(from, i_, owner, false);
        } else if (is(i_, Kind::l_brace) || is(i_, Kind::l_paren)) {
            const std::size_t from { i_ };
            i_ = balanced(i_);
            scan(from, i_, owner, false);
        }
    }

    // A lambda's captures, [open] to its `]`: an init-capture (`x = e`, `&x = e`, `x{e}`, `...x = e`) is
    // a variable, as Clang has it; every initializer's constructs are read.
    void captures(std::size_t open, std::int32_t owner) {
        const std::size_t close_square { balanced(open) - 1 };
        const Scope scope { Context::block, {}, owner, false, false };
        for (std::size_t k { open + 1 }; k < close_square;) {
            std::size_t end { k };
            while (end < close_square && !is(end, Kind::comma)) {
                if (is(end, Kind::l_paren) || is(end, Kind::l_square) || is(end, Kind::l_brace)) end = balanced(end);
                else ++end;
            }
            end = std::min(end, close_square);
            std::size_t name { k };
            while (name < end && (is(name, Kind::amp) || is(name, Kind::ellipsis))) ++name;
            if (identifier(name) && name + 1 < end && (is(name + 1, Kind::equal) || is(name + 1, Kind::l_brace) || is(name + 1, Kind::l_paren))) {
                const std::int32_t index { record(msa::Kind::variable, std::string { tok(name).spelling }, name, k, end - 1, scope, true, false, {}) };
                scan(is(name + 1, Kind::equal) ? name + 2 : name + 1, end, owner, false);
                close(index, end - 1);
            }
            k = end + 1;
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

Syntax parse(std::string_view text, const PreprocessOptions& options, const Known& known) {
    Syntax out;
    out.pp = preprocess(text, options);
    out.line_starts.push_back(0);
    for (std::uint32_t i { 0 }; i < text.size(); ++i)
        if (text[i] == '\n' || (text[i] == '\r' && (i + 1 == text.size() || text[i + 1] != '\n'))) out.line_starts.push_back(i + 1);
    Parser parser { out, known };
    parser.run();
    return out;
}

std::map<std::string, Known::Alias, std::less<>> exported_aliases(const Syntax& syntax) {
    std::map<std::string, Known::Alias, std::less<>> out;
    for (const auto& d : syntax.declarations)
        if (d.kind == msa::Kind::type_alias && d.exported && !d.name.empty()) out.insert_or_assign(d.name, Known::Alias { d.pointer, d.c_array });
    return out;
}

} // namespace mcxx::frontend
