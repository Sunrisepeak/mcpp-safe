// Phase 4 for a module unit: mcxx.frontend:preprocess's definitions.
module mcxx.frontend;

import std;
import mcxx.base;
import :embed;

namespace mcxx::frontend {

namespace {

constexpr std::string_view TRACE { "frontend.preprocess" };

// A token being preprocessed: its hide set ([cpp.rescan], as Prosser's algorithm keeps it), and
// whether it is a placemarker ([cpp.concat]).
struct Tok {
    PpToken t;
    std::uint32_t hide { 0 };
    bool placemarker { false };
    bool final { false };   // not a macro invocation again: what a #embed's parameters read as normal text once made
};

struct Macro {
    bool function_like { false };
    bool variadic { false };
    std::vector<std::string> parameters;   // a variadic macro's last is its variadic one (__VA_ARGS__ or GNU `name...`)
    std::vector<Tok> body;
    bool from_file { false };
};

// Hide sets as ids of sorted vectors of name ids; 0 is the empty set.
class HideSets {
public:
    std::uint32_t with(std::uint32_t set, std::uint32_t name) {
        if (contains(set, name)) return set;
        auto v = sets_[set];
        v.insert(std::ranges::upper_bound(v, name), name);
        return intern(std::move(v));
    }
    bool contains(std::uint32_t set, std::uint32_t name) const { return std::ranges::binary_search(sets_[set], name); }
    std::uint32_t unite(std::uint32_t a, std::uint32_t b) {
        if (a == b || b == 0) return a;
        if (a == 0) return b;
        std::vector<std::uint32_t> v;
        std::ranges::set_union(sets_[a], sets_[b], std::back_inserter(v));
        return intern(std::move(v));
    }
    std::uint32_t intersect(std::uint32_t a, std::uint32_t b) {
        if (a == b) return a;
        if (a == 0 || b == 0) return 0;
        std::vector<std::uint32_t> v;
        std::ranges::set_intersection(sets_[a], sets_[b], std::back_inserter(v));
        return intern(std::move(v));
    }

private:
    std::vector<std::vector<std::uint32_t>> sets_ { {} };
    std::map<std::vector<std::uint32_t>, std::uint32_t> ids_ { { {}, 0 } };
    std::uint32_t intern(std::vector<std::uint32_t> v) {
        const auto [it, added] = ids_.try_emplace(v, static_cast<std::uint32_t>(sets_.size()));
        if (added) sets_.push_back(std::move(v));
        return it->second;
    }
};

// A preprocessor expression's value: [cpp.cond] computes in intmax_t and uintmax_t.
struct Value {
    std::uint64_t bits { 0 };
    bool is_unsigned { false };
    std::int64_t s() const { return static_cast<std::int64_t>(bits); }
    bool truth() const { return bits != 0; }
};

// The names the compiler itself treats as defined in #ifdef and defined(): its builtin macros.
constexpr std::string_view BUILTINS[] {
    "__LINE__", "__FILE__", "__COUNTER__", "__DATE__", "__TIME__", "__TIMESTAMP__", "__INCLUDE_LEVEL__", "__BASE_FILE__", "__FILE_NAME__",
    "__has_include", "__has_include_next", "__has_cpp_attribute", "__has_attribute", "__has_builtin", "__has_feature", "__has_extension",
    "__has_c_attribute", "__has_declspec_attribute", "__is_identifier", "__has_warning", "__has_embed", "__building_module",
    "__is_target_arch", "__is_target_vendor", "__is_target_os", "__is_target_environment", "__is_target_variant_os",
    "__is_target_variant_environment", "__has_constexpr_builtin",
};

// Standard attributes and the values __has_cpp_attribute gives them in C++23 (Clang 23.1).
constexpr std::pair<std::string_view, std::int64_t> STANDARD_ATTRIBUTES[] {
    { "assume", 202207 }, { "carries_dependency", 200809 }, { "deprecated", 201309 }, { "fallthrough", 201603 }, { "likely", 201803 },
    { "maybe_unused", 201603 }, { "no_unique_address", 201803 }, { "nodiscard", 201907 }, { "noreturn", 200809 }, { "unlikely", 201803 },
};

// The names a translation unit does not #define or #undef ([cpp.predefined], [macro.names]; P2843R3): the
// keywords (the alternative tokens too), the identifiers with special meaning (Table 4), the attribute
// tokens (`likely` and `unlikely` may be function-like macros), and the predefined macros and operators the
// standard names. The prefixes `__cpp_` and `__STDCPP_` are the standard's own feature-test and
// implementation-quantity macros.
constexpr std::string_view KEYWORDS[] {
    "alignas", "alignof", "and", "and_eq", "asm", "auto", "bitand", "bitor", "bool", "break", "case", "catch", "char", "char8_t", "char16_t",
    "char32_t", "class", "compl", "concept", "const", "const_cast", "consteval", "constexpr", "constinit", "continue", "contract_assert",
    "co_await", "co_return", "co_yield", "decltype", "default", "delete", "do", "double", "dynamic_cast", "else", "enum", "explicit", "export",
    "extern", "false", "float", "for", "friend", "goto", "if", "inline", "int", "long", "mutable", "namespace", "new", "noexcept", "not",
    "not_eq", "nullptr", "operator", "or", "or_eq", "private", "protected", "public", "register", "reinterpret_cast", "requires", "return",
    "short", "signed", "sizeof", "static", "static_assert", "static_cast", "struct", "switch", "template", "this", "thread_local", "throw",
    "true", "try", "typedef", "typeid", "typename", "union", "unsigned", "using", "virtual", "void", "volatile", "wchar_t", "while", "xor",
    "xor_eq",
};
constexpr std::string_view SPECIAL_IDENTIFIERS[] { "final", "import", "module", "override" };
constexpr std::string_view ATTRIBUTE_TOKENS[] {
    "assume", "carries_dependency", "deprecated", "fallthrough", "indeterminate", "likely", "maybe_unused", "nodiscard", "noreturn",
    "no_unique_address", "unlikely",
};
constexpr std::string_view STANDARD_MACROS[] {
    "__cplusplus", "__DATE__", "__FILE__", "__LINE__", "__TIME__", "__STDC_HOSTED__", "__STDC_EMBED_NOT_FOUND__", "__STDC_EMBED_FOUND__",
    "__STDC_EMBED_EMPTY__", "__STDC_ISO_10646__", "__STDC_MB_MIGHT_NEQ_WC__", "__has_include", "__has_cpp_attribute", "__has_embed",
    "__VA_ARGS__", "__VA_OPT__", "defined",
};

bool is(const Tok& t, Kind kind) { return !t.placemarker && t.t.kind == kind; }
bool is(const Tok& t, std::string_view identifier) { return !t.placemarker && t.t.kind == Kind::raw_identifier && t.t.spelling == identifier; }
bool is_string(Kind k) {
    return k == Kind::string_literal || k == Kind::wide_string_literal || k == Kind::utf8_string_literal || k == Kind::utf16_string_literal ||
           k == Kind::utf32_string_literal;
}
bool is_char(Kind k) {
    return k == Kind::char_constant || k == Kind::wide_char_constant || k == Kind::utf8_char_constant || k == Kind::utf16_char_constant ||
           k == Kind::utf32_char_constant;
}

// Lookups by std::string_view without making a std::string.
struct NameHash {
    using is_transparent = void;
    std::size_t operator()(std::string_view s) const noexcept { return std::hash<std::string_view> {}(s); }
};
template <class T>
using NameMap = std::unordered_map<std::string, T, NameHash, std::equal_to<>>;

// Whether a token's bytes hold a line splice (a backslash, blanks, a line break).
bool has_splice(std::string_view raw) {
    for (std::size_t i { raw.find('\\') }; i != std::string_view::npos; i = raw.find('\\', i + 1)) {
        std::size_t k { i + 1 };
        while (k < raw.size() && (raw[k] == ' ' || raw[k] == '\t' || raw[k] == '\v' || raw[k] == '\f')) ++k;
        if (k < raw.size() && (raw[k] == '\n' || raw[k] == '\r')) return true;
    }
    return false;
}

// A target's predefined macros, made once and shared by every file preprocessed for it.
struct Shared {
    NameMap<Macro> macros;
    std::set<std::string, std::less<>> other_targets;   // predefined for another target, not this one
    std::deque<std::string> storage;                    // the macros' spellings
    bool known { false };
};

class Preprocessor {
public:
    // With `shared` null, it makes the shared table for the options' target (build_shared()).
    Preprocessor(std::string_view text, const PreprocessOptions& options, Preprocessed& out, const Shared* shared)
        : text_ { text }, options_ { options }, out_ { out }, raw_ { lex(text) }, shared_ { shared } {
        starts_.push_back(0);
        for (std::uint32_t i { 0 }; i < text.size(); ++i)
            if (text[i] == '\n' || (text[i] == '\r' && (i + 1 == text.size() || text[i + 1] != '\n'))) starts_.push_back(i + 1);
        if (shared_ == nullptr) predefine_shared();
        else predefine();
    }

    static std::unique_ptr<Shared> build_shared(const std::string& target) {
        PreprocessOptions options;
        options.target = target;
        Preprocessed scratch;
        Preprocessor builder { {}, options, scratch, nullptr };
        auto shared = std::make_unique<Shared>();
        shared->macros = std::move(builder.macros_);
        shared->other_targets = std::move(builder.other_targets_);
        shared->storage = std::move(scratch.storage);   // a deque: the spellings stay where they are
        shared->known = builder.known_;
        return shared;
    }

