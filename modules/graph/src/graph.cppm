// Module facts without a compiler: what a C++ source provides and imports, read lexically, and the
// module graph of a program built from those scans.
//
// The scan does not preprocess (a module declaration and its imports must not come from a macro,
// [cpp.pre]); it skips comments, string and character literals (raw ones included) and directive
// lines, and reads only what is at namespace scope outside every brace.
export module mcxx.graph;

import std;

export namespace mcxx::graph {

struct Scan {
    std::string module;                 // "m", "m:p", or "" when the file is no module unit
    bool exported { false };            // `export module` (an interface or interface partition)
    bool global_fragment { false };     // begins with `module;`
    std::vector<std::string> imports;   // fully qualified: `import :p;` in module m is "m:p"
    std::vector<std::string> exported_imports;
    // Where each import names its module: byte offsets [begin, end) into the text, parallel to `imports`.
    std::vector<std::pair<std::size_t, std::size_t>> import_spans;
    std::pair<std::size_t, std::size_t> module_span { 0, 0 };   // the declaration's module name

    bool is_module_unit() const { return !module.empty(); }
    bool is_partition() const { return module.find(':') != std::string::npos; }
    // A unit whose compilation produces an interface others import: every interface, and every
    // partition (an implementation partition is imported by its own module's units).
    bool provides_interface() const { return exported || is_partition(); }
    std::string primary() const { return module.substr(0, module.find(':')); }
};

Scan scan(std::string_view text);

class Graph {
public:
    // Adds (or replaces) what `file` is, from its scan.
    void set(const std::string& file, Scan scan);
    void remove(const std::string& file);
    void clear();

    const Scan* scan_of(std::string_view file) const;
    // The file that provides the interface of `module` ("m" or "m:p"), or empty.
    std::string provider(std::string_view module) const;
    // Modules the named module's interface imports, directly.
    std::vector<std::string> requires_of(std::string_view module) const;
    // Every module `roots` need, dependencies first, each once; modules nobody provides are left
    // out and reported in `missing`.
    std::vector<std::string> closure(std::span<const std::string> roots, std::vector<std::string>* missing = nullptr) const;
    std::vector<std::string> modules() const;
    // Modules whose interfaces import one another in a cycle.
    std::vector<std::vector<std::string>> cycles() const;

private:
    std::map<std::string, Scan, std::less<>> files_;
    std::map<std::string, std::string, std::less<>> providers_;   // module -> file
    void rebuild_providers_();
};

} // namespace mcxx::graph

namespace mcxx::graph {

namespace {

bool ident_start(char c) { return std::isalpha(static_cast<unsigned char>(c)) || c == '_'; }
bool ident_char(char c) { return std::isalnum(static_cast<unsigned char>(c)) || c == '_'; }

// Tokens of the scan: identifiers, the punctuation a module declaration uses, and everything else
// as "other". Directive lines, comments and literals produce nothing.
struct Token {
    enum class Kind { ident, punct, header, other, end } kind { Kind::end };
    std::string_view text;
    bool line_start { false };
};

class Lexer {
public:
    explicit Lexer(std::string_view text) : text_ { text } {
        if (text_.starts_with("\xEF\xBB\xBF")) at_ = 3;
    }

    Token next() {
        bool lineStart { at_ == 0 || lineStart_ };
        while (at_ < text_.size()) {
            const char c { text_[at_] };
            if (c == '\n') { ++at_; lineStart = true; continue; }
            if (c == ' ' || c == '\t' || c == '\r' || c == '\f' || c == '\v') { ++at_; continue; }
            if (c == '\\' && at_ + 1 < text_.size() && (text_[at_ + 1] == '\n' || text_[at_ + 1] == '\r')) { at_ += 2; continue; }
            if (c == '/' && peek(1) == '/') { skip_to_eol(); continue; }
            if (c == '/' && peek(1) == '*') {
                const std::size_t end { text_.find("*/", at_ + 2) };
                at_ = end == std::string_view::npos ? text_.size() : end + 2;
                continue;
            }
            if (c == '#' && lineStart) { skip_directive(); lineStart = true; continue; }
            break;
        }
        lineStart_ = false;
        if (at_ >= text_.size()) return { Token::Kind::end, {}, lineStart };
        const char c { text_[at_] };
        const std::size_t begin { at_ };
        if (ident_start(c)) {
            while (at_ < text_.size() && ident_char(text_[at_])) ++at_;
            // A raw string or an encoded literal starts with an identifier-like prefix.
            if (at_ < text_.size() && (text_[at_] == '"' || text_[at_] == '\'')) {
                const std::string_view prefix { text_.substr(begin, at_ - begin) };
                if (prefix == "R" || prefix == "u8R" || prefix == "uR" || prefix == "UR" || prefix == "LR") { skip_raw_string(); return { Token::Kind::other, {}, lineStart }; }
                if (prefix == "u8" || prefix == "u" || prefix == "U" || prefix == "L") { skip_quoted(text_[at_]); return { Token::Kind::other, {}, lineStart }; }
            }
            return { Token::Kind::ident, text_.substr(begin, at_ - begin), lineStart };
        }
        if (c == '"' || c == '\'') {
            // `import "h.h";` names a header unit; keep its spelling.
            const std::size_t start { at_ };
            skip_quoted(c);
            return { c == '"' ? Token::Kind::header : Token::Kind::other, text_.substr(start, at_ - start), lineStart };
        }
        ++at_;
        return { Token::Kind::punct, text_.substr(begin, 1), lineStart };
    }

private:
    std::string_view text_;
    std::size_t at_ { 0 };
    bool lineStart_ { true };

