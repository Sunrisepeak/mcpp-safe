// The MC++ plugin SDK (MC4 v0). A plugin is an mcpp package that registers rules at static
// initialization; the program it is linked into (mcxx, or mcppls through libmc++) runs them.
//
//   struct JsonRules final : mcxx::plugin::Rule {
//       std::span<const Feature> features() const override { ... }
//       void check(const Context& c, std::vector<Finding>& out) const override { ... }
//   };
//   mcxx::plugin::Registration<JsonRules> registration;   // in a .cpp of the plugin
//
// A rule reads MSA facts (mcxx.msa fact::Facts) and reports findings; whether a finding is an
// error, a warning or nothing is not the rule's to decide but the configuration's (mcxx.features:
// profile, package, module, namespace, and [[mcpp::allow]] on a declaration).
export module mcxx.plugin;

import std;
import mcxx.msa;

export namespace mcxx::plugin {

enum class Level { allow, warn, deny };

std::string_view to_string(Level level);
std::optional<Level> parse_level(std::string_view name);   // "allow" | "warn" | "deny" (also "off", "error")

// One thing MC++ can gate (MC1): a language feature ("goto"), a library ("lib:std.vector"), or a
// library's pitfall ("json-brace-init").
struct Feature {
    std::string id;
    std::string category;       // "language" | "library" | "pitfall"
    std::string layer;          // where it is decided: "syntax" | "decl" | "expr"
    std::string summary;        // one line, for diagnostics and listings
    std::string fix;            // what to write instead
    Level default_level { Level::allow };
    bool waivable { true };     // [[mcpp::allow("id")]] may waive it
    // The level this feature has under a named profile (`profile = "safe"` in a package's
    // [package.metadata.mcxx]): profiles are what plugins say they are.
    std::vector<std::pair<std::string, Level>> profiles;
};

struct Finding {
    std::string feature;
    msa::Range range;
    std::string message;
    std::string container;      // the enclosing namespace, for namespace-scoped levels
};

struct Context {
    std::string_view path;
    std::string_view module;    // "m", "m:p", or "" for a non-module unit
    const msa::fact::Facts& facts;
};

class Rule {
public:
    virtual ~Rule() = default;
    virtual std::string_view name() const = 0;   // the plugin's name, e.g. "mc++.safe"
    virtual std::span<const Feature> features() const = 0;
    virtual void check(const Context& context, std::vector<Finding>& out) const = 0;
};

// The rules linked into this program, in registration order.
void register_rule(std::unique_ptr<Rule> rule);
std::span<const std::unique_ptr<Rule>> rules();
const Feature* find_feature(std::string_view id);

// ---- Source filters: the second extension point ------------------------------------------------
//
// A source filter sees a file's text before it is parsed, with the target it is compiled for, and
// may return a replacement: `[[mcpp::cfg(windows)]]` (plugins/cfg) blanks what the target does not
// have. The replacement keeps the text's length and its line breaks, so every position a
// diagnostic, an index or an editor names stays where it was; a host refuses one that does not.

// The target a file is compiled for, in the words a configuration uses.
struct Target {
    std::string triple;               // "x86_64-unknown-linux-gnu"
    std::string os;                   // "linux", "windows", "macos", "ios", "android", "freebsd", "wasi", "none"
    std::string family;               // "unix", "windows" or ""
    std::string arch;                 // "x86_64", "aarch64", "riscv64", "wasm32", ...
    std::string env;                  // "gnu", "musl", "msvc", "" ...
    unsigned pointer_width { 64 };
    std::string endian;               // "little" | "big"
    std::vector<std::string> features;   // the active mcpp features, as their MCPP_FEATURE_<NAME> names ("SIMD_AVX")
    bool debug_assertions { false };  // NDEBUG is not defined
};

// A feature name as mcpp spells its macro: uppercased, every other character an underscore.
std::string feature_macro_name(std::string_view feature);

struct SourceContext {
    std::string_view path;
    const Target& target;
};

struct Filtered {
    std::optional<std::string> text;       // the replacement, or nothing when the text is unchanged
    std::vector<msa::Diagnostic> problems; // what the filter could not understand: errors in the file
};

class SourceFilter {
public:
    virtual ~SourceFilter() = default;
    virtual std::string_view name() const = 0;
    virtual Filtered filter(const SourceContext& context, std::string_view text) const = 0;
};

void register_source_filter(std::unique_ptr<SourceFilter> filter);
std::span<const std::unique_ptr<SourceFilter>> source_filters();

// Every filter in order, each over the previous one's text; a replacement that changes the length
// or the line breaks is refused (reported, and that filter's output dropped).
Filtered apply_source_filters(const SourceContext& context, std::string_view text);

template <class R>
struct Registration {
    Registration() {
        if constexpr (std::is_base_of_v<SourceFilter, R>) register_source_filter(std::make_unique<R>());
        else register_rule(std::make_unique<R>());
    }
};

} // namespace mcxx::plugin

namespace mcxx::plugin {

std::string_view to_string(Level level) {
    switch (level) {
    case Level::allow: return "allow";
    case Level::warn: return "warn";
    case Level::deny: return "deny";
    }
    return "allow";
}

std::optional<Level> parse_level(std::string_view name) {
    if (name == "allow" || name == "off") return Level::allow;
    if (name == "warn" || name == "warning") return Level::warn;
    if (name == "deny" || name == "error") return Level::deny;
    return std::nullopt;
}

namespace {
std::vector<std::unique_ptr<Rule>>& registry() {
    static std::vector<std::unique_ptr<Rule>> rules;
    return rules;
}
} // namespace

void register_rule(std::unique_ptr<Rule> rule) { registry().push_back(std::move(rule)); }

std::span<const std::unique_ptr<Rule>> rules() { return registry(); }

const Feature* find_feature(std::string_view id) {
    for (const auto& rule : registry())
        for (const auto& feature : rule->features())
            if (feature.id == id) return &feature;
    return nullptr;
}

std::string feature_macro_name(std::string_view feature) {
    std::string out;
    for (const char c : feature) out += std::isalnum(static_cast<unsigned char>(c)) ? static_cast<char>(std::toupper(static_cast<unsigned char>(c))) : '_';
    return out;
}

namespace {
std::vector<std::unique_ptr<SourceFilter>>& filter_registry() {
    static std::vector<std::unique_ptr<SourceFilter>> filters;
    return filters;
}

bool same_shape(std::string_view a, std::string_view b) {
    if (a.size() != b.size()) return false;
    for (std::size_t i { 0 }; i < a.size(); ++i)
        if ((a[i] == '\n') != (b[i] == '\n')) return false;
    return true;
}
} // namespace

void register_source_filter(std::unique_ptr<SourceFilter> filter) { filter_registry().push_back(std::move(filter)); }

std::span<const std::unique_ptr<SourceFilter>> source_filters() { return filter_registry(); }

Filtered apply_source_filters(const SourceContext& context, std::string_view text) {
    Filtered result;
    std::string current { text };
    bool changed { false };
    for (const auto& filter : filter_registry()) {
        Filtered one { filter->filter(context, current) };
        for (auto& p : one.problems) result.problems.push_back(std::move(p));
        if (!one.text) continue;
        if (!same_shape(current, *one.text)) {
            result.problems.push_back({ {}, msa::Severity::error,
                                        std::format("source filter {} changed the text's length or line breaks; its output is not used", filter->name()),
                                        "mcxx-filter", "MC++ plugin", {} });
            continue;
        }
        current = std::move(*one.text);
        changed = true;
    }
    if (changed) result.text = std::move(current);
    return result;
}

} // namespace mcxx::plugin