    void run() {
        Source file { {}, true };
        // `module;` first opens the global module fragment.
        if (!raw_.empty() && is_module_line(0) && raw_.size() > 1 && raw_[1].kind == Kind::semi) {
            out_.module.global_module_fragment = true;
            in_fragment_ = true;
        }
        while (auto t = next(file)) {
            if (!t->t.expanded && t->t.start_of_line && is_directive_like(*t)) {
                module_line(file, std::move(*t));
                continue;
            }
            if (t->t.kind == Kind::raw_identifier && expand(file, *t)) continue;
            emit(*t);
        }
        if (!groups_.empty()) error(groups_.back().at, "unterminated conditional directive");
    }

private:
    struct Source {
        std::deque<Tok> pending;
        bool file;
    };
    struct Group {
        bool active;
        bool taken;
        bool seen_else;
        bool parent_active;
        Where at;
        std::uint32_t skipped_from;   // the line an inactive stretch starts at, 0 when active
    };

    std::string_view text_;
    const PreprocessOptions& options_;
    Preprocessed& out_;
    std::vector<Token> raw_;
    std::vector<std::uint32_t> starts_;
    std::size_t at_ { 0 };
    std::vector<Group> groups_;
    NameMap<Macro> macros_;          // the file's own, and -D's: over the shared table
    const Shared* shared_;           // the target's predefined macros
    std::set<std::string, std::less<>> removed_;   // shared ones the file #undefs
    bool known_ { false };
    NameMap<std::uint32_t> names_;   // name ids for hide sets
    HideSets hide_;
    std::set<std::string, std::less<>> other_targets_;      // predefined for another target, not this one
    NameMap<std::vector<std::optional<Macro>>> pushed_;   // #pragma push_macro
    bool header_seen_ { false };        // an #include was processed: its macros are not known here
    std::size_t last_include_ { 0 };    // the raw token index of the last #include
    NameMap<std::size_t> touched_;   // the raw index of the file's last #define/#undef of a name
    bool in_fragment_ { false };        // between `module;` and the module declaration
    std::uint32_t counter_ { 0 };
    std::uint32_t collecting_ { 0 };    // macro invocations whose arguments are being read (P2843R3: no directive in them)
    std::deque<Tok> injected_;          // what a directive replaced itself with (#embed), read before the file goes on
    Where site_;                        // the invocation being substituted, as written in the file: where # and ## say what they did

    // ---- places and diagnostics ----

    Where where(std::uint32_t begin, std::uint32_t end) const {
        const auto it = std::ranges::upper_bound(starts_, begin);
        const auto line = static_cast<std::uint32_t>(it - starts_.begin());
        return { begin, end, line, begin - *(it - 1) + 1 };
    }
    Where where(const Token& t) const { return { t.begin, t.end, t.line, t.column }; }
    void diagnose(Diagnostic::Severity s, Where at, std::string message) { out_.diagnostics.push_back({ s, std::move(message), at }); }
    void error(Where at, std::string message) { diagnose(Diagnostic::Severity::error, at, std::move(message)); }
    void uncertain(Where at, std::string why) {
        out_.certain = false;
        diagnose(Diagnostic::Severity::note, at, "not certain: " + std::move(why));
    }

    // ---- the language features of newer standards (:standard) ----

    const Language& language() const { return options_.language; }

    void feature_diagnostic(Diagnostic::Severity s, Where at, std::string message, const LanguageFeature& f) {
        base::trace::debug(TRACE, "{}:{} {} ({}): {}", at.line, at.column, f.id, f.paper, message);
        out_.diagnostics.push_back({ s, std::move(message), at, std::string { f.id }, std::string { f.paper } });
    }

    // The use of `what`, which a feature of a newer standard gives. True when the feature is on (at level warn
    // it says so, in a warning). When it is not it is a gate diagnostic that names the feature and its paper,
    // an error as MC1's `deny` is, and the caller goes on as if it were on, to read the rest of the file.
    bool use(const LanguageFeature& f, Where at, std::string_view what) {
        if (language().on(f)) {
            if (language().level(f.id) == FeatureLevel::warn)
                feature_diagnostic(Diagnostic::Severity::warning, at, std::format("{} is C++{} ({}) [{}]", what, f.year, f.paper, f.id), f);
            return true;
        }
        const std::string why { language().has(f) ? std::format("which is not enabled in this file; to enable it, set \"{}\" = \"allow\"", f.id)
                                                   : std::format("and this file is read as C++{}", language().standard.year) };
        feature_diagnostic(Diagnostic::Severity::error, at, std::format("{} is C++{} ({}), {} [{}]", what, f.year, f.paper, why, f.id), f);
        return false;
    }

    // What a feature that restricts the program forbids, when it is on: an error (a warning at level warn).
    // Nothing when it is not: the program is read as the standard of the file always read it.
    void forbid(const LanguageFeature& f, Where at, std::string message) {
        if (!language().on(f)) return;
        feature_diagnostic(language().level(f.id) == FeatureLevel::warn ? Diagnostic::Severity::warning : Diagnostic::Severity::error, at,
                           std::format("{} [{}]", message, f.id), f);
    }

    // ---- tokens ----

    std::string_view store(std::string s) { return out_.storage.emplace_back(std::move(s)); }

    Tok from_file(const Token& r) {
        Tok t;
        t.t.kind = r.kind;
        t.t.at = where(r);
        t.t.start_of_line = r.start_of_line;
        t.t.leading_space = r.leading_space;
        const std::string_view raw { text_.substr(r.begin, r.end - r.begin) };
        t.t.spelling = has_splice(raw) ? store(spelling(text_, r)) : raw;
        return t;
    }

    std::uint32_t name_id(std::string_view name) {
        if (const auto it = names_.find(name); it != names_.end()) return it->second;
        const auto id = static_cast<std::uint32_t>(names_.size() + 1);
        names_.emplace(std::string { name }, id);
        return id;
    }

    void emit(const Tok& t) {
        if (t.placemarker) return;
        out_.tokens.push_back(t.t);
    }

    // ---- the file, directives processed ----

    bool active() const { return groups_.empty() || groups_.back().active; }

    std::optional<Tok> file_next() {
        while (at_ < raw_.size() || !injected_.empty()) {
            if (!injected_.empty()) {
                Tok t { std::move(injected_.front()) };
                injected_.pop_front();
                return t;
            }
            const Token& r = raw_[at_];
            if (r.start_of_line && r.kind == Kind::hash) {
                directive();
                continue;
            }
            ++at_;
            if (active()) {
                check_identifier(r);
                return from_file(r);
            }
        }
        return std::nullopt;
    }

    // P3658R1: an identifier with a character only the mathematical notation profile gives is lexed as one in
    // every mode (as Clang does), and is a use of the feature that lets it in.
    void check_identifier(const Token& r) {
        if (r.kind != Kind::raw_identifier) return;
        const std::string_view raw { text_.substr(r.begin, r.end - r.begin) };
        if (std::ranges::all_of(raw, [](char c) { return static_cast<unsigned char>(c) < 0x80 && c != '\\'; })) return;
        if (uses_mathematical_notation(text_, r)) use(UNICODE_IDENTIFIERS, where(r), std::format("the identifier `{}`", spelling(text_, r)));
    }

    std::optional<Tok> next(Source& s) {
        if (!s.pending.empty()) {
            Tok t { std::move(s.pending.front()) };
            s.pending.pop_front();
            return t;
        }
        if (s.file) return file_next();
        return std::nullopt;
    }

    // ---- predefined macros ----

    void predefine_shared() {
        const predefined::Table* mine { nullptr };
        for (const auto& table : predefined::TABLES)
            if (table.target == options_.target) mine = &table;
        known_ = mine != nullptr;
        for (const auto& table : predefined::TABLES) {
            for (const auto& d : table.definitions) {
                if (&table == mine) define_text(std::format("{}{} {}", d.name, d.parameters, d.replacement), false);
                else other_targets_.emplace(d.name);
            }
        }
        if (mine != nullptr)
            for (const auto& d : mine->definitions) other_targets_.erase(std::string { d.name });
    }

    void predefine() {
        if (!shared_->known) {
            error({}, std::format("no predefined macros for target `{}`", options_.target));
            out_.certain = false;
        }
        // The shared table is C++23's: the standard of the file and its feature-test macros on top.
        if (language().standard.year != 23) define_text(std::format("__cplusplus {}", cplusplus_value(language().standard)), false);
        if (language().on(EMBED)) define_text("__cpp_pp_embed 202502L", false);
        for (const auto& d : options_.defines) {
            const auto eq = d.find('=');
            define_text(eq == std::string::npos ? d + " 1" : d.substr(0, eq) + " " + d.substr(eq + 1), false);
        }
        for (const auto& u : options_.undefines) undefine(u);
    }

    // The macro `name` is here: the file's own, else the target's unless the file #undef'd it.
    const Macro* find_macro(std::string_view name) const {
        if (const auto it = macros_.find(name); it != macros_.end()) return &it->second;
        if (shared_ == nullptr || removed_.contains(name)) return nullptr;
        const auto it = shared_->macros.find(name);
        return it == shared_->macros.end() ? nullptr : &it->second;
    }
    bool has_macro(std::string_view name) const { return find_macro(name) != nullptr; }
    void undefine(std::string_view name) {
        if (const auto it = macros_.find(name); it != macros_.end()) macros_.erase(it);
        if (shared_ != nullptr && shared_->macros.contains(name)) removed_.emplace(name);
    }
    bool other_target(std::string_view name) const { return shared_ != nullptr ? shared_->other_targets.contains(name) : other_targets_.contains(name); }

    // A definition written as a #define's text after the directive name ("NAME(a) body"), not from the file.
    void define_text(std::string text, bool from_file) {
        const std::string_view held { store(std::move(text)) };
        const auto tokens = lex(held);
        std::vector<Tok> line;
        for (const auto& r : tokens) {
            Tok t;
            t.t.kind = r.kind;
            t.t.spelling = held.substr(r.begin, r.end - r.begin);
            t.t.leading_space = r.leading_space;
            line.push_back(t);
        }
        if (!line.empty()) define(line, from_file, {});
    }