    char peek(std::size_t n) const { return at_ + n < text_.size() ? text_[at_ + n] : '\0'; }
    void skip_to_eol() {
        while (at_ < text_.size() && text_[at_] != '\n') {
            if (text_[at_] == '\\' && at_ + 1 < text_.size() && text_[at_ + 1] == '\n') at_ += 2;
            else ++at_;
        }
    }
    void skip_directive() {
        // A directive runs to the end of its (continued) line; comments inside it end it early.
        while (at_ < text_.size() && text_[at_] != '\n') {
            if (text_[at_] == '\\' && at_ + 1 < text_.size() && (text_[at_ + 1] == '\n' || text_[at_ + 1] == '\r')) { at_ += 2; continue; }
            if (text_[at_] == '/' && peek(1) == '*') {
                const std::size_t end { text_.find("*/", at_ + 2) };
                at_ = end == std::string_view::npos ? text_.size() : end + 2;
                continue;
            }
            ++at_;
        }
    }
    void skip_quoted(char quote) {
        ++at_;
        while (at_ < text_.size() && text_[at_] != quote && text_[at_] != '\n') {
            if (text_[at_] == '\\') ++at_;
            ++at_;
        }
        if (at_ < text_.size() && text_[at_] == quote) ++at_;
    }
    void skip_raw_string() {
        // R"delim( ... )delim"
        ++at_;   // the opening quote
        const std::size_t paren { text_.find('(', at_) };
        if (paren == std::string_view::npos) { at_ = text_.size(); return; }
        const std::string close { ")" + std::string { text_.substr(at_, paren - at_) } + "\"" };
        const std::size_t end { text_.find(close, paren + 1) };
        at_ = end == std::string_view::npos ? text_.size() : end + close.size();
    }
};

// Reads `a.b.c` (and `:p.q` when `partition`) from the lexer; empty on anything else.
std::string read_name(Lexer& lexer, Token& token, const char*& end) {
    std::string name;
    while (token.kind == Token::Kind::ident) {
        name += token.text;
        end = token.text.data() + token.text.size();
        token = lexer.next();
        if (token.kind == Token::Kind::punct && token.text == ".") {
            name += '.';
            token = lexer.next();
            continue;
        }
        break;
    }
    return name;
}

} // namespace

Scan scan(std::string_view text) {
    Scan result;
    Lexer lexer { text };
    int depth { 0 };
    Token token { lexer.next() };
    bool first { true };
    while (token.kind != Token::Kind::end) {
        if (token.kind == Token::Kind::punct) {
            if (token.text == "{") ++depth;
            else if (token.text == "}" && depth > 0) --depth;
            token = lexer.next();
            first = false;
            continue;
        }
        if (depth != 0 || token.kind != Token::Kind::ident) {
            token = lexer.next();
            first = false;
            continue;
        }
        bool exported { false };
        if (token.text == "export") {
            exported = true;
            token = lexer.next();
            if (token.kind != Token::Kind::ident) continue;
        }
        if (token.text == "module") {
            token = lexer.next();
            if (token.kind == Token::Kind::punct && token.text == ";") {   // `module;`
                if (first) result.global_fragment = true;
                token = lexer.next();
                first = false;
                continue;
            }
            if (token.kind == Token::Kind::punct && token.text == ":") {   // `module :private;`
                token = lexer.next();
                continue;
            }
            const char* begin { token.text.data() };
            const char* end { begin };
            std::string name { read_name(lexer, token, end) };
            if (!name.empty() && token.kind == Token::Kind::punct && token.text == ":") {
                token = lexer.next();
                const std::string partition { read_name(lexer, token, end) };
                if (!partition.empty()) name += ":" + partition;
            }
            if (!name.empty() && result.module.empty()) {
                result.module = name;
                result.exported = exported;
                result.module_span = { static_cast<std::size_t>(begin - text.data()), static_cast<std::size_t>(end - text.data()) };
            }
            first = false;
            continue;
        }
        if (token.text == "import") {
            token = lexer.next();
            std::string name;
            const char* begin { token.text.data() };
            const char* end { begin };
            if (token.kind == Token::Kind::punct && token.text == ":") {
                token = lexer.next();
                const std::string partition { read_name(lexer, token, end) };
                if (!partition.empty()) name = result.module.substr(0, result.module.find(':')) + ":" + partition;
            } else if (token.kind == Token::Kind::ident) {
                name = read_name(lexer, token, end);
            } else {
                // `import <h>` / `import "h"`: a header unit, not a named module.
                token = lexer.next();
                continue;
            }
            // `import m [[attributes]];`: MC++'s annotation of an import (mcpp::allow, M1.2) is still the import.
            if (!name.empty() && token.kind == Token::Kind::punct && token.text == "[") {
                int brackets { 0 };
                do {
                    if (token.kind == Token::Kind::punct && token.text == "[") ++brackets;
                    else if (token.kind == Token::Kind::punct && token.text == "]") --brackets;
                    token = lexer.next();
                } while (brackets > 0 && token.kind != Token::Kind::end);
            }
            if (!name.empty() && token.kind == Token::Kind::punct && token.text == ";") {
                if (std::ranges::find(result.imports, name) == result.imports.end()) {
                    result.imports.push_back(name);
                    result.import_spans.emplace_back(static_cast<std::size_t>(begin - text.data()), static_cast<std::size_t>(end - text.data()));
                }
                if (exported && std::ranges::find(result.exported_imports, name) == result.exported_imports.end())
                    result.exported_imports.push_back(name);
            }
            first = false;
            continue;
        }
        token = lexer.next();
        first = false;
    }
    return result;
}

void Graph::set(const std::string& file, Scan s) {
    files_[file] = std::move(s);
    rebuild_providers_();
}

void Graph::remove(const std::string& file) {
    files_.erase(file);
    rebuild_providers_();
}

void Graph::clear() {
    files_.clear();
    providers_.clear();
}

const Scan* Graph::scan_of(std::string_view file) const {
    const auto it = files_.find(file);
    return it == files_.end() ? nullptr : &it->second;
}

std::string Graph::provider(std::string_view module) const {
    const auto it = providers_.find(module);
    return it == providers_.end() ? std::string {} : it->second;
}

std::vector<std::string> Graph::requires_of(std::string_view module) const {
    const std::string file { provider(module) };
    if (file.empty()) return {};
    return files_.find(file)->second.imports;
}

std::vector<std::string> Graph::closure(std::span<const std::string> roots, std::vector<std::string>* missing) const {
    std::vector<std::string> order;
    std::set<std::string, std::less<>> done, visiting;
    std::function<void(const std::string&)> visit = [&](const std::string& module) {
        if (done.contains(module) || visiting.contains(module)) return;
        if (provider(module).empty()) {
            if (missing && std::ranges::find(*missing, module) == missing->end()) missing->push_back(module);
            done.insert(module);
            return;
        }
        visiting.insert(module);
        for (const auto& dependency : requires_of(module)) visit(dependency);
        visiting.erase(module);
        done.insert(module);
        order.push_back(module);
    };
    for (const auto& root : roots) visit(root);
    return order;
}

std::vector<std::string> Graph::modules() const {
    std::vector<std::string> names;
    for (const auto& [module, file] : providers_) names.push_back(module);
    return names;
}

std::vector<std::vector<std::string>> Graph::cycles() const {
    std::vector<std::vector<std::string>> found;
    std::map<std::string, int, std::less<>> state;   // 0 new, 1 on stack, 2 done
    std::vector<std::string> stack;
    std::function<void(const std::string&)> visit = [&](const std::string& module) {
        state[module] = 1;
        stack.push_back(module);
        for (const auto& dependency : requires_of(module)) {
            if (provider(dependency).empty()) continue;
            const int s { state[dependency] };
            if (s == 1) {
                const auto from = std::ranges::find(stack, dependency);
                found.emplace_back(from, stack.end());
            } else if (s == 0) {
                visit(dependency);
            }
        }
        stack.pop_back();
        state[module] = 2;
    };
    for (const auto& [module, file] : providers_)
        if (state[module] == 0) visit(module);
    return found;
}

void Graph::rebuild_providers_() {
    providers_.clear();
    for (const auto& [file, s] : files_) {
        if (!s.provides_interface()) continue;
        // The first file (by path) providing a module wins; a duplicate is the build's problem.
        providers_.try_emplace(s.module, file);
    }
}

} // namespace mcxx::graph
