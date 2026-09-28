// [[mcpp::cfg(predicate)]]: a declaration that exists for some targets only, as Rust's #[cfg].
//
//   [[mcpp::cfg(windows)]] void open_console();                  // family (or os) windows
//   [[mcpp::cfg(target_os = "linux")]] int use_epoll();
//   [[mcpp::cfg(any(unix, target_os = "wasi"))]] ...
//   [[mcpp::cfg(all(target_arch = "x86_64", not(debug_assertions)))]] ...
//   [[mcpp::cfg(feature = "simd")]] ...                          // an active mcpp feature
//
// Keys (the target_ prefix is optional): target_os, target_family, target_arch, target_env,
// target_pointer_width, target_endian; feature. Bare names: windows, unix (families), linux, macos,
// ios, android, freebsd, wasi (systems), debug_assertions (NDEBUG undefined).
//
// It is a source filter: before the file is parsed, a declaration the target does not satisfy is
// blanked -- characters become spaces, line breaks stay -- so the compiler never sees it (it may
// name what only another platform declares) and every position after it is unchanged. When the
// target satisfies it, only the attribute is blanked. Where it applies: declarations and
// statements that end with `;` or a block; `export` before it goes with it. Not on `import`: a
// build's dependency scan reads imports before any compiler plugin runs.
export module mcxx.cfg;

import std;
import mcxx.msa;
import mcxx.plugin;

export namespace mcxx::cfg {

// Whether the target satisfies a predicate (the text inside cfg(...)); an error says what is wrong.
std::expected<bool, std::string> evaluate(std::string_view predicate, const plugin::Target& target);

class Filter final : public plugin::SourceFilter {
public:
    std::string_view name() const override { return "mcxx.cfg"; }
    plugin::Filtered filter(const plugin::SourceContext& context, std::string_view text) const override;
};

} // namespace mcxx::cfg

namespace mcxx::cfg {

namespace {

// ---- the predicate ---------------------------------------------------------------------------

struct Parser {
    std::string_view s;
    std::size_t i { 0 };
    const plugin::Target& target;

    void ws() {
        while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i]))) ++i;
    }
    bool eat(char c) {
        ws();
        if (i < s.size() && s[i] == c) {
            ++i;
            return true;
        }
        return false;
    }
    std::string ident() {
        ws();
        const std::size_t b { i };
        while (i < s.size() && (std::isalnum(static_cast<unsigned char>(s[i])) || s[i] == '_')) ++i;
        return std::string { s.substr(b, i - b) };
    }
    std::expected<std::string, std::string> string() {
        ws();
        if (i >= s.size() || s[i] != '"') return std::unexpected("a string (\"...\") was expected");
        const std::size_t b { ++i };
        while (i < s.size() && s[i] != '"') ++i;
        if (i >= s.size()) return std::unexpected("the string is not closed");
        return std::string { s.substr(b, i++ - b) };
    }

    std::expected<bool, std::string> predicate() {
        const std::string name { ident() };
        if (name.empty()) return std::unexpected(std::format("a predicate was expected at `{}`", s.substr(std::min(i, s.size()))));
        if (name == "all" || name == "any") {
            if (!eat('(')) return std::unexpected(std::format("`{}` takes a list: {}(a, b, ...)", name, name));
            bool value { name == "all" };
            if (eat(')')) return value;
            do {
                auto one = predicate();
                if (!one) return one;
                value = name == "all" ? value && *one : value || *one;
            } while (eat(','));
            if (!eat(')')) return std::unexpected(std::format("`)` was expected to close {}(...)", name));
            return value;
        }
        if (name == "not") {
            if (!eat('(')) return std::unexpected("`not` takes one predicate: not(a)");
            auto one = predicate();
            if (!one) return one;
            if (!eat(')')) return std::unexpected("`not` takes exactly one predicate");
            return !*one;
        }
        if (eat('=')) {
            auto value = string();
            if (!value) return std::unexpected(value.error());
            std::string key { name };
            if (key.starts_with("target_")) key.erase(0, 7);
            if (key == "os") return target.os == *value;
            if (key == "family") return target.family == *value;
            if (key == "arch") return target.arch == *value;
            if (key == "env") return target.env == *value;
            if (key == "pointer_width") return std::to_string(target.pointer_width) == *value;
            if (key == "endian") return target.endian == *value;
            if (key == "feature") return std::ranges::find(target.features, plugin::feature_macro_name(*value)) != target.features.end();
            return std::unexpected(std::format("unknown key `{}` (target_os, target_family, target_arch, target_env, target_pointer_width, target_endian, feature)",
                                               name));
        }
        if (name == "windows" || name == "unix") return target.family == name || target.os == name;
        if (name == "linux" || name == "macos" || name == "ios" || name == "android" || name == "freebsd" || name == "wasi") return target.os == name;
        if (name == "debug_assertions") return target.debug_assertions;
        return std::unexpected(std::format("unknown name `{}` (windows, unix, linux, macos, ios, android, freebsd, wasi, debug_assertions; or key = \"value\")",
                                           name));
    }
};