    // ---- directives ----

    std::string_view spelled(const Token& t) const { return text_.substr(t.begin, t.end - t.begin); }

    void directive() {
        const Token& hash = raw_[at_++];
        std::size_t end { at_ };
        while (end < raw_.size() && !raw_[end].start_of_line) ++end;
        const std::span<const Token> line { raw_.data() + at_, end - at_ };
        const std::size_t first { at_ };
        at_ = end;
        if (line.empty()) return;   // the null directive
        const Where at { hash.begin, line.back().end, hash.line, hash.column };
        const std::string_view name { line[0].kind == Kind::raw_identifier ? spelled(line[0]) : std::string_view {} };
        const auto rest = line.subspan(1);

        // [cpp.replace.general]/13 (P2843R3): what would act as a directive in the arguments of a macro
        // invocation makes the program ill-formed.
        if (collecting_ > 0 && (groups_.empty() || groups_.back().parent_active))
            forbid(PREPROCESSING_NEVER_UNDEFINED, at, std::format("`#{}` is a directive in the arguments of a macro invocation", name.empty() ? spelled(line[0]) : name));
        if (active())
            for (const auto& t : line) check_identifier(t);

        if (name == "if" || name == "ifdef" || name == "ifndef") {
            const bool parent { active() };
            bool value { false };
            if (parent) value = name == "if" ? condition(rest, at) : defined_directive(rest, at, name == "ifndef");
            groups_.push_back({ parent && value, value, false, parent, at, parent && !value ? at.line + 1 : 0 });
            return;
        }
        if (name == "elif" || name == "elifdef" || name == "elifndef" || name == "else") {
            if (groups_.empty()) return error(at, std::format("#{} without #if", name));
            Group& g = groups_.back();
            if (g.seen_else) return error(at, std::format("#{} after #else", name));
            if (g.active) g.skipped_from = at.line + 1;
            bool now { false };
            if (g.parent_active && !g.taken) {
                if (name == "else") now = true;
                else if (name == "elif") now = condition(rest, at);
                else now = defined_directive(rest, at, name == "elifndef");
            }
            if (name == "else") g.seen_else = true;
            if (now) close_skipped(g, at.line - 1);
            else if (g.parent_active && g.skipped_from == 0) g.skipped_from = at.line + 1;
            g.active = now;
            g.taken = g.taken || now;
            return;
        }
        if (name == "endif") {
            if (groups_.empty()) return error(at, "#endif without #if");
            close_skipped(groups_.back(), at.line - 1);
            groups_.pop_back();
            return;
        }
        if (!active()) return;

        // `# 12 "file" 1`: a line marker (GNU), what -E and the preprocessed files have; where the line goes is not modeled.
        if (line[0].kind == Kind::numeric_constant) return uncertain(at, "a line marker is outside what MC++'s preprocessor covers");

        if (name == "define") return define_directive(rest, first + 1, at);
        if (name == "undef") {
            if (rest.empty() || rest[0].kind != Kind::raw_identifier) return error(at, "macro name missing");
            const std::string n { spelled(rest[0]) };
            check_macro_name(n, "#undef", where(rest[0]), false);
            undefine(n);
            touched_[n] = first;
            return;
        }
        if (name == "include") return include_directive(rest, at, line[0]);
        if (name == "embed") return embed_directive(rest, at);
        if (name == "line") return line_directive(rest, at);
        if (name == "error" || name == "warning") {
            const auto from = rest.empty() ? line[0].end : rest.front().begin;
            const std::string message { text_.substr(from, line.back().end - from) };
            return diagnose(name == "error" ? Diagnostic::Severity::error : Diagnostic::Severity::warning, at, std::format("#{} {}", name, message));
        }
        if (name == "pragma") return pragma(rest, at);
        if (name == "include_next" || name == "import" || name == "ident" || name == "sccs" || name == "assert" || name == "unassert")
            return uncertain(at, std::format("#{} is outside what MC++'s preprocessor covers", name));
        error(at, std::format("invalid preprocessing directive #{}", spelled(line[0])));
    }

    void close_skipped(Group& g, std::uint32_t last) {
        if (g.skipped_from != 0 && g.parent_active && last >= g.skipped_from) out_.skipped.emplace_back(g.skipped_from, last);
        g.skipped_from = 0;
    }

    bool defined_directive(std::span<const Token> rest, Where at, bool negate) {
        if (rest.empty() || rest[0].kind != Kind::raw_identifier) {
            error(at, "macro name missing");
            return false;
        }
        const bool d { is_defined(spelled(rest[0]), where(rest[0]), at_) };
        return negate ? !d : d;
    }

    // Whether `name` is a macro here; when it is not and a header included before might define it,
    // the answer is not certain.
    bool is_defined(std::string_view name, Where at, std::size_t index) {
        if (has_macro(name)) return true;
        if (std::ranges::contains(BUILTINS, name)) return true;
        if (!knowably_absent(name, index)) uncertain(at, std::format("`{}` may be a macro of a header included before", name));
        return false;
    }

    bool knowably_absent(std::string_view name, std::size_t index) const {
        if (!header_seen_ || options_.header_macros_complete) return true;
        if (other_target(name)) return true;
        const auto it = touched_.find(name);
        return it != touched_.end() && it->second > last_include_ && it->second < index;
    }

    void define_directive(std::span<const Token> rest, std::size_t index, Where at) {
        if (rest.empty() || rest[0].kind != Kind::raw_identifier) return error(at, "macro name missing");
        std::vector<Tok> line;
        for (const auto& r : rest) line.push_back(from_file(r));
        const std::string name { spelled(rest[0]) };
        if (name == "defined") return error(where(rest[0]), "`defined` cannot be a macro name");
        // A function-like one may be `likely` or `unlikely` ([macro.names]).
        check_macro_name(name, "#define", where(rest[0]), rest.size() > 1 && rest[1].kind == Kind::l_paren && rest[1].begin == rest[0].end);
        if (define(line, true, where(rest[0]))) {
            out_.macros.push_back({ name, where(rest[0]), find_macro(name)->function_like });
            touched_[name] = index;
        }
    }

    // line: NAME [ ( params ) ] body. False when malformed.
    bool define(const std::vector<Tok>& line, bool from_file, Where at) {
        Macro m;
        m.from_file = from_file;
        std::size_t i { 1 };
        if (i < line.size() && is(line[i], Kind::l_paren) && !line[i].t.leading_space) {
            m.function_like = true;
            ++i;
            bool expect_name { true };
            for (;; ++i) {
                if (i >= line.size()) {
                    error(at, "missing ')' in macro parameter list");
                    return false;
                }
                const Tok& p = line[i];
                if (is(p, Kind::r_paren) && (!expect_name || m.parameters.empty())) break;
                if (expect_name && is(p, Kind::ellipsis)) {
                    m.variadic = true;
                    m.parameters.emplace_back("__VA_ARGS__");
                    if (i + 1 >= line.size() || !is(line[i + 1], Kind::r_paren)) {
                        error(at, "missing ')' after '...'");
                        return false;
                    }
                    ++i;
                    break;
                }
                if (expect_name && p.t.kind == Kind::raw_identifier) {
                    m.parameters.emplace_back(p.t.spelling);
                    if (i + 1 < line.size() && is(line[i + 1], Kind::ellipsis)) {   // GNU: name...
                        m.variadic = true;
                        i += 2;
                        if (i >= line.size() || !is(line[i], Kind::r_paren)) {
                            error(at, "missing ')' after '...'");
                            return false;
                        }
                        break;
                    }
                    expect_name = false;
                    continue;
                }
                if (!expect_name && is(p, Kind::comma)) {
                    expect_name = true;
                    continue;
                }
                if (!expect_name && is(p, Kind::r_paren)) break;
                error(at, "invalid macro parameter list");
                return false;
            }
            ++i;
        }
        for (; i < line.size(); ++i) m.body.push_back(line[i]);
        if (!m.body.empty()) m.body.front().t.leading_space = false;
        // [cpp.stringize]/1: a # is followed by a parameter; [cpp.concat]/1: ## is at neither end.
        for (std::size_t k { 0 }; m.function_like && k < m.body.size(); ++k) {
            if (!is(m.body[k], Kind::hash)) continue;
            if (k + 1 == m.body.size() || (parameter(m, m.body[k + 1]) < 0 && !(m.variadic && is(m.body[k + 1], "__VA_OPT__")))) {
                error(at, "'#' is not followed by a macro parameter");
                return false;
            }
        }
        if (!m.body.empty() && (is(m.body.front(), Kind::hashhash) || is(m.body.back(), Kind::hashhash))) {
            error(at, "'##' cannot be at either end of a macro's replacement");
            return false;
        }
        removed_.erase(std::string { line[0].t.spelling });
        macros_.insert_or_assign(std::string { line[0].t.spelling }, std::move(m));
        return true;
    }

    // [cpp.predefined]/4 and [macro.names]/2 (P2843R3): the names the standard keeps from #define and #undef.
    void check_macro_name(std::string_view name, std::string_view directive, Where at, bool function_like) {
        if (!language().on(PREPROCESSING_NEVER_UNDEFINED)) return;
        std::string_view what;
        if (std::ranges::contains(KEYWORDS, name)) what = "a keyword";
        else if (std::ranges::contains(SPECIAL_IDENTIFIERS, name)) what = "an identifier with special meaning";
        else if (std::ranges::contains(ATTRIBUTE_TOKENS, name)) {
            if ((name == "likely" || name == "unlikely") && function_like) return;
            what = "an attribute-token";
        } else if (std::ranges::contains(STANDARD_MACROS, name) || name.starts_with("__cpp_") || name.starts_with("__STDCPP_")) {
            what = "a predefined macro of the standard";
        }
        if (!what.empty()) forbid(PREPROCESSING_NEVER_UNDEFINED, at, std::format("`{} {}` names {}", directive, name, what));
    }

