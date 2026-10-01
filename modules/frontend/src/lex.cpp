// C++23's raw lexer, rule for rule as Clang 23.1's (lib/Lex/Lexer.cpp): mcxx.frontend:lex's definitions.
module mcxx.frontend;

import std;

namespace mcxx::frontend {

std::string_view name(Kind kind) {
    static constexpr std::string_view names[] {
        "whitespace", "comment", "unknown",
        "raw_identifier", "numeric_constant",
        "char_constant", "wide_char_constant", "utf8_char_constant", "utf16_char_constant", "utf32_char_constant",
        "string_literal", "wide_string_literal", "utf8_string_literal", "utf16_string_literal", "utf32_string_literal",
        "l_square", "r_square", "l_paren", "r_paren", "l_brace", "r_brace", "period", "ellipsis", "amp", "ampamp", "ampequal", "star", "starequal",
        "plus", "plusplus", "plusequal", "minus", "arrow", "minusminus", "minusequal", "tilde", "exclaim", "exclaimequal", "slash", "slashequal",
        "percent", "percentequal", "less", "lessless", "lessequal", "lesslessequal", "spaceship", "greater", "greatergreater", "greaterequal",
        "greatergreaterequal", "caret", "caretequal", "pipe", "pipepipe", "pipeequal", "question", "colon", "semi", "equal", "equalequal", "comma",
        "hash", "hashhash", "periodstar", "arrowstar", "coloncolon",
    };
    return names[static_cast<std::size_t>(kind)];
}

namespace {

bool letter(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }
bool digit(char c) { return c >= '0' && c <= '9'; }
bool ascii(char c) { return static_cast<unsigned char>(c) < 0x80; }
// [A-Za-z0-9_]: what follows a digit separator.
bool word(char c) { return letter(c) || digit(c) || c == '_'; }
// An identifier's ASCII characters; `$` is one, as Clang allows by default.
bool identifier_start(char c) { return letter(c) || c == '_' || c == '$'; }
bool identifier_continue(char c) { return identifier_start(c) || digit(c); }
// A pp-number's body: [A-Za-z0-9_.].
bool number_body(char c) { return word(c) || c == '.'; }
bool horizontal_space(char c) { return c == ' ' || c == '\t' || c == '\v' || c == '\f'; }
bool space(char c) { return horizontal_space(c) || c == '\n' || c == '\r'; }
bool hex_value(char c, std::uint32_t& value) {
    if (digit(c)) value = static_cast<std::uint32_t>(c - '0');
    else if (c >= 'a' && c <= 'f') value = static_cast<std::uint32_t>(c - 'a' + 10);
    else if (c >= 'A' && c <= 'F') value = static_cast<std::uint32_t>(c - 'A' + 10);
    else return false;
    return true;
}
// A raw string delimiter's characters: the basic character set's graphic characters but `(`, `)`
// and `\` (Clang's isRawStringDelimBody: `$`, `@` and the backquote too, as C++26 allows).
bool delimiter_char(char c) {
    return letter(c) || digit(c) || (c > ' ' && c < 0x7F && c != '(' && c != ')' && c != '\\');
}

struct Decoded {
    char32_t code_point;
    std::size_t size;
};

// The UTF-8 sequence at `p`, strictly (no overlong forms, surrogates or code points past U+10FFFF).
std::optional<Decoded> decode(std::string_view s, std::size_t p) {
    const auto byte = [&](std::size_t i) { return p + i < s.size() ? static_cast<unsigned char>(s[p + i]) : 0u; };
    const unsigned lead { byte(0) };
    std::size_t size { 0 };
    unsigned low { 0x80 }, high { 0xBF };
    char32_t value { 0 };
    if (lead >= 0xC2 && lead <= 0xDF) size = 2, value = lead & 0x1F;
    else if (lead >= 0xE0 && lead <= 0xEF) {
        size = 3, value = lead & 0x0F;
        if (lead == 0xE0) low = 0xA0;
        if (lead == 0xED) high = 0x9F;
    } else if (lead >= 0xF0 && lead <= 0xF4) {
        size = 4, value = lead & 0x07;
        if (lead == 0xF0) low = 0x90;
        if (lead == 0xF4) high = 0x8F;
    } else return std::nullopt;
    for (std::size_t i { 1 }; i < size; ++i) {
        const unsigned b { byte(i) };
        if (b < (i == 1 ? low : 0x80) || b > (i == 1 ? high : 0xBF)) return std::nullopt;
        value = (value << 6) | (b & 0x3F);
    }
    return Decoded { value, size };
}

// Reads the text as translation phase 2 sees it: a backslash, optional horizontal whitespace and a
// line break (\n, \r, \r\n or \n\r) are not there. Positions are the text's own bytes.
class Cursor {
public:
    // A UTF-8 byte order mark at the start is not part of the text.
    explicit Cursor(std::string_view text) : s_ { text }, i_ { text.starts_with("\xEF\xBB\xBF") ? 3u : 0u } {}