// ---- the text ------------------------------------------------------------------------------------

bool ident_char(char c) { return std::isalnum(static_cast<unsigned char>(c)) || c == '_'; }

struct Token {
    enum class Kind { end, ident, punct, literal } kind { Kind::end };
    std::size_t begin { 0 };
    std::size_t end { 0 };
};

class Scanner {
public:
    explicit Scanner(std::string_view text) : t_ { text } {}

    // The next token from `at`: comments, whitespace and preprocessor lines skipped; a string or
    // character literal (raw ones too) is one token.
    Token next(std::size_t& at) const {
        skip(at);
        if (at >= t_.size()) return { Token::Kind::end, at, at };
        const std::size_t b { at };
        const char c { t_[at] };
        if (ident_char(c)) {
            while (at < t_.size() && ident_char(t_[at])) ++at;
            const std::string_view word { t_.substr(b, at - b) };
            if (at < t_.size() && (t_[at] == '"' || t_[at] == '\'') && (word == "R" || word == "u8R" || word == "uR" || word == "UR" || word == "LR")) {
                raw(at);
                return { Token::Kind::literal, b, at };
            }
            if (at < t_.size() && (t_[at] == '"' || t_[at] == '\'') && (word == "u8" || word == "u" || word == "U" || word == "L")) {
                quoted(at);
                return { Token::Kind::literal, b, at };
            }
            return { Token::Kind::ident, b, at };
        }
        if (c == '"' || c == '\'') {
            quoted(at);
            return { Token::Kind::literal, b, at };
        }
        // Two-character punctuation that matters here.
        if (at + 1 < t_.size()) {
            const std::string_view two { t_.substr(at, 2) };
            if (two == "::" || two == "[[" || two == "]]" || two == "->") {
                at += 2;
                return { Token::Kind::punct, b, at };
            }
        }
        ++at;
        return { Token::Kind::punct, b, at };
    }

    std::string_view text(const Token& t) const { return t_.substr(t.begin, t.end - t.begin); }
    std::string_view source() const { return t_; }

    void skip(std::size_t& at) const {
        bool line_start { at == 0 || t_[at - 1] == '\n' };
        while (at < t_.size()) {
            const char c { t_[at] };
            if (c == '\n') {
                ++at;
                line_start = true;
            } else if (std::isspace(static_cast<unsigned char>(c))) {
                ++at;
            } else if (c == '/' && at + 1 < t_.size() && t_[at + 1] == '/') {
                while (at < t_.size() && t_[at] != '\n') ++at;
            } else if (c == '/' && at + 1 < t_.size() && t_[at + 1] == '*') {
                const auto e = t_.find("*/", at + 2);
                at = e == std::string_view::npos ? t_.size() : e + 2;
            } else if (c == '#' && line_start) {
                while (at < t_.size() && t_[at] != '\n') {
                    if (t_[at] == '\\' && at + 1 < t_.size() && t_[at + 1] == '\n') ++at;
                    ++at;
                }
            } else {
                return;
            }
        }
    }

private:
    std::string_view t_;

    void quoted(std::size_t& at) const {
        const char q { t_[at++] };
        while (at < t_.size() && t_[at] != q && t_[at] != '\n') {
            if (t_[at] == '\\') ++at;
            ++at;
        }
        if (at < t_.size() && t_[at] == q) ++at;
    }
    void raw(std::size_t& at) const {
        ++at;   // the quote
        const auto open = t_.find('(', at);
        if (open == std::string_view::npos) {
            at = t_.size();
            return;
        }
        const std::string close { ")" + std::string { t_.substr(at, open - at) } + "\"" };
        const auto e = t_.find(close, open + 1);
        at = e == std::string_view::npos ? t_.size() : e + close.size();
    }
};

// The end of the declaration (or statement) that starts at `at`.
std::size_t declaration_end(const Scanner& sc, std::size_t at) {
    std::size_t pos { at };
    Token first { sc.next(pos) };
    // namespace ..., inline namespace, export namespace, export { }, extern "C" { }: a block ends them.
    bool block_ended { false };
    {
        std::size_t p { first.begin };
        Token a { sc.next(p) };
        Token b { sc.next(p) };
        const auto ta = sc.text(a), tb = sc.text(b);
        if (ta == "namespace" || (ta == "inline" && tb == "namespace") || (ta == "export" && (tb == "namespace" || tb == "{"))
            || (ta == "extern" && b.kind == Token::Kind::literal))
            block_ended = true;
    }
    pos = first.begin;
    int round { 0 };
    bool equal { false }, parens { false }, member_init { false };
    Token previous {};
    std::string_view previous_text;
    for (Token t { sc.next(pos) }; t.kind != Token::Kind::end; t = sc.next(pos)) {
        const std::string_view x { sc.text(t) };
        if (x == "(" || x == "[") {
            if (x == "(" && round == 0 && !equal) parens = true;
            ++round;
        } else if (x == ")" || x == "]") {
            if (round > 0) --round;
        } else if (round == 0) {
            if (x == ";") return t.end;
            if (x == "=") equal = true;
            if (x == ":" && parens && !equal) member_init = true;
            if (x == "{") {
                const bool initializer { member_init && (previous.kind == Token::Kind::ident || previous_text == ">") };
                const bool body { block_ended || (parens && !equal && !initializer) };
                // Skip the balanced block.
                int depth { 1 };
                std::size_t p { t.end };
                Token u {};
                for (u = sc.next(p); u.kind != Token::Kind::end && depth > 0; u = sc.next(p)) {
                    const auto y = sc.text(u);
                    if (y == "{") ++depth;
                    else if (y == "}") --depth;
                    if (depth == 0) break;
                }
                if (u.kind == Token::Kind::end) return sc.source().size();
                pos = u.end;
                if (body) return u.end;
                previous = u;
                previous_text = "}";
                continue;
            }
        }
        previous = t;
        previous_text = x;
    }
    return sc.source().size();
}