    // [cpp.line] (P2843R3): the line number is a digit sequence from 1 to 2147483647 and the directive is
    // `# line digit-sequence` or `# line digit-sequence "s-char-sequence"` after macro replacement. Where
    // the line goes is not modeled, so the directive is still not certain.
    void line_directive(std::span<const Token> rest, Where at) {
        if (language().on(PREPROCESSING_NEVER_UNDEFINED)) {
            std::vector<Tok> tokens;
            for (const auto& r : rest) tokens.push_back(from_file(r));
            const auto expanded = expand_all(tokens);
            const auto bad = [&](std::string message) { forbid(PREPROCESSING_NEVER_UNDEFINED, at, "`#line` " + std::move(message)); };
            if (expanded.empty()) {
                bad("takes a digit sequence");
            } else {
                std::string digits;
                for (const char c : expanded[0].t.spelling)
                    if (c != '\'') digits += c;
                const bool sequence { expanded[0].t.kind == Kind::numeric_constant && !digits.empty() &&
                                      std::ranges::all_of(digits, [](char c) { return c >= '0' && c <= '9'; }) };
                if (!sequence) {
                    bad(std::format("takes a digit sequence, not `{}`", expanded[0].t.spelling));
                } else {
                    const auto stripped = digits.find_first_not_of('0');
                    const std::string_view significant { stripped == std::string::npos ? std::string_view {} : std::string_view { digits }.substr(stripped) };
                    if (significant.empty() || significant.size() > 10 || std::stoull(std::string { significant }) > 2147483647ull)
                        bad(std::format("takes a line number from 1 to 2147483647, not {}", digits));
                    if (expanded.size() > 2 || (expanded.size() == 2 && !is(expanded[1], Kind::string_literal)))
                        bad("takes a digit sequence and at most one string literal");
                }
            }
        }
        uncertain(at, "#line is outside what MC++'s preprocessor covers");
    }

    // ---- #embed and __has_embed (P1967R14; P3540R3's offset) ----

    // `<h-char-sequence>` or a string literal at the start of `v`: the name as written, with `next` where the
    // parameters start.
    static std::optional<std::string> resource_name(const std::vector<Tok>& v, std::size_t& next) {
        if (v.empty()) return std::nullopt;
        if (is(v[0], Kind::string_literal)) {
            next = 1;
            return std::string { v[0].t.spelling };
        }
        if (!is(v[0], Kind::less)) return std::nullopt;
        std::string name;
        for (std::size_t k { 1 }; k < v.size(); ++k) {
            if (is(v[k], Kind::greater)) {
                next = k + 1;
                return "<" + name + ">";
            }
            if (k > 1 && v[k].t.leading_space) name += ' ';
            name += v[k].t.spelling;
        }
        return std::nullopt;
    }

    // The resource and the parameters of a #embed's tokens, or of a __has_embed's: the first two forms read the
    // name as written and replace the macros after it, the third replaces them first ([cpp.embed.gen]).
    bool split_embed(const std::vector<Tok>& tokens, std::string& header, std::vector<Tok>& params) {
        const bool named { !tokens.empty() && (is(tokens[0], Kind::less) || is(tokens[0], Kind::string_literal)) };
        const auto all = named ? tokens : expand_all(tokens);
        std::size_t next { 0 };
        const auto name = resource_name(all, next);
        if (!name) return false;
        header = *name;
        params.assign(all.begin() + static_cast<std::ptrdiff_t>(next), all.end());
        if (named) params = expand_all(params);
        return true;
    }

    // What an embed's parameters and its resource come to.
    struct Embedded {
        bool well_formed { true };     // the parameters are, as far as they were read (the errors are given)
        bool supported { true };       // every parameter is one this front end supports (__has_embed's 0 otherwise)
        bool unknown { false };        // the host reads no resources: what the resource says is not known
        bool found { false };
        std::string bytes;
        std::uint64_t offset { 0 };
        std::uint64_t count { 0 };     // the resource-count
        std::optional<std::vector<Tok>> prefix, suffix, if_empty;
    };

    // The value of a limit's or an offset's constant-expression ([cpp.embed.param.limit]): already replaced as
    // normal text, `defined` is not in it, and in a __has_embed neither is __has_include and its kin.
    std::optional<std::uint64_t> embed_value(const std::vector<Tok>& tokens, Where at, bool query) {
        if (tokens.empty()) {
            error(at, "an embed parameter needs a constant expression");
            return std::nullopt;
        }
        for (const auto& t : tokens) {
            if (is(t, "defined")) {
                error(at, "`defined` cannot appear in an embed parameter's constant expression");
                return std::nullopt;
            }
            if (query && t.t.kind == Kind::raw_identifier && (t.t.spelling == "__has_include" || t.t.spelling == "__has_embed" || t.t.spelling == "__has_cpp_attribute")) {
                error(at, std::format("`{}` cannot appear in a __has_embed's embed parameter", t.t.spelling));
                return std::nullopt;
            }
        }
        const auto evaluated = evaluate_operators(tokens, at);
        Expression e { *this, evaluated, at };
        const auto v = e.parse();
        if (!v) return std::nullopt;   // the expression said why
        if (!v->is_unsigned && v->s() < 0) {
            error(at, "the constant expression of an embed parameter cannot be negative");
            return std::nullopt;
        }
        return v->bits;
    }

    Embedded embed_resource(const std::string& header, const std::vector<Tok>& params, Where at, bool query) {
        Embedded e;
        std::vector<PpToken> flat;
        for (const auto& p : params) flat.push_back(p.t);
        const auto parsed = parse_embed_parameters(flat);
        const auto fail = [&](std::string message) {
            e.well_formed = false;
            error(at, std::move(message));
        };
        if (parsed.error_at) {
            fail(parsed.error);
            return e;
        }
        std::optional<std::uint64_t> limit;
        std::set<std::string> seen;
        for (const auto& p : parsed.list) {
            std::string name { p.name };
            if (name == "clang::offset" || name == "gnu::offset") {
                name = "offset";   // what Clang and GCC have as an extension, P3540R3's before it was the standard's
            } else if (name == "offset") {
                use(EMBED_OFFSET, at, "the embed parameter `offset`");
            } else if (name != "limit" && name != "prefix" && name != "suffix" && name != "if_empty") {
                e.supported = false;   // conditionally supported: __has_embed says 0, a #embed is an error
                if (!query) fail(std::format("unknown embed parameter `{}`", p.name));
                continue;
            }
            if (!seen.insert(name).second) {
                fail(std::format("the embed parameter `{}` appears more than once", p.name));
                continue;
            }
            if (!p.parenthesized) {
                fail(std::format("the embed parameter `{}` takes arguments in parentheses", p.name));
                continue;
            }
            const auto first { static_cast<std::size_t>(p.arguments.data() - flat.data()) };
            const std::vector<Tok> arguments { params.begin() + static_cast<std::ptrdiff_t>(first),
                                               params.begin() + static_cast<std::ptrdiff_t>(first + p.arguments.size()) };
            if (name == "limit" || name == "offset") {
                const auto value = embed_value(arguments, at, query);
                if (!value) e.well_formed = false;
                else if (name == "limit") limit = value;
                else e.offset = *value;
            } else if (name == "prefix") {
                e.prefix = arguments;
            } else if (name == "suffix") {
                e.suffix = arguments;
            } else {
                e.if_empty = arguments;
            }
        }
        if (!options_.read_resource) {
            e.unknown = true;
            return e;
        }
        auto bytes = options_.read_resource(header);
        // "name" that is not found is looked for as <name> ([cpp.embed.gen]).
        if (!bytes && header.starts_with('"') && header.size() >= 2) bytes = options_.read_resource("<" + header.substr(1, header.size() - 2) + ">");
        e.found = bytes.has_value();
        if (bytes) {
            e.bytes = std::move(*bytes);
            e.count = embed_count(e.bytes.size(), e.offset, limit);
        }
        return e;
    }

    // `#embed resource parameters` is replaced by the resource's elements as integer literals, between the
    // prefix and the suffix; by the if_empty tokens, or nothing, when it has none.
    void embed_directive(std::span<const Token> rest, Where at) {
        use(EMBED, at, "`#embed`");
        std::vector<Tok> tokens;
        for (const auto& r : rest) tokens.push_back(from_file(r));
        std::string header;
        std::vector<Tok> params;
        if (!split_embed(tokens, header, params)) return error(at, "expected \"FILENAME\" or <FILENAME> after #embed");
        const Embedded e { embed_resource(header, params, at, false) };
        if (!e.well_formed) return;
        if (e.unknown) return uncertain(at, std::format("#embed {}: the resource is not read (the host gives no resources)", header));
        base::trace::debug(TRACE, "{}:{} #embed {} {} ({} bytes, {} elements)", at.line, at.column, header, e.found ? "found" : "not found", e.bytes.size(), e.count);
        out_.embeds.push_back({ header, at, e.found, e.found ? e.count : 0 });
        if (!e.found) return error(at, std::format("{} file not found", header));
        std::vector<Tok> result;
        const auto append = [&](const std::optional<std::vector<Tok>>& tokens) {
            if (tokens) result.insert(result.end(), tokens->begin(), tokens->end());
        };
        if (e.count == 0) {
            append(e.if_empty);
        } else {
            append(e.prefix);
            static const std::array<std::string, 256> DECIMAL { [] {
                std::array<std::string, 256> a;
                for (std::size_t i { 0 }; i < a.size(); ++i) a[i] = std::to_string(i);
                return a;
            }() };
            const auto from { static_cast<std::size_t>(std::min<std::uint64_t>(e.offset, e.bytes.size())) };
            for (std::size_t i { 0 }; i < e.count; ++i) {
                if (i > 0) {
                    Tok comma;
                    comma.t.kind = Kind::comma;
                    comma.t.spelling = ",";
                    result.push_back(comma);
                }
                Tok number;
                number.t.kind = Kind::numeric_constant;
                number.t.spelling = DECIMAL[static_cast<unsigned char>(e.bytes[from + i])];
                number.t.leading_space = i > 0;
                result.push_back(number);
            }
            append(e.suffix);
        }
        // What the directive's tokens read as normal text once made them: not read as macro invocations again.
        for (auto& t : result) {
            t.t.at = at;
            t.t.expanded = true;
            t.t.macro_end = at.end;
            t.t.start_of_line = false;
            t.final = true;
        }
        for (auto& t : result) injected_.push_back(std::move(t));
    }

