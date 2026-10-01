// mcxx.frontend:lex -- C++23 lexing, raw (no preprocessing), as Clang's raw lexer does it: the tokens
// a translation phase 3 gives, each with the kind name Clang uses (`raw_identifier`, `l_paren`,
// `utf8_string_literal`...), so the two are compared token for token (tools/checks/lexdiff.py).
//
//   for (const auto& t : mcxx::frontend::lex(text)) ... t.kind, t.begin, t.end, t.line, t.column
//
// What it covers, each as Clang 23.1's raw lexer does it under C++23 (lib/Lex/Lexer.cpp):
// identifiers (keywords are identifiers here, as in a raw lexer; `$`, universal character names and
// UTF-8 letters by UAX #31, :unicode), pp-numbers (digit separators, exponents and a hexadecimal
// float's `p+`), character and string literals with every prefix, raw strings, user-defined literal
// suffixes (a string's or character's only when it starts with `_` or is the standard library's),
// every punctuator and digraph with the `<::` rule, comments, line splices (a backslash, optional
// spaces and a line break join the lines, inside any token but a raw string's body), a byte order
// mark, and what is not a token: an empty or unterminated literal, a bad raw string delimiter, an
// unterminated comment, a stray character are each one `unknown` token. The one place it parts from
// Clang: a \N{name} outside a literal naming a letter (Clang looks the name up; there is no Unicode
// name table here), which Clang reads as an identifier and this as `unknown`.
export module mcxx.frontend:lex;

import std;
import :unicode;

export namespace mcxx::frontend {

enum class Kind : std::uint8_t {
    whitespace, comment, unknown,
    raw_identifier, numeric_constant,
    char_constant, wide_char_constant, utf8_char_constant, utf16_char_constant, utf32_char_constant,
    string_literal, wide_string_literal, utf8_string_literal, utf16_string_literal, utf32_string_literal,
    l_square, r_square, l_paren, r_paren, l_brace, r_brace, period, ellipsis, amp, ampamp, ampequal, star, starequal,
    plus, plusplus, plusequal, minus, arrow, minusminus, minusequal, tilde, exclaim, exclaimequal, slash, slashequal,
    percent, percentequal, less, lessless, lessequal, lesslessequal, spaceship, greater, greatergreater, greaterequal,
    greatergreaterequal, caret, caretequal, pipe, pipepipe, pipeequal, question, colon, semi, equal, equalequal, comma,
    hash, hashhash, periodstar, arrowstar, coloncolon,
};

// The kind's name as Clang spells it (tools/checks/lexdiff.py compares on it).
std::string_view name(Kind kind);

struct Token {
    Kind kind { Kind::unknown };
    std::uint32_t begin { 0 };     // byte offsets into the text, [begin, end)
    std::uint32_t end { 0 };
    std::uint32_t line { 1 };      // 1-based, of the token's first byte
    std::uint32_t column { 1 };    // 1-based, in bytes
    bool start_of_line { false };  // the first token on its line (whitespace and comments aside)
    bool leading_space { false };  // whitespace or a comment comes right before it
};

struct Options {
    bool whitespace { false };     // keep whitespace tokens
    bool comments { false };       // keep comments
};

std::vector<Token> lex(std::string_view text, Options options = {});

// A token's text with its line splices taken out: what it means rather than how it is written.
std::string spelling(std::string_view text, const Token& token);

// Whether an identifier token has a character that only the mathematical notation profile (UAX #31's
// ID_Compat_Math_Start and _Continue) lets into an identifier, written as UTF-8 or as a universal
// character name: `∇f`, `x²`, `²`. The lexer takes them in every standard, as Clang does (as an
// extension); C++29 takes them (P3658R1), and that is what a front end that gates it asks about.
bool uses_mathematical_notation(std::string_view text, const Token& token);

} // namespace mcxx::frontend