// Where the attributes and `export` directly before `at` begin: they belong to the declaration.
std::size_t leading_start(std::string_view text, std::size_t at) {
    std::size_t start { at };
    while (true) {
        std::size_t p { start };
        while (p > 0 && std::isspace(static_cast<unsigned char>(text[p - 1]))) --p;
        if (p >= 2 && text.substr(p - 2, 2) == "]]") {
            const auto open = text.rfind("[[", p - 2);
            if (open == std::string_view::npos) return start;
            start = open;
            continue;
        }
        if (p >= 6 && text.substr(p - 6, 6) == "export" && (p == 6 || !ident_char(text[p - 7]))) {
            start = p - 6;
            continue;
        }
        return start;
    }
}

void blank(std::string& text, std::size_t b, std::size_t e) {
    for (std::size_t i { b }; i < e && i < text.size(); ++i)
        if (text[i] != '\n' && text[i] != '\r') text[i] = ' ';
}

msa::Position position_at(std::string_view text, std::size_t offset) {
    msa::Position p;
    for (std::size_t i { 0 }; i < offset && i < text.size(); ++i) {
        if (text[i] == '\n') {
            ++p.line;
            p.column = 0;
        } else {
            ++p.column;
        }
    }
    return p;
}

} // namespace

std::expected<bool, std::string> evaluate(std::string_view predicate, const plugin::Target& target) {
    Parser parser { predicate, 0, target };
    auto value = parser.predicate();
    if (!value) return value;
    parser.ws();
    if (parser.i != predicate.size()) return std::unexpected(std::format("unexpected `{}` after the predicate", predicate.substr(parser.i)));
    return value;
}

plugin::Filtered Filter::filter(const plugin::SourceContext& context, std::string_view text) const {
    plugin::Filtered result;
    if (text.find("mcpp") == std::string_view::npos) return result;   // the common case, at once
    const Scanner sc { text };
    std::string out { text };
    bool changed { false };
    std::size_t pos { 0 };
    for (Token t { sc.next(pos) }; t.kind != Token::Kind::end; t = sc.next(pos)) {
        if (sc.text(t) != "[[") continue;
        // [[ mcpp :: cfg ( ... ) ]]
        std::size_t p { t.end };
        const Token scope { sc.next(p) }, colons { sc.next(p) }, name { sc.next(p) }, open { sc.next(p) };
        if (sc.text(scope) != "mcpp" || sc.text(colons) != "::" || sc.text(name) != "cfg" || sc.text(open) != "(") continue;
        int depth { 1 };
        Token u {};
        for (u = sc.next(p); u.kind != Token::Kind::end; u = sc.next(p)) {
            if (sc.text(u) == "(") ++depth;
            else if (sc.text(u) == ")" && --depth == 0) break;
        }
        const Token close { sc.next(p) };
        if (u.kind == Token::Kind::end || sc.text(close) != "]]") {
            result.problems.push_back({ { position_at(text, t.begin), position_at(text, t.end) }, msa::Severity::error,
                                        "[[mcpp::cfg(...)]] is not closed: `)]]` was expected", "mcpp-cfg", "MC++ cfg", {} });
            continue;
        }
        const std::string_view predicate { text.substr(open.end, u.begin - open.end) };
        const msa::Range where { position_at(text, t.begin), position_at(text, close.end) };
        const auto value = evaluate(predicate, context.target);
        if (!value) {
            result.problems.push_back({ where, msa::Severity::error, std::format("[[mcpp::cfg({})]]: {}", predicate, value.error()), "mcpp-cfg", "MC++ cfg", {} });
            pos = close.end;
            continue;
        }
        if (*value) {
            blank(out, t.begin, close.end);
            pos = close.end;
        } else {
            const std::size_t end { declaration_end(sc, close.end) };
            blank(out, leading_start(text, t.begin), end);
            pos = end;
        }
        changed = true;
    }
    if (changed) result.text = std::move(out);
    return result;
}

plugin::Registration<Filter> registration;

} // namespace mcxx::cfg