    // __has_embed ( resource parameters ): 0 when there is no such resource or a parameter is not supported, 1
    // when it is there and has elements under the parameters, 2 when it has none (__STDC_EMBED_*).
    std::int64_t has_embed(const std::vector<Tok>& operand, Where at) {
        use(EMBED, at, "`__has_embed`");
        std::string header;
        std::vector<Tok> params;
        if (!split_embed(operand, header, params)) {
            error(at, "__has_embed expects \"FILENAME\" or <FILENAME>");
            return 0;
        }
        const Embedded e { embed_resource(header, params, at, true) };
        if (!e.well_formed || !e.supported) return 0;
        if (e.unknown) {
            uncertain(at, std::format("__has_embed({}) needs the resources the host has", header));
            return 0;
        }
        return !e.found ? 0 : e.count == 0 ? 2 : 1;
    }

    // [cpp.include]/4 (P2843R3): a directive that is not one of the two forms once its macros are replaced is
    // ill-formed, no diagnostic required; one with more after the header name is one (adjacent string literals are
    // not concatenated: `#include "a" "b"`). Diagnosed when the feature is on, as every compiler does.
    void trailing_include_tokens(Where at) {
        forbid(PREPROCESSING_NEVER_UNDEFINED, at, "`#include` has tokens after the header name; adjacent string literals are not concatenated");
    }

    void include_directive(std::span<const Token> rest, Where at, const Token& keyword) {
        std::string header;
        std::uint32_t end { keyword.end };
        if (!rest.empty() && rest[0].kind == Kind::less) {
            // <header>: the bytes up to the closing '>' as written.
            std::size_t k { 1 };
            while (k < rest.size() && rest[k].kind != Kind::greater) ++k;
            if (k == rest.size()) return error(at, "expected '>' to end the header name");
            header = std::format("<{}>", text_.substr(rest[0].end, rest[k].begin - rest[0].end));
            end = rest[k].end;
            if (k + 1 < rest.size()) trailing_include_tokens(at);
        } else if (!rest.empty() && rest[0].kind == Kind::string_literal) {
            header = std::string { spelled(rest[0]) };
            end = rest[0].end;
            if (rest.size() > 1) trailing_include_tokens(at);
        } else {
            return uncertain(at, "a computed #include");
        }
        const std::uint32_t line_start { starts_[at.line - 1] };
        out_.includes.push_back({ header, { line_start, end, at.line, 1 }, in_fragment_ });
        // The headers' macros, when the host has them, are there from the first #include on.
        if (!header_seen_)
            for (const auto& definition : options_.header_macros) {
                const std::string_view l { definition };
                if (l.starts_with("#define ")) define_text(std::string { l.substr(8) }, false);
            }
        header_seen_ = true;
        last_include_ = at_;
        if (out_.module.present && !in_fragment_) uncertain(at, std::format("#include {} in the module's purview: its text is not read", header));
    }

    void pragma(std::span<const Token> rest, Where at) {
        if (rest.empty() || rest[0].kind != Kind::raw_identifier) return;
        const std::string_view what { spelled(rest[0]) };
        if ((what == "push_macro" || what == "pop_macro") && rest.size() >= 4 && rest[1].kind == Kind::l_paren && rest[2].kind == Kind::string_literal) {
            const std::string_view quoted { spelled(rest[2]) };
            const std::string name { quoted.substr(1, quoted.size() - 2) };
            if (what == "push_macro") {
                const Macro* m { find_macro(name) };
                pushed_[name].push_back(m == nullptr ? std::nullopt : std::optional<Macro> { *m });
            } else if (auto& stack = pushed_[name]; !stack.empty()) {
                if (stack.back()) {
                    removed_.erase(name);
                    macros_.insert_or_assign(name, *stack.back());
                } else {
                    undefine(name);
                }
                stack.pop_back();
            }
            return;
        }
        (void)at;   // #pragma once, GCC diagnostic, ...: nothing that changes the tokens
    }

    // ---- module declarations and imports (pp-module, pp-import) ----

    bool is_module_line(std::size_t i) const {
        if (i >= raw_.size() || raw_[i].kind != Kind::raw_identifier || spelled(raw_[i]) != "module") return false;
        return i + 1 < raw_.size() && !raw_[i + 1].start_of_line &&
               (raw_[i + 1].kind == Kind::semi || raw_[i + 1].kind == Kind::raw_identifier || raw_[i + 1].kind == Kind::colon);
    }

    bool is_directive_like(const Tok& t) {
        if (t.t.kind != Kind::raw_identifier) return false;
        const std::string_view s { t.t.spelling };
        return s == "module" || s == "import" || s == "export";
    }

    // A line that starts with `module`, `import` or `export`: a module declaration or an import when
    // what follows makes it one, and ordinary text otherwise.
    void module_line(Source& file, Tok first) {
        std::vector<Tok> line { std::move(first) };
        // Reads the next token onto the line: false at the end of the file.
        const auto read = [&] {
            auto t = next(file);
            if (!t) return false;
            line.push_back(std::move(*t));
            return true;
        };
        bool exported { false };
        if (is(line[0], "export")) {
            exported = true;
            if (!read() || line[1].t.start_of_line || !(is(line[1], "module") || is(line[1], "import"))) return plain(file, std::move(line));
        }
        const bool module { is(line.back(), "module") };
        if (!read() || line.back().t.start_of_line) return plain(file, std::move(line));
        const Tok after { line.back() };
        const bool header { !module && (is(after, Kind::less) || is_string(after.t.kind)) };
        if (!(is(after, Kind::semi) || after.t.kind == Kind::raw_identifier || is(after, Kind::colon) || header)) return plain(file, std::move(line));
        std::string name;
        std::string partition;
        bool in_partition { false };
        if (header && is(after, Kind::less)) {
            // import <header>; -- the header name is the bytes up to '>'.
            while (read() && !is(line.back(), Kind::greater) && !is(line.back(), Kind::semi)) {
            }
            if (!is(line.back(), Kind::greater)) return plain(file, std::move(line));
            name = std::format("<{}>", text_.substr(after.t.at.end, line.back().t.at.begin - after.t.at.end));
            read();
        } else if (header) {
            name = std::string { after.t.spelling };
            read();
        } else {
            // module-name [: partition] [attributes] ;
            bool attributes { false };
            for (std::size_t k { line.size() - 1 }; !is(line[k], Kind::semi);) {
                if (is(line[k], Kind::l_square)) attributes = true;
                else if (!attributes && is(line[k], Kind::colon)) in_partition = true;
                else if (!attributes) (in_partition ? partition : name) += line[k].t.spelling;
                if (!read()) break;
                k = line.size() - 1;
            }
        }
        const Where at { line.front().t.at.begin, line.back().t.at.end, line.front().t.at.line, line.front().t.at.column };
        if (module) {
            if (name.empty() && partition == "private") {
                out_.module.private_fragment = true;
            } else if (!name.empty()) {
                out_.module.present = true;
                out_.module.name = name;
                out_.module.partition = partition;
                out_.module.exported = exported;
                out_.module.at = at;
                in_fragment_ = false;
            }
            // `module;` opens the global module fragment only first in the file (run()).
        } else {
            out_.imports.push_back({ in_partition ? name + ":" + partition : name, at, exported });
        }
        for (const auto& t : line) emit(t);
    }

    // Tokens read as a possible module line that are ordinary text: back to the stream, the first
    // (which is not a macro invocation: `module`, `import` and `export` are not macro names in practice)
    // out as is.
    void plain(Source& file, std::vector<Tok> line) {
        emit(line.front());
        for (std::size_t i { line.size() }; i-- > 1;) file.pending.push_front(std::move(line[i]));
    }

    // ---- macro expansion ----

    static int parameter(const Macro& m, const Tok& t) {
        if (t.placemarker || t.t.kind != Kind::raw_identifier) return -1;
        for (std::size_t i { 0 }; i < m.parameters.size(); ++i)
            if (m.parameters[i] == t.t.spelling) return static_cast<int>(i);
        return -1;
    }

