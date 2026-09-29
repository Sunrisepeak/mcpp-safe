// Declared types as text, as Clang's TypePrinter prints them: mcxx.frontend:types's definitions.
module mcxx.frontend;

import std;
import mcxx.msa;

namespace mcxx::frontend {

namespace {

using Tokens = std::span<const PpToken>;

bool is(const Tokens& t, std::size_t k, Kind kind) { return k < t.size() && t[k].kind == kind; }
bool word(const Tokens& t, std::size_t k, std::string_view w) { return k < t.size() && t[k].kind == Kind::raw_identifier && t[k].spelling == w; }

// Past the bracket at k: ( [ { < (a `<` only where the caller knows it opens template arguments).
std::size_t close_of(const Tokens& t, std::size_t k) {
    const Kind open { t[k].kind };
    const Kind close { open == Kind::l_paren ? Kind::r_paren : open == Kind::l_square ? Kind::r_square : open == Kind::l_brace ? Kind::r_brace : Kind::greater };
    int depth { 0 };
    for (std::size_t j { k }; j < t.size(); ++j) {
        const Kind kind { t[j].kind };
        if (kind == open) ++depth;
        else if (kind == close && --depth == 0) return j + 1;
        else if (open != Kind::l_paren && kind == Kind::l_paren) j = close_of(t, j) - 1;
        else if (open == Kind::less && kind == Kind::l_square) j = close_of(t, j) - 1;
    }
    return t.size();
}

// Top-level pieces of [from, to) split at `,` (brackets and template arguments held together).
std::vector<Tokens> split_commas(const Tokens& t) {
    std::vector<Tokens> out;
    std::size_t start { 0 };
    for (std::size_t k { 0 }; k < t.size(); ++k) {
        if (is(t, k, Kind::l_paren) || is(t, k, Kind::l_square) || is(t, k, Kind::l_brace) ||
            (is(t, k, Kind::less) && k > 0 && (t[k - 1].kind == Kind::raw_identifier || t[k - 1].kind == Kind::greater))) {
            k = close_of(t, k) - 1;
            continue;
        }
        if (is(t, k, Kind::comma)) {
            out.push_back(t.subspan(start, k - start));
            start = k + 1;
        }
    }
    if (start < t.size() || !out.empty()) out.push_back(t.subspan(start));
    return out;
}

constexpr std::string_view STORAGE[] {
    "static", "extern", "inline", "constexpr", "consteval", "constinit", "mutable", "thread_local", "register", "virtual", "explicit",
    "friend", "typedef", "__extension__", "_Thread_local", "__thread", "__inline", "__inline__", "_Noreturn", "export",
};
constexpr std::string_view BUILTIN[] {
    "void", "bool", "char", "wchar_t", "char8_t", "char16_t", "char32_t", "short", "int", "long", "signed", "unsigned", "float", "double",
    "__int128", "_Bool", "__signed", "__signed__", "__unsigned__",
};
bool builtin(std::string_view w) { return std::ranges::contains(BUILTIN, w); }

// A run of builtin type keywords, as Clang names the type: "unsigned int", "long", "unsigned long long".
std::optional<std::string> builtin_name(const std::vector<std::string_view>& words) {
    int longs { 0 }, shorts { 0 };
    bool is_unsigned { false }, is_signed { false }, is_int { false }, is_char { false };
    std::string_view other;
    for (const auto w : words) {
        if (w == "long") ++longs;
        else if (w == "short") ++shorts;
        else if (w == "unsigned" || w == "__unsigned__") is_unsigned = true;
        else if (w == "signed" || w == "__signed" || w == "__signed__") is_signed = true;
        else if (w == "int") is_int = true;
        else if (w == "char") is_char = true;
        else if (other.empty()) other = w;
        else return std::nullopt;
    }
    const std::string_view prefix { is_unsigned ? "unsigned " : "" };
    if (!other.empty()) {
        if (other == "double") return longs == 1 ? std::string { "long double" } : std::string { "double" };
        if (other == "__int128") return std::format("{}__int128", prefix);
        if (other == "_Bool") return std::string { "bool" };
        if (longs || shorts || is_int || is_char || is_unsigned || is_signed) return std::nullopt;
        return std::string { other };
    }
    if (is_char) return is_unsigned ? std::string { "unsigned char" } : is_signed ? std::string { "signed char" } : std::string { "char" };
    if (shorts) return std::format("{}short", prefix);
    if (longs == 1) return std::format("{}long", prefix);
    if (longs == 2) return std::format("{}long long", prefix);
    return std::format("{}int", prefix);
}

bool binary_operator(Kind k) {
    switch (k) {
    case Kind::plus: case Kind::minus: case Kind::star: case Kind::slash: case Kind::percent: case Kind::equalequal: case Kind::exclaimequal:
    case Kind::less: case Kind::greater: case Kind::lessequal: case Kind::greaterequal: case Kind::ampamp: case Kind::pipepipe: case Kind::amp:
    case Kind::pipe: case Kind::caret: case Kind::lessless: case Kind::greatergreater: case Kind::equal: case Kind::question: case Kind::colon:
        return true;
    default: return false;
    }
}

// An expression as Clang's StmtPrinter lays it out: binary operators spaced, `, ` between arguments,
// unary ones and everything else joined.
std::string expression_text(const Tokens& t) {
    std::string out;
    for (std::size_t k { 0 }; k < t.size(); ++k) {
        const Kind kind { t[k].kind };
        const bool operand_before { k > 0 && (t[k - 1].kind == Kind::raw_identifier || t[k - 1].kind == Kind::numeric_constant ||
                                              t[k - 1].kind == Kind::r_paren || t[k - 1].kind == Kind::r_square ||
                                              t[k - 1].kind == Kind::string_literal || t[k - 1].kind == Kind::char_constant) };
        if (binary_operator(kind) && operand_before) out += std::format(" {} ", t[k].spelling);
        else if (kind == Kind::comma) out += ", ";
        else {
            if (!out.empty() && k > 0 && t[k - 1].kind == Kind::raw_identifier && kind == Kind::raw_identifier) out += ' ';
            out += t[k].spelling;
        }
    }
    return out;
}

std::string type_of(const Tokens& t);
std::string named_type_text(Tokens tokens);

// A template argument: a type when its tokens read as one, an expression otherwise.
std::string argument_text(const Tokens& t) {
    if (t.empty()) return {};
    const bool expression { t[0].kind == Kind::numeric_constant || t[0].kind == Kind::string_literal || t[0].kind == Kind::char_constant ||
                            t[0].kind == Kind::minus || t[0].kind == Kind::exclaim || t[0].kind == Kind::l_paren || word(t, 0, "true") ||
                            word(t, 0, "false") || word(t, 0, "sizeof") || word(t, 0, "alignof") || word(t, 0, "nullptr") ||
                            std::ranges::any_of(t, [](const PpToken& x) { return x.kind == Kind::equalequal || x.kind == Kind::plus || x.kind == Kind::period; }) };
    if (expression) return expression_text(t);
    const std::string as_type { type_of(t) };
    return as_type.empty() ? expression_text(t) : as_type;
}

// A name as written, `::` joined, template arguments printed (`std::vector<std::vector<int>>`).
std::optional<std::string> name_text(const Tokens& t, std::size_t& k) {
    std::string out;
    if (is(t, k, Kind::coloncolon)) {
        out += "::";
        ++k;
    }
    for (;;) {
        if (word(t, k, "template")) {   // `typename T::template X<U>`
            out += "template ";
            ++k;
        }
        if (k >= t.size() || t[k].kind != Kind::raw_identifier) return std::nullopt;
        out += t[k].spelling;
        ++k;
        if (is(t, k, Kind::less)) {
            const std::size_t end { close_of(t, k) };
            if (end > t.size() || t[end - 1].kind != Kind::greater) return std::nullopt;
            std::vector<std::string> args;
            for (const auto& arg : split_commas(t.subspan(k + 1, end - 1 - (k + 1)))) {
                std::string printed { argument_text(arg) };
                if (printed.empty()) return std::nullopt;
                args.push_back(std::move(printed));
            }
            out += "<";
            for (std::size_t a { 0 }; a < args.size(); ++a) out += (a ? ", " : "") + args[a];
            out += ">";
            k = end;
        }
        if (!is(t, k, Kind::coloncolon)) return out;
        out += "::";
        ++k;
    }
}

struct Base {
    std::string text;
    bool is_const { false }, is_volatile { false };
    bool is_constexpr { false };   // a constexpr variable is const: Clang's type says so
};

// The decl-specifiers' type: the type-specifier printed, cv-qualifiers apart; storage and function
// specifiers and attributes dropped. Nothing when not followed (`auto`, a class defined here); a
// parameter's `auto` is its own (an invented template parameter's type, which Clang prints `auto`).
std::optional<Base> base_of(const Tokens& t, bool parameter = false) {
    Base b;
    std::vector<std::string_view> builtins;
    std::string named;
    std::string elaborated;
    for (std::size_t k { 0 }; k < t.size();) {
        const auto& x = t[k];
        if (x.kind == Kind::l_square && is(t, k + 1, Kind::l_square)) {   // [[attributes]]
            k = close_of(t, k);
            continue;
        }
        if (x.kind != Kind::raw_identifier && x.kind != Kind::coloncolon) return std::nullopt;
        const std::string_view w { x.spelling };
        if (w == "alignas" || w == "__attribute__" || w == "__declspec" || ((w == "explicit" || w == "noexcept") && is(t, k + 1, Kind::l_paren))) {
            k = is(t, k + 1, Kind::l_paren) ? close_of(t, k + 1) : k + 1;
            continue;
        }
        if (std::ranges::contains(STORAGE, w)) {
            b.is_constexpr = b.is_constexpr || w == "constexpr";
            ++k;
            continue;
        }
        if (w == "const") {
            b.is_const = true;
            ++k;
            continue;
        }
        if (w == "volatile") {
            b.is_volatile = true;
            ++k;
            continue;
        }
        if (w == "auto" && parameter && named.empty() && builtins.empty()) {
            named = "auto";
            ++k;
            continue;
        }
        if (w == "auto" || w == "__auto_type") return std::nullopt;   // deduced
        if (builtin(w)) {
            builtins.push_back(w);
            ++k;
            continue;
        }
        if (!named.empty()) return std::nullopt;
        if (w == "struct" || w == "class" || w == "union" || w == "enum") {
            elaborated = std::format("{} ", w);
            ++k;
            if (word(t, k, "class") || word(t, k, "struct")) ++k;   // enum class E
            continue;
        }
        if (w == "typename") {
            elaborated = "typename ";
            ++k;
            continue;
        }
        if ((w == "decltype" || w == "__typeof__" || w == "typeof") && is(t, k + 1, Kind::l_paren)) {
            if (word(t, k + 2, "auto")) return std::nullopt;   // decltype(auto): deduced
            const std::size_t end { close_of(t, k + 1) };
            named = std::format("decltype({})", expression_text(t.subspan(k + 2, end - 1 - (k + 2))));
            k = end;
            continue;
        }
        auto name { name_text(t, k) };
        if (!name) return std::nullopt;
        named = elaborated + *name;
    }
    if (!builtins.empty()) {
        if (!named.empty()) return std::nullopt;
        auto name { builtin_name(builtins) };
        if (!name) return std::nullopt;
        b.text = *name;
    } else {
        if (named.empty()) return std::nullopt;
        b.text = named;
    }
    return b;
}

// A function type's parameter list, (params) at t[open]: "(int, const std::string &)"; each
// parameter's name and default argument taken out. `(void)` is "()". Its cv/ref/noexcept after.
std::optional<std::string> parameters_text(const Tokens& t, std::size_t open, std::size_t& end) {
    end = close_of(t, open);
    const Tokens inside { t.subspan(open + 1, end - 1 - (open + 1)) };
    std::vector<std::string> params;
    bool variadic { false };
    if (!(inside.size() == 1 && word(inside, 0, "void"))) {
        for (auto p : split_commas(inside)) {
            if (p.empty()) continue;
            if (p.size() == 1 && p[0].kind == Kind::ellipsis) {
                variadic = true;
                continue;
            }
            // Its default argument off, then its name (the last identifier after a type's last token).
            for (std::size_t k { 0 }; k < p.size(); ++k) {
                if (is(p, k, Kind::less) && k > 0 && p[k - 1].kind == Kind::raw_identifier) k = close_of(p, k) - 1;
                else if (is(p, k, Kind::l_paren) || is(p, k, Kind::l_square)) k = close_of(p, k) - 1;
                else if (is(p, k, Kind::equal)) {
                    p = p.subspan(0, k);
                    break;
                }
            }
            std::string printed { named_type_text(p) };
            if (printed.empty()) return std::nullopt;
            params.push_back(std::move(printed));
        }
    }
    std::string out { "(" };
    for (std::size_t a { 0 }; a < params.size(); ++a) out += (a ? ", " : "") + params[a];
    if (variadic) out += params.empty() ? "..." : ", ...";
    out += ")";
    for (std::size_t k { end }; k < t.size(); ++k) {
        if (word(t, k, "const") || word(t, k, "volatile")) out += std::format(" {}", t[k].spelling);
        else if (word(t, k, "noexcept")) {
            if (is(t, k + 1, Kind::l_paren)) return std::nullopt;
            out += " noexcept";
        } else if (is(t, k, Kind::amp) || is(t, k, Kind::ampamp)) out += std::format(" {}", t[k].spelling);
        else break;
        end = k + 1;
    }
    return out;
}

// Pointer and reference operators as Clang prints them after their pointee: " *", "*const", " &".
void append_operator(std::string& out, std::string_view op) {
    if (!out.empty() && out.back() != '*' && out.back() != '&' && out.back() != '(') out += ' ';
    out += op;
}

// The declarator's part of the type, around `inner` (what an enclosing declarator put in the middle):
// ptr-operators before, [bounds] and (params) after. `t` is the declarator with its id taken out.
std::optional<std::string> declarator_text(const Tokens& t, std::string base) {
    std::size_t k { 0 };
    std::string ops;   // this level's operators, in order
    for (;;) {
        if (is(t, k, Kind::star) || is(t, k, Kind::amp) || is(t, k, Kind::ampamp)) {
            append_operator(ops, t[k].spelling);
            ++k;
            while (word(t, k, "const") || word(t, k, "volatile") || word(t, k, "__restrict") || word(t, k, "__restrict__")) {
                ops += t[k].spelling == "__restrict__" ? std::string_view { "__restrict" } : t[k].spelling;
                ++k;
                if (word(t, k, "const") || word(t, k, "volatile")) ops += ' ';
            }
            continue;
        }
        if (is(t, k, Kind::l_square) && is(t, k + 1, Kind::l_square)) {   // [[attributes]]
            k = close_of(t, k);
            continue;
        }
        // A member pointer: `C::*`, `ns::C::*`, printed as written.
        {
            std::size_t j { k };
            std::string cls;
            while (j + 1 < t.size() && t[j].kind == Kind::raw_identifier && is(t, j + 1, Kind::coloncolon)) {
                cls += std::string { t[j].spelling } + "::";
                j += 2;
            }
            if (!cls.empty() && is(t, j, Kind::star)) {
                append_operator(ops, cls + "*");
                k = j + 1;
                while (word(t, k, "const") || word(t, k, "volatile")) ops += t[k++].spelling;
                continue;
            }
        }
        break;
    }
    // A nested declarator: `(*)`, `(&)` around what is inside, then this level's suffixes.
    std::optional<std::pair<std::size_t, std::size_t>> nested;
    if (is(t, k, Kind::l_paren)) {
        const std::size_t end { close_of(t, k) };
        const Tokens inner { t.subspan(k + 1, end - 1 - (k + 1)) };
        if (!inner.empty() && (inner[0].kind == Kind::star || inner[0].kind == Kind::amp || inner[0].kind == Kind::ampamp)) {
            nested = std::pair { k + 1, end - 1 };
            k = end;
        }
    }
    bool pack { false };
    if (is(t, k, Kind::ellipsis)) {
        pack = true;
        ++k;
    }
    std::string suffix;
    while (k < t.size()) {
        if (is(t, k, Kind::l_square)) {
            const std::size_t end { close_of(t, k) };
            const Tokens bound { t.subspan(k + 1, end - 1 - (k + 1)) };
            if (bound.empty()) suffix += "[]";
            else if (bound.size() == 1 && bound[0].kind == Kind::numeric_constant) {
                std::string digits;
                for (const char c : bound[0].spelling)
                    if (c != '\'') digits += c;
                while (!digits.empty() && (digits.back() == 'u' || digits.back() == 'U' || digits.back() == 'l' || digits.back() == 'L')) digits.pop_back();
                if (digits.empty() || !std::ranges::all_of(digits, [](char c) { return c >= '0' && c <= '9'; })) return std::nullopt;
                suffix += std::format("[{}]", digits);
            } else return std::nullopt;   // Clang prints the bound's value
            k = end;
            continue;
        }
        if (is(t, k, Kind::l_paren)) {
            std::size_t end { 0 };
            auto params { parameters_text(t, k, end) };
            if (!params) return std::nullopt;
            suffix += *params;
            k = end;
            continue;
        }
        if (is(t, k, Kind::l_square) && is(t, k + 1, Kind::l_square)) {
            k = close_of(t, k);
            continue;
        }
        return std::nullopt;
    }
    if (nested) {
        // T (*)(params): the inner declarator's operators in parentheses, between the outer parts.
        std::string middle { "(" };
        const Tokens inner { t.subspan(nested->first, nested->second - nested->first) };
        auto inner_text { declarator_text(inner, {}) };
        if (!inner_text) return std::nullopt;
        middle += *inner_text + ")";
        std::string out { base };
        if (!ops.empty()) append_operator(out, ops);
        out += " " + middle + suffix;
        return out;
    }
    std::string out { base };
    if (!ops.empty()) append_operator(out, ops);
    // A function type: `void (int)`, but `int *(int)` -- a pointer or reference return type is followed at once.
    if (!suffix.empty() && suffix.front() == '(' && !out.ends_with('*') && !out.ends_with('&')) out += " ";
    out += suffix;
    if (pack) out += "...";
    return out;
}

// A whole type: decl-specifiers then an abstract declarator (a template argument, a parameter).
std::string type_of(const Tokens& t) {
    // The decl-specifiers end where a declarator's first operator, bracket or parenthesis starts.
    std::size_t k { 0 };
    while (k < t.size()) {
        if (t[k].kind == Kind::raw_identifier) {
            ++k;
            if (is(t, k, Kind::less) && t[k - 1].spelling != "operator") k = close_of(t, k);
            continue;
        }
        if (t[k].kind == Kind::coloncolon) {
            ++k;
            continue;
        }
        if (t[k].kind == Kind::l_square && is(t, k + 1, Kind::l_square)) {
            k = close_of(t, k);
            continue;
        }
        if (t[k].kind == Kind::l_paren && k > 0 && (t[k - 1].spelling == "decltype" || t[k - 1].spelling == "alignas" || t[k - 1].spelling == "__attribute__")) {
            k = close_of(t, k);
            continue;
        }
        break;
    }
    // `T C::*`: the class and its `::` are the member pointer's, the declarator's.
    {
        std::size_t q { k };
        while (q >= 2 && is(t, q - 1, Kind::coloncolon) && t[q - 2].kind == Kind::raw_identifier) q -= 2;
        if (q < k && q > 0 && is(t, k, Kind::star)) k = q;
    }
    const auto base { base_of(t.subspan(0, k)) };
    if (!base) return {};
    std::string text;
    if (base->is_const) text += "const ";
    if (base->is_volatile) text += "volatile ";
    text += base->text;
    const auto declared { declarator_text(t.subspan(k), text) };
    return declared.value_or(std::string {});
}

// The tokens with each `>>` as two `>`: in a type, it closes two template argument lists (a shift
// there needs parentheses).
std::vector<PpToken> split_shifts(std::span<const PpToken> tokens) {
    static constexpr std::string_view GREATER { ">" };
    std::vector<PpToken> out;
    out.reserve(tokens.size());
    for (const auto& t : tokens) {
        if (t.kind != Kind::greatergreater) {
            out.push_back(t);
            continue;
        }
        PpToken half { t };
        half.kind = Kind::greater;
        half.spelling = GREATER;
        out.push_back(half);
        out.push_back(half);
    }
    return out;
}

std::string named_type_text(Tokens tokens) {
    // A named parameter: its name is the last identifier, after a type's last token.
    if (tokens.size() >= 2 && tokens.back().kind == Kind::raw_identifier && !builtin(tokens.back().spelling) &&
        tokens.back().spelling != "const" && tokens.back().spelling != "volatile") {
        const auto& before = tokens[tokens.size() - 2];
        if (before.kind == Kind::raw_identifier || before.kind == Kind::greater || before.kind == Kind::star || before.kind == Kind::amp ||
            before.kind == Kind::ampamp || before.kind == Kind::ellipsis)
            if (!(before.kind == Kind::raw_identifier && (before.spelling == "struct" || before.spelling == "class" || before.spelling == "enum" ||
                                                          before.spelling == "union" || before.spelling == "typename")))
                tokens = tokens.subspan(0, tokens.size() - 1);
    }
    return type_of(tokens);
}

} // namespace

std::string type_text(std::span<const PpToken> tokens) {
    const auto split { split_shifts(tokens) };
    return named_type_text(split);
}

std::string type_text(const Syntax& syntax, const Declaration& d, bool return_type) {
    const auto& all = syntax.pp.tokens;
    if (d.specifiers_end == 0 || d.specifiers_end > all.size() || d.declarator_end > all.size() || d.specifiers_begin > d.specifiers_end ||
        d.declarator_begin > d.declarator_end)
        return {};
    // An alias-declaration's type (`using X = a::B;`) has no declarator-id: what the declarator read as
    // one is the last part of the type's name.
    if (d.kind == msa::Kind::type_alias && d.id_end > d.id_begin && d.specifiers_begin > 0 && all[d.specifiers_begin - 1].kind == Kind::equal) {
        Declaration whole { d };
        whole.specifiers_end = d.id_end;
        whole.declarator_begin = d.id_end;
        whole.id_begin = whole.id_end = 0;
        return type_text(syntax, whole, return_type);
    }
    const auto specifier_tokens { split_shifts(std::span { all }.subspan(d.specifiers_begin, d.specifiers_end - d.specifiers_begin)) };
    const Tokens specifiers { specifier_tokens };
    // The declarator with its id taken out; a function's return type: what precedes its name.
    std::vector<PpToken> written;
    const std::uint32_t declarator_end { return_type && d.id_end > d.id_begin ? d.id_begin : d.declarator_end };
    for (std::uint32_t k { d.declarator_begin }; k < declarator_end; ++k)
        if (d.id_end <= d.id_begin || k < d.id_begin || k >= d.id_end) written.push_back(all[k]);
    const auto declarator { split_shifts(written) };
    // A class defined in the specifiers (`struct S { ... } s;`) is not followed.
    if (std::ranges::any_of(specifiers, [](const PpToken& t) { return t.kind == Kind::l_brace; })) return {};
    const auto base { base_of(specifiers, d.kind == msa::Kind::parameter) };
    if (!base) return {};
    // A constexpr variable is const at its top level: its base's, or its outermost pointer's.
    const bool outer_pointer { !declarator.empty() && std::ranges::any_of(declarator, [](const PpToken& t) { return t.kind == Kind::star; }) };
    const bool outer_reference { std::ranges::any_of(declarator, [](const PpToken& t) { return t.kind == Kind::amp || t.kind == Kind::ampamp; }) };
    const bool constexpr_const { base->is_constexpr && !return_type && d.kind != msa::Kind::parameter && !outer_reference };
    std::string text;
    if (base->is_const || (constexpr_const && !outer_pointer)) text += "const ";
    if (base->is_volatile) text += "volatile ";
    text += base->text;
    auto declared { declarator_text(declarator, text) };
    if (!declared) return {};
    // The pointer itself is const: `const char *const`, before an array's bound (`const char *const[3]`).
    if (constexpr_const && outer_pointer && !declared->ends_with("const")) {
        const auto bracket = declared->find('[');
        if (bracket == std::string::npos) *declared += "const";
        else if (declared->substr(0, bracket).ends_with('*')) declared->insert(bracket, "const");
    }
    return *declared;
}

} // namespace mcxx::frontend