    // Where the cursor is: past the last character read, before any splice that follows it.
    std::size_t at() const { return i_; }
    // Where the next character is: past any splice.
    std::size_t here() const { return skip(i_); }
    bool done() const { return here() >= s_.size(); }
    // The character `n` characters ahead ('\0' past the end).
    char peek(std::size_t n = 0) const {
        std::size_t p { skip(i_) };
        for (; n > 0 && p < s_.size(); --n) p = skip(p + 1);
        return p < s_.size() ? s_[p] : '\0';
    }
    char get() {
        i_ = skip(i_);
        return i_ < s_.size() ? s_[i_++] : '\0';
    }
    void advance(std::size_t n) {
        while (n-- > 0) get();
    }
    // To a byte of the text: after a raw string or a UTF-8 sequence, which are read as bytes.
    void jump(std::size_t p) { i_ = std::min(p, s_.size()); }

private:
    std::string_view s_;
    std::size_t i_;

    std::size_t skip(std::size_t p) const {
        while (p + 1 < s_.size() && s_[p] == '\\') {
            std::size_t q { p + 1 };
            while (q < s_.size() && horizontal_space(s_[q])) ++q;
            if (q == s_.size() || (s_[q] != '\n' && s_[q] != '\r')) break;
            p = q + 1 < s_.size() && (s_[q + 1] == '\n' || s_[q + 1] == '\r') && s_[q + 1] != s_[q] ? q + 2 : q + 1;
        }
        return p;
    }
};

struct Ucn {
    // What it names, when it is a code point a UCN may name outside a literal (U+00A0 and up, not a
    // surrogate); none for any other, and for every \N{name}: names are not looked up (no Unicode
    // name table here -- Clang has one, so a \N{} naming a letter is where the two part, and only there).
    std::optional<char32_t> code_point;
    std::size_t length;   // in characters, splices aside
};

// The universal character name `n` characters ahead, when it is one as written: \uXXXX, \UXXXXXXXX,
// \u{X...} or \N{name} (Clang's tryReadNumericUCN and tryReadNamedUCN).
std::optional<Ucn> ucn(const Cursor& c, std::size_t n) {
    if (c.peek(n) != '\\') return std::nullopt;
    const char kind { c.peek(n + 1) };
    if (kind == 'N') {
        if (c.peek(n + 2) != '{') return std::nullopt;
        std::size_t k { n + 3 };
        for (;; ++k) {
            const char x { c.peek(k) };
            if (x == '}') break;
            if (x == '\0' || x == '\n' || x == '\r') return std::nullopt;
        }
        if (k == n + 3) return std::nullopt;
        return Ucn { std::nullopt, k + 1 - n };
    }
    if (kind != 'u' && kind != 'U') return std::nullopt;
    const std::size_t digits { kind == 'u' ? 4u : 8u };
    std::size_t k { n + 2 }, count { 0 };
    bool delimited { false };
    std::uint32_t value { 0 };
    while (count != digits || delimited) {
        const char x { c.peek(k) };
        if (!delimited && count == 0 && x == '{') {
            delimited = true;
            ++k;
            continue;
        }
        if (delimited && x == '}') {
            ++k;
            break;
        }
        std::uint32_t v { 0 };
        if (!hex_value(x, v)) {
            if (!delimited) break;
            return std::nullopt;
        }
        if ((value & 0xF000'0000u) != 0) return std::nullopt;
        value = (value << 4) | v;
        ++k;
        ++count;
    }
    if (count == 0 || (delimited && kind == 'U') || (!delimited && count != digits)) return std::nullopt;
    if (value < 0xA0 || (value >= 0xD800 && value <= 0xDFFF)) return Ucn { std::nullopt, k - n };
    return Ucn { static_cast<char32_t>(value), k - n };
}

// A UCN `n` characters ahead that may continue an identifier: its length, or 0.
std::size_t continuing_ucn(const Cursor& c, std::size_t n) {
    const auto u = ucn(c, n);
    return u && u->code_point && unicode::identifier_continue(*u->code_point) ? u->length : 0;
}

// The suffixes the standard library declares for string literals (C++23): Clang takes one of these
// after a string as its ud-suffix, and otherwise only a suffix that starts with `_`.
bool standard_string_suffix(std::string_view s) {
    return s == "s" || s == "sv" || s == "h" || s == "min" || s == "ms" || s == "us" || s == "ns" || s == "il" || s == "i" || s == "if" ||
           s == "d" || s == "y";
}

} // namespace

std::vector<Token> lex(std::string_view text, Options options) {
    std::vector<Token> out;
    out.reserve(text.size() / 4);
    Cursor c { text };
    // Line starts, for a byte's line and column: \n, \r\n and a lone \r end a line.
    std::vector<std::uint32_t> starts { 0 };
    for (std::uint32_t i { 0 }; i < text.size(); ++i)
        if (text[i] == '\n' || (text[i] == '\r' && (i + 1 == text.size() || text[i + 1] != '\n'))) starts.push_back(i + 1);
    bool start_of_line { true };
    bool leading_space { false };
    auto emit = [&](Kind kind, std::size_t begin) {
        Token t { kind, static_cast<std::uint32_t>(begin), static_cast<std::uint32_t>(c.at()) };
        const auto it = std::ranges::upper_bound(starts, t.begin);
        t.line = static_cast<std::uint32_t>(it - starts.begin());
        t.column = t.begin - *(it - 1) + 1;
        if (kind != Kind::whitespace && kind != Kind::comment) {
            t.start_of_line = start_of_line;
            t.leading_space = leading_space;
            start_of_line = false;
            leading_space = false;
        } else {
            leading_space = true;
        }
        if ((kind != Kind::whitespace || options.whitespace) && (kind != Kind::comment || options.comments)) out.push_back(t);
    };
    // The rest of an identifier: ASCII letters, digits, `_` and `$`, and UCNs and UTF-8 characters
    // that may continue one.
    auto identifier_rest = [&] {
        for (;;) {
            const char x { c.peek() };
            if (identifier_continue(x)) {
                c.get();
            } else if (x == '\\') {
                const std::size_t n { continuing_ucn(c, 0) };
                if (n == 0) return;
                c.advance(n);
            } else if (!ascii(x)) {
                const std::size_t p { c.here() };
                const auto d = decode(text, p);
                if (!d || !unicode::identifier_continue(d->code_point)) return;
                c.jump(p + d->size);
            } else {
                return;
            }
        }
    };
    // A string's or character's ud-suffix, as Clang takes it (Lexer::LexUDSuffix).
    auto literal_suffix = [&](bool string) {
        const char first { c.peek() };
        if (!identifier_start(first)) {
            if (first == '\\') {
                const std::size_t n { continuing_ucn(c, 0) };
                if (n == 0) return;
                c.advance(n);
            } else if (!ascii(first)) {
                const std::size_t p { c.here() };
                const auto d = decode(text, p);
                if (!d || !unicode::identifier_continue(d->code_point)) return;
                c.jump(p + d->size);
            } else {
                return;
            }
        } else if (first != '_') {
            if (!string) return;
            std::string suffix { first };
            for (std::size_t k { 1 };; ++k) {
                const char x { c.peek(k) };
                if (!identifier_continue(x)) break;
                if (suffix.size() == 3) return;
                suffix += x;
            }
            if (!standard_string_suffix(suffix)) return;
            c.get();
        } else {
            c.get();
        }
        identifier_rest();
    };
    // A character or string literal from its prefix (`n` characters) on.
    auto quoted = [&](std::size_t begin, std::size_t n, char quote, Kind kind) {
        c.advance(n + 1);
        if (quote == '\'' && !c.done() && c.peek() == '\'') {   // '' is not a literal
            c.get();
            emit(Kind::unknown, begin);
            return;
        }
        for (;;) {
            if (c.done()) break;
            char x { c.peek() };
            if (x == quote) {
                c.get();
                literal_suffix(quote == '"');
                emit(kind, begin);
                return;
            }
            if (x == '\\') {
                c.get();
                if (c.done()) break;
                x = c.peek();
            }
            if (x == '\n' || x == '\r') break;
            c.get();
        }
        // Unterminated: everything up to the line break or the end is one unknown token.
        c.jump(c.here());
        emit(Kind::unknown, begin);
    };
    // A raw string from its prefix (`n` characters, then R") on; its body is read as bytes.
    auto raw_string = [&](std::size_t begin, std::size_t n, Kind kind) {
        c.advance(n + 2);
        const std::size_t from { c.at() };
        std::size_t d { 0 };
        while (d != 16 && from + d < text.size() && delimiter_char(text[from + d])) ++d;
        if (from + d == text.size() || text[from + d] != '(') {
            // Not a delimiter: up to the next quote is one unknown token.
            const std::size_t q { text.find('"', from) };
            c.jump(q == std::string_view::npos ? text.size() : q + 1);
            emit(Kind::unknown, begin);
            return;
        }
        std::string close { ")" };
        close += text.substr(from, d);
        close += '"';
        const std::size_t found { text.find(close, from + d + 1) };
        if (found == std::string_view::npos) {
            c.jump(text.size());
            emit(Kind::unknown, begin);
            return;
        }
        c.jump(found + close.size());
        literal_suffix(true);
        emit(kind, begin);
    };
    auto number = [&](std::size_t begin) {
        const bool hex { c.peek() == '0' && (c.peek(1) == 'x' || c.peek(1) == 'X') };
        char previous { 0 };
        for (;;) {
            const char x { c.peek() };
            if (number_body(x)) {
                c.get();
                previous = x;
                continue;
            }
            if ((x == '+' || x == '-') && (previous == 'e' || previous == 'E' || (hex && (previous == 'p' || previous == 'P')))) {
                c.get();
            } else if (x == '\'' && word(c.peek(1))) {
                c.advance(2);
            } else if (x == '$') {
                c.get();
            } else if (x == '\\') {
                const std::size_t n { continuing_ucn(c, 0) };
                if (n == 0) break;
                c.advance(n);
            } else if (!ascii(x)) {
                const std::size_t p { c.here() };
                const auto d = decode(text, p);
                if (!d || !unicode::identifier_continue(d->code_point)) break;
                c.jump(p + d->size);
            } else {
                break;
            }
            previous = 0;
        }
        emit(Kind::numeric_constant, begin);
    };
    auto punctuator = [&](std::size_t begin, std::size_t n, Kind kind) {
        c.advance(n);
        emit(kind, begin);
    };

    while (!c.done()) {
        // A token starts where the last one ended: splices right before it are its own, as in Clang.
        const std::size_t begin { c.at() };
        const char ch { c.peek() };
        // Whitespace; a NUL byte inside the text is whitespace too.
        if (space(ch) || ch == '\0') {
            while (!c.done() && (space(c.peek()) || c.peek() == '\0')) {
                if (c.peek() == '\n' || c.peek() == '\r') start_of_line = true;
                c.get();
            }
            emit(Kind::whitespace, begin);
            continue;
        }
        const char a { c.peek(1) };
        switch (ch) {
        case '/':
            if (a == '/') {
                while (!c.done() && c.peek() != '\n' && c.peek() != '\r') c.get();
                emit(Kind::comment, begin);
            } else if (a == '*') {
                c.advance(2);
                bool closed { false };
                while (!c.done() && !closed) closed = c.get() == '*' && c.peek() == '/' && c.get() == '/';
                if (!closed) c.jump(text.size());
                emit(closed ? Kind::comment : Kind::unknown, begin);
            } else {
                punctuator(begin, a == '=' ? 2 : 1, a == '=' ? Kind::slashequal : Kind::slash);
            }
            continue;
        case 'u':
        case 'U':
        case 'L': {
            // u8"..." u8'...' u8R"(...)"; u"..." u'...' uR"(...)", and the same with U and L.
            const bool u8 { ch == 'u' && a == '8' };
            const std::size_t n { u8 ? 2u : 1u };
            const Kind string_kind { u8 ? Kind::utf8_string_literal : ch == 'u' ? Kind::utf16_string_literal : ch == 'U' ? Kind::utf32_string_literal : Kind::wide_string_literal };
            const Kind char_kind { u8 ? Kind::utf8_char_constant : ch == 'u' ? Kind::utf16_char_constant : ch == 'U' ? Kind::utf32_char_constant : Kind::wide_char_constant };
            const char next { c.peek(n) };
            if (next == '"' || next == '\'') {
                quoted(begin, n, next, next == '"' ? string_kind : char_kind);
                continue;
            }
            if (next == 'R' && c.peek(n + 1) == '"') {
                raw_string(begin, n, string_kind);
                continue;
            }
            break;
        }
        case 'R':
            if (a == '"') {
                raw_string(begin, 0, Kind::string_literal);
                continue;
            }
            break;
        case '"':
        case '\'':
            quoted(begin, 0, ch, ch == '"' ? Kind::string_literal : Kind::char_constant);
            continue;
        case '.':
            if (digit(a)) number(begin);
            else if (a == '*') punctuator(begin, 2, Kind::periodstar);
            else if (a == '.' && c.peek(2) == '.') punctuator(begin, 3, Kind::ellipsis);
            else punctuator(begin, 1, Kind::period);
            continue;
        case '\\':
            // A UCN as written is read whole, whatever it names; it starts an identifier when it
            // names a letter, and is one unknown token otherwise.
            if (const auto u = ucn(c, 0)) {
                c.advance(u->length);
                if (u->code_point && unicode::identifier_start(*u->code_point)) {
                    identifier_rest();
                    emit(Kind::raw_identifier, begin);
                } else {
                    emit(Kind::unknown, begin);
                }
            } else {
                punctuator(begin, 1, Kind::unknown);
            }
            continue;
        case '[': punctuator(begin, 1, Kind::l_square); continue;
        case ']': punctuator(begin, 1, Kind::r_square); continue;
        case '(': punctuator(begin, 1, Kind::l_paren); continue;
        case ')': punctuator(begin, 1, Kind::r_paren); continue;
        case '{': punctuator(begin, 1, Kind::l_brace); continue;
        case '}': punctuator(begin, 1, Kind::r_brace); continue;
        case '?': punctuator(begin, 1, Kind::question); continue;
        case '~': punctuator(begin, 1, Kind::tilde); continue;
        case ',': punctuator(begin, 1, Kind::comma); continue;
        case ';': punctuator(begin, 1, Kind::semi); continue;
        case '&':
            if (a == '&') punctuator(begin, 2, Kind::ampamp);
            else if (a == '=') punctuator(begin, 2, Kind::ampequal);
            else punctuator(begin, 1, Kind::amp);
            continue;
        case '*': punctuator(begin, a == '=' ? 2 : 1, a == '=' ? Kind::starequal : Kind::star); continue;
        case '+':
            if (a == '+') punctuator(begin, 2, Kind::plusplus);
            else if (a == '=') punctuator(begin, 2, Kind::plusequal);
            else punctuator(begin, 1, Kind::plus);
            continue;
        case '-':
            if (a == '-') punctuator(begin, 2, Kind::minusminus);
            else if (a == '>' && c.peek(2) == '*') punctuator(begin, 3, Kind::arrowstar);
            else if (a == '>') punctuator(begin, 2, Kind::arrow);
            else if (a == '=') punctuator(begin, 2, Kind::minusequal);
            else punctuator(begin, 1, Kind::minus);
            continue;
        case '!': punctuator(begin, a == '=' ? 2 : 1, a == '=' ? Kind::exclaimequal : Kind::exclaim); continue;
        case '%':
            if (a == '=') punctuator(begin, 2, Kind::percentequal);
            else if (a == '>') punctuator(begin, 2, Kind::r_brace);
            else if (a == ':' && c.peek(2) == '%' && c.peek(3) == ':') punctuator(begin, 4, Kind::hashhash);
            else if (a == ':') punctuator(begin, 2, Kind::hash);
            else punctuator(begin, 1, Kind::percent);
            continue;
        case '<':
            if (a == '<') punctuator(begin, c.peek(2) == '=' ? 3 : 2, c.peek(2) == '=' ? Kind::lesslessequal : Kind::lessless);
            else if (a == '=') punctuator(begin, c.peek(2) == '>' ? 3 : 2, c.peek(2) == '>' ? Kind::spaceship : Kind::lessequal);
            // `<::` is `<` `::` unless `<:::` or `<::>` (C++11 [lex.pptoken]/3.2).
            else if (a == ':' && !(c.peek(2) == ':' && c.peek(3) != ':' && c.peek(3) != '>')) punctuator(begin, 2, Kind::l_square);
            else if (a == '%') punctuator(begin, 2, Kind::l_brace);
            else punctuator(begin, 1, Kind::less);
            continue;
        case '>':
            if (a == '=') punctuator(begin, 2, Kind::greaterequal);
            else if (a == '>') punctuator(begin, c.peek(2) == '=' ? 3 : 2, c.peek(2) == '=' ? Kind::greatergreaterequal : Kind::greatergreater);
            else punctuator(begin, 1, Kind::greater);
            continue;
        case '^': punctuator(begin, a == '=' ? 2 : 1, a == '=' ? Kind::caretequal : Kind::caret); continue;
        case '|':
            if (a == '=') punctuator(begin, 2, Kind::pipeequal);
            else if (a == '|') punctuator(begin, 2, Kind::pipepipe);
            else punctuator(begin, 1, Kind::pipe);
            continue;
        case ':':
            if (a == '>') punctuator(begin, 2, Kind::r_square);
            else if (a == ':') punctuator(begin, 2, Kind::coloncolon);
            else punctuator(begin, 1, Kind::colon);
            continue;
        case '=': punctuator(begin, a == '=' ? 2 : 1, a == '=' ? Kind::equalequal : Kind::equal); continue;
        case '#': punctuator(begin, a == '#' ? 2 : 1, a == '#' ? Kind::hashhash : Kind::hash); continue;
        default:
            break;
        }
        if (digit(ch)) {
            number(begin);
        } else if (identifier_start(ch)) {
            c.get();
            identifier_rest();
            emit(Kind::raw_identifier, begin);
        } else if (!ascii(ch)) {
            // A UTF-8 character that may start an identifier does; any other is one unknown token,
            // and a byte that is not UTF-8 one by itself.
            const std::size_t p { c.here() };
            const auto d = decode(text, p);
            c.jump(p + (d ? d->size : 1));
            if (d && unicode::identifier_start(d->code_point)) {
                identifier_rest();
                emit(Kind::raw_identifier, begin);
            } else {
                emit(Kind::unknown, begin);
            }
        } else {
            punctuator(begin, 1, Kind::unknown);
        }
    }
    return out;
}

std::string spelling(std::string_view text, const Token& token) {
    std::string out;
    Cursor c { text.substr(0, token.end) };
    c.jump(token.begin);
    while (c.at() < token.end && !c.done()) {
        out += c.get();
        // A raw string's body is as written: from its opening quote on, splices stay.
        if (out.back() == '"' && out.size() >= 2 && out[out.size() - 2] == 'R') {
            out += text.substr(c.at(), token.end - c.at());
            break;
        }
    }
    return out;
}

bool uses_mathematical_notation(std::string_view text, const Token& token) {
    if (token.kind != Kind::raw_identifier) return false;
    const std::string s { spelling(text, token) };
    if (std::ranges::all_of(s, [](char c) { return ascii(c) && c != '\\'; })) return false;
    bool first { true };
    for (std::size_t i { 0 }; i < s.size(); first = false) {
        char32_t cp { static_cast<unsigned char>(s[i]) };
        std::size_t size { 1 };
        if (!ascii(s[i])) {
            const auto d = decode(s, i);
            if (!d) {
                ++i;
                continue;
            }
            cp = d->code_point, size = d->size;
        } else if (s[i] == '\\') {
            // \uXXXX, \UXXXXXXXX, \u{X...}, read as the lexer reads them (a \N{name} is not looked up).
            const Cursor c { std::string_view { s }.substr(i) };
            const auto u = ucn(c, 0);
            if (!u || !u->code_point) {
                i += u ? u->length : 1;
                continue;
            }
            cp = *u->code_point, size = u->length;
        }
        i += size;
        if (cp >= 0x80 && (first ? unicode::math_start(cp) : unicode::math_continue(cp))) return true;
    }
    return false;
}

} // namespace mcxx::frontend