    // Expands `t` when it names a macro that is not hidden for it, onto the front of `s`: true then.
    bool expand(Source& s, const Tok& t) {
        if (t.final) return false;
        const std::string_view name { t.t.spelling };
        if (name.starts_with("__") || name == "_Pragma")
            if (builtin(s, t)) return true;
        const Macro* found { find_macro(name) };
        if (found == nullptr) return false;
        const std::uint32_t id { name_id(name) };
        if (hide_.contains(t.hide, id)) return false;
        const Macro& m = *found;
        Where site { t.t.at };
        std::uint32_t hs { hide_.with(t.hide, id) };
        std::vector<std::vector<Tok>> args;
        if (m.function_like) {
            auto open = next(s);
            if (!open) return false;
            if (!is(*open, Kind::l_paren)) {
                s.pending.push_front(std::move(*open));
                return false;
            }
            std::vector<Tok> consumed { *open };
            std::vector<Tok> current;
            int depth { 1 };
            std::optional<Tok> close;
            const std::size_t fixed { m.variadic ? m.parameters.size() - 1 : m.parameters.size() };
            ++collecting_;
            while (auto a = next(s)) {
                consumed.push_back(*a);
                if (is(*a, Kind::l_paren)) ++depth;
                else if (is(*a, Kind::r_paren) && --depth == 0) {
                    close = *a;
                    break;
                } else if (is(*a, Kind::comma) && depth == 1 && !(m.variadic && args.size() >= fixed)) {
                    args.push_back(std::move(current));
                    current.clear();
                    continue;
                }
                current.push_back(std::move(*a));
            }
            --collecting_;
            if (!close) {
                error(t.t.at, std::format("unterminated function-like macro invocation of `{}`", name));
                for (std::size_t i { consumed.size() }; i-- > 0;) s.pending.push_front(std::move(consumed[i]));
                return false;
            }
            args.push_back(std::move(current));
            if (m.parameters.empty() && args.size() == 1 && args[0].empty()) args.clear();
            if (m.variadic && args.size() == fixed) args.emplace_back();   // no variadic arguments
            if (args.size() != m.parameters.size()) {
                error(t.t.at, std::format("macro `{}` takes {} argument{}, {} given", name, m.parameters.size(), m.parameters.size() == 1 ? "" : "s", args.size()));
                for (std::size_t i { consumed.size() }; i-- > 0;) s.pending.push_front(std::move(consumed[i]));
                return false;
            }
            hs = hide_.with(hide_.intersect(t.hide, close->hide), id);
            if (!close->t.expanded && !t.t.expanded) site = where(t.t.at.begin, close->t.at.end);
        }
        if (!t.t.expanded) out_.expansions.push_back({ std::string { name }, site });
        const Where outer { site_ };
        site_ = site;
        auto result = substitute(m, args, hs);
        site_ = outer;
        bool first { true };
        const std::uint32_t macro_end { t.t.expanded ? t.t.macro_end : t.t.at.end };
        for (auto& r : result) {
            r.t.at = site;
            r.t.macro_end = macro_end;
            r.t.expanded = true;
            r.t.start_of_line = false;
            if (first) r.t.leading_space = t.t.leading_space;
            first = false;
        }
        for (std::size_t i { result.size() }; i-- > 0;) s.pending.push_front(std::move(result[i]));
        return true;
    }

    // __LINE__ and its kin, and _Pragma: true when `t` was one (its replacement, if any, pushed).
    bool builtin(Source& s, const Tok& t) {
        const std::string_view n { t.t.spelling };
        const auto make = [&](Kind kind, std::string spelling) {
            Tok r { t };
            r.t.kind = kind;
            r.t.spelling = store(std::move(spelling));
            if (!t.t.expanded) r.t.macro_end = t.t.at.end;
            r.t.expanded = true;
            s.pending.push_front(std::move(r));
            return true;
        };
        if (has_macro(n)) return false;
        // In a macro's expansion, the line the outermost invocation ends on (as GCC, and Clang after it).
        if (n == "__LINE__") return make(Kind::numeric_constant, std::to_string(t.t.expanded ? where(t.t.at.end - 1, t.t.at.end).line : t.t.at.line));
        if (n == "__FILE__" || n == "__BASE_FILE__") return make(Kind::string_literal, quote(options_.file));
        if (n == "__FILE_NAME__") return make(Kind::string_literal, quote(std::filesystem::path { options_.file }.filename().string()));
        if (n == "__COUNTER__") return make(Kind::numeric_constant, std::to_string(counter_++));
        if (n == "__INCLUDE_LEVEL__") return make(Kind::numeric_constant, "0");
        if (n == "_Pragma") {
            // _Pragma ( string-literal ): a #pragma, which changes no token.
            auto open = next(s);
            if (!open || !is(*open, Kind::l_paren)) {
                if (open) s.pending.push_front(std::move(*open));
                error(t.t.at, "_Pragma takes a parenthesized string literal");
                return false;
            }
            int depth { 1 };
            while (auto a = next(s)) {
                if (is(*a, Kind::l_paren)) ++depth;
                if (is(*a, Kind::r_paren) && --depth == 0) break;
            }
            return true;
        }
        return false;
    }

    static std::string quote(std::string_view s) {
        std::string out { "\"" };
        for (const char c : s) {
            if (c == '"' || c == '\\') out += '\\';
            out += c;
        }
        return out + "\"";
    }

    // The fully expanded form of `tokens` on their own ([cpp.subst]/1: an argument before substitution).
    std::vector<Tok> expand_all(const std::vector<Tok>& tokens) {
        Source s { { tokens.begin(), tokens.end() }, false };
        std::vector<Tok> out;
        while (auto t = next(s)) {
            if (t->t.kind == Kind::raw_identifier && !t->placemarker && expand(s, *t)) continue;
            out.push_back(std::move(*t));
        }
        return out;
    }

    // [cpp.subst], [cpp.stringize], [cpp.concat]: the replacement list with the arguments in it.
    std::vector<Tok> substitute(const Macro& m, const std::vector<std::vector<Tok>>& args, std::uint32_t hs) {
        std::vector<std::optional<std::vector<Tok>>> expanded(args.size());
        const auto pre = [&](std::size_t i) -> const std::vector<Tok>& {
            if (!expanded[i]) expanded[i] = expand_all(args[i]);
            return *expanded[i];
        };
        const bool va_opt { m.variadic && !pre(args.size() - 1).empty() };
        std::vector<Tok> out;
        replace(m, std::span<const Tok> { m.body }, args, pre, va_opt, out);
        std::vector<Tok> result;
        result.reserve(out.size());
        for (auto& t : out) {
            if (t.placemarker) continue;
            t.hide = hide_.unite(t.hide, hs);
            result.push_back(std::move(t));
        }
        return result;
    }

    template <class Pre>
    void replace(const Macro& m, std::span<const Tok> body, const std::vector<std::vector<Tok>>& args, Pre& pre, bool va_opt, std::vector<Tok>& out) {
        const auto is_paste = [](const Tok& t) { return is(t, Kind::hashhash); };
        const auto placemarker = [] {
            Tok p;
            p.placemarker = true;
            return p;
        };
        // The __VA_OPT__ ( ... ) at `i`: its content's end (the ')'), or `i` when it is not one.
        const auto va_opt_end = [&](std::size_t i) -> std::size_t {
            if (!m.variadic || !is(body[i], "__VA_OPT__") || i + 1 >= body.size() || !is(body[i + 1], Kind::l_paren)) return i;
            int depth { 0 };
            for (std::size_t k { i + 1 }; k < body.size(); ++k) {
                if (is(body[k], Kind::l_paren)) ++depth;
                if (is(body[k], Kind::r_paren) && --depth == 0) return k;
            }
            return i;
        };
        // Its replacement: the content replaced as a replacement list is, placemarkers kept (a ## next
        // to it pastes with its first or last token, which may be one), or one placemarker.
        const auto va_opt_result = [&](std::size_t i, std::size_t close) {
            std::vector<Tok> r;
            if (va_opt) replace(m, body.subspan(i + 2, close - i - 2), args, pre, va_opt, r);
            if (r.empty()) r.push_back(placemarker());
            return r;
        };
        for (std::size_t i { 0 }; i < body.size(); ++i) {
            const Tok& b = body[i];
            const bool paste_next { i + 1 < body.size() && is_paste(body[i + 1]) };
            // # parameter, # __VA_OPT__(...)
            if (m.function_like && is(b, Kind::hash) && i + 1 < body.size()) {
                if (const int p { parameter(m, body[i + 1]) }; p >= 0) {
                    out.push_back(stringize(args[static_cast<std::size_t>(p)], b));
                    ++i;
                    continue;
                }
                if (const std::size_t close { va_opt_end(i + 1) }; close != i + 1) {
                    auto r = va_opt_result(i + 1, close);
                    std::erase_if(r, [](const Tok& t) { return t.placemarker; });
                    out.push_back(stringize(r, b));
                    i = close;
                    continue;
                }
            }
            // ## operand
            if (is_paste(b) && i + 1 < body.size() && !out.empty()) {
                const Tok& r = body[i + 1];
                std::vector<Tok> rhs;
                std::size_t skip { i + 1 };
                if (const int p { parameter(m, r) }; p >= 0) {
                    rhs = args[static_cast<std::size_t>(p)];
                    // GNU: `, ## __VA_ARGS__` pastes nothing: without variadic arguments the comma
                    // goes, with them they follow it as written.
                    if (m.variadic && static_cast<std::size_t>(p) == args.size() - 1 && is(out.back(), Kind::comma)) {
                        if (rhs.empty()) out.pop_back();
                        else out.insert(out.end(), rhs.begin(), rhs.end());
                        i = skip;
                        continue;
                    }
                } else if (const std::size_t close { va_opt_end(i + 1) }; close != i + 1) {
                    rhs = va_opt_result(i + 1, close);
                    skip = close;
                } else {
                    rhs = { r };
                }
                // lhs ## placemarker is lhs; placemarker ## rhs is rhs.
                if (!rhs.empty() && !rhs[0].placemarker) {
                    if (out.back().placemarker) out.back() = rhs[0];
                    else out.back() = paste(out.back(), rhs[0]);
                }
                if (!rhs.empty()) out.insert(out.end(), rhs.begin() + 1, rhs.end());
                i = skip;
                continue;
            }
            if (const int p { parameter(m, b) }; p >= 0) {
                const auto& a = paste_next || (i > 0 && is_paste(body[i - 1])) ? args[static_cast<std::size_t>(p)] : pre(static_cast<std::size_t>(p));
                if (a.empty()) {
                    out.push_back(placemarker());
                } else {
                    const std::size_t from { out.size() };
                    out.insert(out.end(), a.begin(), a.end());
                    out[from].t.leading_space = b.t.leading_space;
                }
                continue;
            }
            if (const std::size_t close { va_opt_end(i) }; close != i) {
                const auto r = va_opt_result(i, close);
                const std::size_t from { out.size() };
                out.insert(out.end(), r.begin(), r.end());
                for (std::size_t k { from }; k < out.size(); ++k)
                    if (!out[k].placemarker) {
                        out[k].t.leading_space = b.t.leading_space;
                        break;
                    }
                i = close;
                continue;
            }
            out.push_back(b);
        }
    }

