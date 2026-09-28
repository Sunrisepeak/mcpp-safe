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

template <class R>
struct Registration {
    Registration() { register_rule(std::make_unique<R>()); }
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

} // namespace mcxx::plugin