    Tok stringize(const std::vector<Tok>& tokens, const Tok& hash) {
        std::string s { "\"" };
        bool first { true };
        for (const auto& t : tokens) {
            if (t.placemarker) continue;
            if (!first && t.t.leading_space) s += ' ';
            first = false;
            if (is_string(t.t.kind) || is_char(t.t.kind)) {
                for (const char c : t.t.spelling) {
                    // A new-line in a raw string literal becomes `\n` ([cpp.stringize]/2, P2843R3 and CWG1709: what
                    // Clang does).
                    if (c == '\n') {
                        s += "\\n";
                        continue;
                    }
                    if (c == '"' || c == '\\') s += '\\';
                    s += c;
                }
            } else {
                s += t.t.spelling;
            }
        }
        // A final backslash left alone would escape the closing quote: no valid literal. P2843R3 makes it
        // ill-formed; as Clang does, the backslash is ignored (a warning, or the error where the feature is on).
        std::size_t slashes { 0 };
        for (std::size_t i { s.size() }; i > 1 && s[i - 1] == '\\'; --i) ++slashes;
        if (slashes % 2 == 1) {
            s.pop_back();
            if (language().on(PREPROCESSING_NEVER_UNDEFINED)) forbid(PREPROCESSING_NEVER_UNDEFINED, site_, "the # operator forms an invalid string literal: it ends in a backslash");
            else diagnose(Diagnostic::Severity::warning, site_, "invalid string literal, ignoring final '\\'");
        }
        s += '"';
        Tok r { hash };
        r.t.kind = Kind::string_literal;
        r.t.spelling = store(std::move(s));
        return r;
    }

    Tok paste(const Tok& lhs, const Tok& rhs) {
        std::string s { lhs.t.spelling };
        s += rhs.t.spelling;
        const auto held = store(std::move(s));
        const auto tokens = lex(held, { .whitespace = true, .comments = true });
        Tok r { lhs };
        if (tokens.size() == 1 && tokens[0].kind != Kind::comment && tokens[0].kind != Kind::whitespace) {
            r.t.kind = tokens[0].kind;
            r.t.spelling = held;
            return r;
        }
        error(site_, std::format("pasting formed '{}', an invalid preprocessing token", held));
        r.t.spelling = held;
        r.t.kind = Kind::unknown;
        return r;
    }

    // ---- #if ----

    bool condition(std::span<const Token> rest, Where at) {
        if (rest.empty()) {
            error(at, "#if with no expression");
            return false;
        }
        // defined and the __has_ family first, on the tokens as written; then macro expansion.
        std::vector<Tok> tokens;
        for (const auto& r : rest) tokens.push_back(from_file(r));
        const auto evaluated = evaluate_operators(tokens, at);
        const auto expanded = expand_all(evaluated);
        auto final_tokens = evaluate_operators(expanded, at);
        // The alternative tokens are the operators in C++ ([lex.digraph]): `#if a and not b`.
        for (auto& t : final_tokens) {
            if (t.t.kind != Kind::raw_identifier || t.placemarker) continue;
            static constexpr std::pair<std::string_view, Kind> ALTERNATIVES[] {
                { "and", Kind::ampamp }, { "or", Kind::pipepipe }, { "not", Kind::exclaim }, { "bitand", Kind::amp }, { "bitor", Kind::pipe },
                { "xor", Kind::caret }, { "compl", Kind::tilde }, { "not_eq", Kind::exclaimequal },
            };
            for (const auto& [word, kind] : ALTERNATIVES)
                if (t.t.spelling == word) t.t.kind = kind;
        }
        Expression e { *this, final_tokens, at };
        const auto v = e.parse();
        return v && v->truth();
    }

    Tok number(const Tok& like, std::int64_t v) {
        Tok r { like };
        r.t.kind = Kind::numeric_constant;
        r.t.spelling = store(std::to_string(v));
        return r;
    }

    // `defined X`, `defined(X)` and `__has_*( ... )` replaced by their values.
    std::vector<Tok> evaluate_operators(const std::vector<Tok>& in, Where at) {
        std::vector<Tok> out;
        for (std::size_t i { 0 }; i < in.size(); ++i) {
            const Tok& t = in[i];
            if (is(t, "defined")) {
                std::size_t k { i + 1 };
                const bool paren { k < in.size() && is(in[k], Kind::l_paren) };
                if (paren) ++k;
                if (k >= in.size() || in[k].t.kind != Kind::raw_identifier) {
                    error(at, "macro name missing after `defined`");
                    out.push_back(number(t, 0));
                    i = k;
                    continue;
                }
                const bool d { is_defined(in[k].t.spelling, in[k].t.at, at_) };
                if (paren && (k + 1 >= in.size() || !is(in[k + 1], Kind::r_paren))) error(at, "missing ')' after `defined`");
                out.push_back(number(t, d ? 1 : 0));
                i = paren ? k + 1 : k;
                continue;
            }
            if (is(t, "__has_embed") && i + 1 < in.size() && is(in[i + 1], Kind::l_paren) && !has_macro("__has_embed")) {
                std::size_t k { i + 2 };
                int depth { 1 };
                const std::size_t from { k };
                for (; k < in.size(); ++k) {
                    if (is(in[k], Kind::l_paren)) ++depth;
                    if (is(in[k], Kind::r_paren) && --depth == 0) break;
                }
                const std::vector<Tok> operand { in.begin() + static_cast<std::ptrdiff_t>(from), in.begin() + static_cast<std::ptrdiff_t>(std::min(k, in.size())) };
                out.push_back(number(t, has_embed(operand, t.t.at)));
                i = k;
                continue;
            }
            if (t.t.kind == Kind::raw_identifier && t.t.spelling.starts_with("__") && i + 1 < in.size() && is(in[i + 1], Kind::l_paren) &&
                std::ranges::contains(BUILTINS, t.t.spelling) && !has_macro(t.t.spelling)) {
                // The operand, to the matching ')'.
                std::size_t k { i + 2 };
                int depth { 1 };
                const std::size_t from { k };
                for (; k < in.size(); ++k) {
                    if (is(in[k], Kind::l_paren)) ++depth;
                    if (is(in[k], Kind::r_paren) && --depth == 0) break;
                }
                std::string operand;
                for (std::size_t j { from }; j < k && j < in.size(); ++j) operand += in[j].t.spelling;
                out.push_back(number(t, has(t.t.spelling, operand, t.t.at)));
                i = k;
                continue;
            }
            out.push_back(t);
        }
        return out;
    }

    // Whether `name` is a keyword in the standard the file is read as: the C++11 ones are keywords from C++11 on, the
    // C++20 ones (char8_t, concept, consteval, constinit, co_await, co_return, co_yield, requires) from C++20, and
    // contract_assert from C++26.
    bool keyword(std::string_view name) const {
        if (!std::ranges::contains(KEYWORDS, name)) return false;
        const int year { language().standard.year };
        if (name == "contract_assert") return year >= 26;
        if (name == "char8_t" || name == "concept" || name == "consteval" || name == "constinit" || name == "co_await" || name == "co_return" || name == "co_yield" || name == "requires")
            return year >= 20;
        if (name == "alignas" || name == "alignof" || name == "char16_t" || name == "char32_t" || name == "constexpr" || name == "decltype" || name == "noexcept" ||
            name == "nullptr" || name == "static_assert" || name == "thread_local")
            return year >= 11;
        return true;
    }

    std::int64_t has(std::string_view op, std::string_view operand, Where at) {
        if (op == "__has_cpp_attribute") {
            const std::string_view name { operand.starts_with("std::") ? operand.substr(5) : operand };
            for (const auto& [attribute, value] : STANDARD_ATTRIBUTES)
                if (attribute == name) return value;
            if (operand.find("::") == std::string_view::npos) return 0;
        }
        if (op == "__is_identifier") {
            // 0 for a keyword of the standard the file is read as, 1 for any other name; a name the compiler makes
            // a keyword of its own (`__attribute`, `_Bool`) is for the compiler to say.
            if (keyword(operand)) return 0;
            if (operand.starts_with("__") || (operand.size() > 1 && operand[0] == '_' && operand[1] >= 'A' && operand[1] <= 'Z')) uncertain(at, std::format("__is_identifier({}) needs what the compiler knows", operand));
            return 1;
        }
        uncertain(at, std::format("{}({}) needs what the compiler knows", op, operand));
        return 0;
    }

    // [cpp.cond]: an integral constant expression over intmax_t and uintmax_t.
    class Expression {
    public:
        Expression(Preprocessor& pp, const std::vector<Tok>& tokens, Where at) : pp_ { pp }, t_ { tokens }, at_ { at } {}

        std::optional<Value> parse() {
            auto v = comma(true);
            if (v && i_ != t_.size()) return fail("unexpected token in preprocessor expression");
            return v;
        }

    private:
        Preprocessor& pp_;
        const std::vector<Tok>& t_;
        Where at_;
        std::size_t i_ { 0 };

        std::nullopt_t fail(std::string_view message) {
            if (i_ != std::numeric_limits<std::size_t>::max()) pp_.error(at_, std::string { message });
            i_ = std::numeric_limits<std::size_t>::max();
            return std::nullopt;
        }
        bool at(Kind k) const { return i_ < t_.size() && is(t_[i_], k); }

        std::optional<Value> comma(bool live) {
            auto v = conditional(live);
            while (v && at(Kind::comma)) {
                ++i_;
                v = conditional(live);
            }
            return v;
        }

        std::optional<Value> conditional(bool live) {
            auto c = binary(0, live);
            if (!c || !at(Kind::question)) return c;
            ++i_;
            auto a = comma(live && c->truth());
            if (!a || !at(Kind::colon)) return a ? fail("expected ':' in preprocessor expression") : a;
            ++i_;
            auto b = conditional(live && !c->truth());
            if (!b) return b;
            Value r { c->truth() ? *a : *b };
            r.is_unsigned = a->is_unsigned || b->is_unsigned;
            return r;
        }

        static int precedence(Kind k) {
            switch (k) {
            case Kind::pipepipe: return 1;
            case Kind::ampamp: return 2;
            case Kind::pipe: return 3;
            case Kind::caret: return 4;
            case Kind::amp: return 5;
            case Kind::equalequal: case Kind::exclaimequal: return 6;
            case Kind::less: case Kind::greater: case Kind::lessequal: case Kind::greaterequal: return 7;
            case Kind::lessless: case Kind::greatergreater: return 8;
            case Kind::plus: case Kind::minus: return 9;
            case Kind::star: case Kind::slash: case Kind::percent: return 10;
            default: return 0;
            }
        }

        std::optional<Value> binary(int min, bool live) {
            auto lhs = unary(live);
            while (lhs && i_ < t_.size()) {
                const Kind op { t_[i_].placemarker ? Kind::unknown : t_[i_].t.kind };
                const int p { precedence(op) };
                if (p == 0 || p <= min) break;
                ++i_;
                const bool rhs_live { live && !(op == Kind::ampamp && !lhs->truth()) && !(op == Kind::pipepipe && lhs->truth()) };
                auto rhs = binary(p, rhs_live);
                if (!rhs) return rhs;
                lhs = apply(op, *lhs, *rhs, rhs_live);
                if (!lhs) return lhs;
            }
            return lhs;
        }

        std::optional<Value> apply(Kind op, Value a, Value b, bool live) {
            const bool u { a.is_unsigned || b.is_unsigned };
            const auto logical = [](bool v) { return Value { v ? 1u : 0u, false }; };
            switch (op) {
            case Kind::pipepipe: return logical(a.truth() || b.truth());
            case Kind::ampamp: return logical(a.truth() && b.truth());
            case Kind::pipe: return Value { a.bits | b.bits, u };
            case Kind::caret: return Value { a.bits ^ b.bits, u };
            case Kind::amp: return Value { a.bits & b.bits, u };
            case Kind::equalequal: return logical(a.bits == b.bits);
            case Kind::exclaimequal: return logical(a.bits != b.bits);
            case Kind::less: return logical(u ? a.bits < b.bits : a.s() < b.s());
            case Kind::greater: return logical(u ? a.bits > b.bits : a.s() > b.s());
            case Kind::lessequal: return logical(u ? a.bits <= b.bits : a.s() <= b.s());
            case Kind::greaterequal: return logical(u ? a.bits >= b.bits : a.s() >= b.s());
            case Kind::lessless: return Value { b.bits >= 64 ? 0 : a.bits << b.bits, a.is_unsigned };
            case Kind::greatergreater:
                if (b.bits >= 64) return Value { a.is_unsigned || a.s() >= 0 ? 0u : ~0ull, a.is_unsigned };
                return Value { a.is_unsigned ? a.bits >> b.bits : static_cast<std::uint64_t>(a.s() >> b.bits), a.is_unsigned };
            case Kind::plus: return Value { a.bits + b.bits, u };
            case Kind::minus: return Value { a.bits - b.bits, u };
            case Kind::star: return Value { u ? a.bits * b.bits : static_cast<std::uint64_t>(a.s() * b.s()), u };
            case Kind::slash:
            case Kind::percent:
                if (b.bits == 0) {
                    if (!live) return Value { 0, u };
                    return fail(op == Kind::slash ? "division by zero in preprocessor expression" : "remainder by zero in preprocessor expression");
                }
                if (u) return Value { op == Kind::slash ? a.bits / b.bits : a.bits % b.bits, true };
                if (b.s() == -1) return Value { op == Kind::slash ? static_cast<std::uint64_t>(-a.bits) : 0u, false };
                return Value { static_cast<std::uint64_t>(op == Kind::slash ? a.s() / b.s() : a.s() % b.s()), false };
            default: return fail("unexpected operator in preprocessor expression");
            }
        }

        std::optional<Value> unary(bool live) {
            if (i_ >= t_.size()) return fail("expected a value in preprocessor expression");
            const Tok& t = t_[i_];
            if (is(t, Kind::plus) || is(t, Kind::minus) || is(t, Kind::tilde) || is(t, Kind::exclaim)) {
                ++i_;
                auto v = unary(live);
                if (!v) return v;
                if (is(t, Kind::minus)) return Value { 0 - v->bits, v->is_unsigned };
                if (is(t, Kind::tilde)) return Value { ~v->bits, v->is_unsigned };
                if (is(t, Kind::exclaim)) return Value { v->truth() ? 0u : 1u, false };
                return v;
            }
            if (is(t, Kind::l_paren)) {
                ++i_;
                auto v = comma(live);
                if (!v) return v;
                if (!at(Kind::r_paren)) return fail("expected ')' in preprocessor expression");
                ++i_;
                return v;
            }
            ++i_;
            if (t.t.kind == Kind::numeric_constant) return integer(t.t.spelling);
            if (is_char(t.t.kind)) return character(t.t.spelling);
            if (t.t.kind == Kind::raw_identifier) {
                if (t.t.spelling == "true") return Value { 1, false };
                if (t.t.spelling == "false") return Value { 0, false };
                // A name that is no macro is 0 -- unless a header might have made it one.
                if (!pp_.knowably_absent(t.t.spelling, pp_.at_))
                    pp_.uncertain(t.t.at, std::format("`{}` may be a macro of a header included before", t.t.spelling));
                return Value { 0, false };
            }
            --i_;
            return fail("invalid token in preprocessor expression");
        }

        std::optional<Value> integer(std::string_view s) {
            std::string digits;
            for (const char c : s)
                if (c != '\'') digits += c;
            std::size_t k { 0 };
            unsigned base { 10 };
            if (digits.size() > 1 && digits[0] == '0' && (digits[1] == 'x' || digits[1] == 'X')) base = 16, k = 2;
            else if (digits.size() > 1 && digits[0] == '0' && (digits[1] == 'b' || digits[1] == 'B')) base = 2, k = 2;
            else if (digits.size() > 1 && digits[0] == '0') base = 8, k = 1;
            std::uint64_t v { 0 };
            bool overflow { false };
            std::size_t start { k };
            for (; k < digits.size(); ++k) {
                const char c { digits[k] };
                unsigned d { 99 };
                if (c >= '0' && c <= '9') d = static_cast<unsigned>(c - '0');
                else if (c >= 'a' && c <= 'f') d = static_cast<unsigned>(c - 'a' + 10);
                else if (c >= 'A' && c <= 'F') d = static_cast<unsigned>(c - 'A' + 10);
                if (d >= base) break;
                if (v > (std::numeric_limits<std::uint64_t>::max() - d) / base) overflow = true;
                v = v * base + d;
            }
            if (k == start && base != 8) return fail("invalid integer in preprocessor expression");
            const std::string_view suffix { std::string_view { digits }.substr(k) };
            if (suffix.find_first_of(".eEpP") == 0 && base != 16) return fail("floating point literal in preprocessor expression");
            bool is_unsigned { false };
            for (const char c : suffix) {
                if (c == 'u' || c == 'U') is_unsigned = true;
                else if (c != 'l' && c != 'L' && c != 'z' && c != 'Z') return fail("invalid suffix on integer in preprocessor expression");
            }
            if (overflow) pp_.error(at_, "integer literal is too large to be represented in any integer type");
            if (!is_unsigned && v > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())) is_unsigned = true;
            return Value { v, is_unsigned };
        }

        std::optional<Value> character(std::string_view s) {
            const auto open = s.find('\'');
            const std::string_view body { s.substr(open + 1, s.rfind('\'') - open - 1) };
            std::uint64_t v { 0 };
            std::size_t count { 0 };
            for (std::size_t k { 0 }; k < body.size(); ++k, ++count) {
                unsigned char c { static_cast<unsigned char>(body[k]) };
                if (c == '\\' && k + 1 < body.size()) {
                    const char e { body[++k] };
                    switch (e) {
                    case 'n': c = '\n'; break;
                    case 't': c = '\t'; break;
                    case 'r': c = '\r'; break;
                    case '0': c = 0; break;
                    case 'a': c = '\a'; break;
                    case 'b': c = '\b'; break;
                    case 'f': c = '\f'; break;
                    case 'v': c = '\v'; break;
                    default: c = static_cast<unsigned char>(e);
                    }
                }
                v = (v << 8) | c;
            }
            if (count != 1 || open != 0) pp_.uncertain(at_, "a character literal in #if that is not one plain character");
            // A plain char is signed on the targets MC++ builds for.
            if (open == 0 && count == 1 && v >= 0x80) return Value { static_cast<std::uint64_t>(static_cast<std::int64_t>(static_cast<signed char>(v))), false };
            return Value { v, false };
        }
    };
};

} // namespace

Preprocessed preprocess(std::string_view text, const PreprocessOptions& options) {
    // Each target's predefined macros are made once, for every file after.
    static std::mutex mutex;
    static std::map<std::string, std::unique_ptr<Shared>, std::less<>> shared;
    const Shared* table { nullptr };
    {
        std::lock_guard lock { mutex };
        auto& slot = shared[options.target];
        if (!slot) slot = Preprocessor::build_shared(options.target);
        table = slot.get();
    }
    Preprocessed out;
    out.target = options.target;
    Preprocessor pp { text, options, out, table };
    pp.run();
    return out;
}

std::vector<std::string_view> targets() {
    std::vector<std::string_view> out;
    for (const auto& t : predefined::TABLES) out.push_back(t.target);
    return out;
}

} // namespace mcxx::frontend
