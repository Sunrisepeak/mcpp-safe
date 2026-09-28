// The MC++ plugin SDK (MC4 v0). MC++ is itself built from providers written against it: its ISO C++
// feature controls are the built-in provider mc++.iso (mcxx.features), and a plugin is a provider
// in an mcpp package, registered at static initialization, that the program it is linked into
// (mcxx, or mcppls through libmc++) runs. A plugin can control (gate features of any category),
// extend (source filters: text before parsing), and override: provide a feature another provider
// -- MC++'s own included -- provides (Feature::replaces), or stand in for a whole provider
// (Provider::replaces).
//
//   struct JsonRules final : mcxx::plugin::Rule {
//       std::string_view name() const override { return "mcxx.plugins.json"; }
//       std::span<const Feature> features() const override { ... }
//       void check(const Context& c, std::vector<Finding>& out) const override { ... }
//   };
//   mcxx::plugin::Registration<JsonRules> registration;   // in the plugin's module
//
// A rule reads MSA facts (mcxx.msa fact::Facts) and reports findings; whether a finding is an
// error, a warning or nothing is not the rule's to decide but the configuration's (mcxx.features:
// profile, package, module, namespace, and [[mcpp::allow]] on a declaration).
//
// Cost: the registrations are resolved once into a Catalog. A host asks a rule only for the
// features that are not `allow` where it runs (Context::wants) and that can occur in the file
// (Feature::requires_declaration), and collects only the facts those features are decided from
// (Feature::needs); a file where nothing is left to ask is not walked at all.
export module mcxx.plugin;

import std;
import mcxx.msa;

export namespace mcxx::plugin {

enum class Level { allow, warn, deny };

std::string_view to_string(Level level);
std::optional<Level> parse_level(std::string_view name);   // "allow" | "warn" | "deny" (also "off", "error")

// What kind of thing a feature is. Every category is open to plugins.
enum class Category {
    iso,         // an ISO C++ language feature, by its stable names: denying it is subtraction, and what
                 // remains is ISO C++ (MC++'s built-in provider mc++.iso; a plugin may replace one)
    policy,      // how the language is used, not one ISO feature: "raw-pointers"
    library,     // which library facilities a program may use: "lib:std.vector"
    pitfall,     // a trap in a library or the language: "json-brace-init"
    extension,   // what MC++ adds to C++ ([[mcpp::cfg]]): code using it needs MC++ (profile portable denies them)
};

std::string_view to_string(Category category);
std::optional<Category> parse_category(std::string_view name);

// Who registered a provider. A built-in one is MC++'s; resolution prefers it, unless a plugin says
// it replaces it.
enum class Origin { builtin, plugin };

// One thing MC++ can gate (MC1).
struct Feature {
    std::string id;
    Category category { Category::policy };
    std::string standard;       // iso: the ISO C++ stable names it controls, "[stmt.goto]"
    std::string layer;          // where it is decided: "syntax" | "decl" | "expr" | "text"
    std::string summary;        // one line, for diagnostics and listings
    std::string fix;            // what to write instead
    Level default_level { Level::allow };
    bool waivable { true };     // [[mcpp::allow("id")]] may waive it
    // The level this feature has under a named profile (`profile = "safe"` in a package's
    // [package.metadata.mcxx]): a feature joins a profile by naming it.
    std::vector<std::pair<std::string, Level>> profiles;
    // The facts it is decided from: what a host collects when the feature is not `allow`.
    msa::fact::Kinds needs { msa::fact::Kinds::all };
    // A name at global scope (a namespace or a class: "nlohmann") without which the feature cannot
    // be found: a host skips it for a file that neither declares nor imports the name.
    std::string requires_declaration;
    // Provides an id another provider (MC++'s built-in one included) provides, instead of it. Without
    // it, a second provider of an id is a conflict, reported, and not used.
    bool replaces { false };
};

// A named set of levels (`profile = "safe"`, or several: `profile = ["safe", "modules"]`). Features
// join one by Feature::profiles; a profile may also include others and set a whole category.
struct Profile {
    std::string name;
    std::string summary;
    std::vector<std::string> includes;                    // profiles it contains
    std::vector<std::pair<Category, Level>> categories;   // a level for every feature of a category
    bool replaces { false };                              // redefines a profile of that name
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
    // The features this rule is asked for: those it provides that are not `allow` anywhere in the
    // file. Null: all of them (a test, a listing).
    const std::vector<std::string>* wanted { nullptr };

    bool wants(std::string_view id) const { return wanted == nullptr || std::ranges::find(*wanted, id) != wanted->end(); }
};

// What every plugin is: a name, what it can gate, the profiles it defines, and whom it stands in for.
class Provider {
public:
    virtual ~Provider() = default;
    virtual std::string_view name() const = 0;   // "mc++.iso", "mcxx.plugins.json"
    virtual std::span<const Feature> features() const { return {}; }
    virtual std::span<const Profile> profiles() const { return {}; }
    // Providers this one replaces, by name: they are not run, and their features and profiles are
    // this one's to provide, or nobody's.
    virtual std::span<const std::string_view> replaces() const { return {}; }
};

// Reads facts, reports findings.
class Rule : public Provider {
public:
    virtual void check(const Context& context, std::vector<Finding>& out) const = 0;
};

// ---- Source filters: text before parsing ---------------------------------------------------------
//
// A source filter sees a file's text before it is parsed, with the target it is compiled for, and
// may return a replacement: `[[mcpp::cfg(windows)]]` (plugins/std cfg) blanks what the target does
// not have. The replacement keeps the text's length and its line breaks, so every position a
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
    std::vector<Finding> findings;         // uses of the filter's own features (an extension's), gated as a rule's are
};

class SourceFilter : public Provider {
public:
    virtual Filtered filter(const SourceContext& context, std::string_view text) const = 0;
};

// ---- Registration and resolution -----------------------------------------------------------------

void register_rule(std::unique_ptr<Rule> rule, Origin origin = Origin::plugin);
void register_source_filter(std::unique_ptr<SourceFilter> filter, Origin origin = Origin::plugin);

template <class R, Origin O = Origin::plugin>
struct Registration {
    Registration() {
        if constexpr (std::is_base_of_v<SourceFilter, R>) register_source_filter(std::make_unique<R>(), O);
        else register_rule(std::make_unique<R>(), O);
    }
};

// Every registration, resolved: which providers run, which provider each feature id and profile
// name comes from, and what did not fit. Built once per set of registrations.
struct Catalog {
    struct Entry {
        const Feature* feature { nullptr };
        const Provider* provider { nullptr };
        Origin origin { Origin::plugin };
        std::vector<std::string> shadowed;   // providers whose feature of this id is not used
    };
    struct ProfileEntry {
        const Profile* profile { nullptr };
        const Provider* provider { nullptr };
    };
    struct ProviderEntry {
        const Provider* provider { nullptr };
        Origin origin { Origin::plugin };
    };
    std::uint64_t generation { 0 };
    std::vector<ProviderEntry> providers;        // the active ones: built-in first, then in registration order
    std::vector<const Rule*> rules;              // in that order
    std::vector<const SourceFilter*> filters;
    std::vector<Entry> features;                 // by id
    std::vector<ProfileEntry> profiles;          // by name
    std::vector<std::string> replaced;           // providers another stands in for
    std::vector<std::string> problems;           // conflicts: two providers of one id, neither replacing

    const Entry* find(std::string_view id) const;
    const Profile* profile(std::string_view name) const;
    Origin origin_of(const Provider* provider) const;
};

std::shared_ptr<const Catalog> catalog();
const Feature* find_feature(std::string_view id);   // the active provider's

// Every active filter in order, each over the previous one's text; a replacement that changes the
// length or the line breaks is refused (reported, and that filter's output dropped).
Filtered apply_source_filters(const SourceContext& context, std::string_view text);

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

std::string_view to_string(Category category) {
    switch (category) {
    case Category::iso: return "iso";
    case Category::policy: return "policy";
    case Category::library: return "library";
    case Category::pitfall: return "pitfall";
    case Category::extension: return "extension";
    }
    return "policy";
}

std::optional<Category> parse_category(std::string_view name) {
    for (const auto c : { Category::iso, Category::policy, Category::library, Category::pitfall, Category::extension })
        if (to_string(c) == name) return c;
    return std::nullopt;
}

std::string feature_macro_name(std::string_view feature) {
    std::string out;
    for (const char c : feature) out += std::isalnum(static_cast<unsigned char>(c)) ? static_cast<char>(std::toupper(static_cast<unsigned char>(c))) : '_';
    return out;
}

namespace {

struct Registered {
    std::unique_ptr<Provider> provider;
    const Rule* rule { nullptr };
    const SourceFilter* filter { nullptr };
    Origin origin { Origin::plugin };
};

struct Registry {
    std::mutex mutex;
    std::vector<Registered> providers;
    std::uint64_t generation { 1 };
    std::shared_ptr<const Catalog> catalog;
};

Registry& registry() {
    static Registry r;
    return r;
}

void add(Registered entry) {
    auto& r = registry();
    std::lock_guard lock { r.mutex };
    r.providers.push_back(std::move(entry));
    ++r.generation;
    r.catalog.reset();
}

// Of several providers of one name (a feature id, a profile), the one used: the only one that says
// it replaces; else a built-in one; else the first. The others are shadowed; a second provider
// that does not say it replaces is a conflict.
template <class T>
std::size_t choose(const std::vector<std::pair<const T*, const Registered*>>& candidates, bool (*replaces)(const T&), std::string_view what,
                   std::string_view name, std::vector<std::string>& problems) {
    std::vector<std::size_t> replacing;
    for (std::size_t i { 0 }; i < candidates.size(); ++i)
        if (replaces(*candidates[i].first)) replacing.push_back(i);
    auto provider_name = [&](std::size_t i) { return std::string { candidates[i].second->provider->name() }; };
    if (replacing.size() > 1) {
        std::string names;
        for (const auto i : replacing) names += (names.empty() ? "" : ", ") + provider_name(i);
        problems.push_back(std::format("{} `{}` is replaced by several providers ({}); {}'s is used", what, name, names, provider_name(replacing.front())));
    }
    if (!replacing.empty()) return replacing.front();
    std::size_t chosen { 0 };
    for (std::size_t i { 0 }; i < candidates.size(); ++i)
        if (candidates[i].second->origin == Origin::builtin) {
            chosen = i;
            break;
        }
    for (std::size_t i { 0 }; i < candidates.size(); ++i)
        if (i != chosen)
            problems.push_back(std::format("{} `{}` is provided by {} and by {}; {}'s is not used (to take it over, mark it `replaces`)", what, name,
                                           provider_name(chosen), provider_name(i), provider_name(i)));
    return chosen;
}

// MC1 §2: an id's shape, and what its category asks of it.
void check_feature(const Catalog::Entry& e, std::vector<std::string>& problems) {
    const Feature& f { *e.feature };
    std::string_view rest { f.id };
    if (rest.starts_with("lib:") || rest.starts_with("ext:")) rest.remove_prefix(4);
    bool well_formed { !rest.empty() };
    char previous { '-' };
    for (const char c : rest) {
        const bool word { (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') };
        if (!word && (c != '-' && c != '.')) well_formed = false;
        if (!word && previous == '-') well_formed = false;   // a separator after a separator, or first
        previous = word ? 'a' : '-';
    }
    if (previous == '-') well_formed = false;
    if (!well_formed) problems.push_back(std::format("feature id `{}` (of {}) is not lower-case words joined by - or . (MC1-2-1)", f.id, e.provider->name()));
    if (f.category == Category::library && !f.id.starts_with("lib:"))
        problems.push_back(std::format("library feature `{}` (of {}) does not start with lib: (MC1-2.1-2)", f.id, e.provider->name()));
    if (f.category == Category::extension && !f.id.starts_with("ext:"))
        problems.push_back(std::format("extension feature `{}` (of {}) does not start with ext: (MC1-2.1-3)", f.id, e.provider->name()));
    if (f.category == Category::iso && f.standard.empty())
        problems.push_back(std::format("ISO feature `{}` (of {}) names no stable name (MC1-2-3)", f.id, e.provider->name()));
    // An ISO feature is MC++'s to define; a plugin may only provide one instead of it.
    if (f.category == Category::iso && e.origin != Origin::builtin && e.shadowed.empty())
        problems.push_back(std::format("`{}` is category iso, but only MC++ defines ISO features; {} may replace one of them (MC1-2.1-1)", f.id,
                                       e.provider->name()));
}

std::shared_ptr<const Catalog> build(const std::vector<Registered>& providers, std::uint64_t generation) {
    auto c = std::make_shared<Catalog>();
    c->generation = generation;
    std::set<std::string, std::less<>> replaced;
    for (const auto& p : providers)
        for (const auto name : p.provider->replaces()) replaced.emplace(name);
    std::vector<const Registered*> active;
    // Built-in providers first: the order rules run and filters apply in.
    for (const auto origin : { Origin::builtin, Origin::plugin })
        for (const auto& p : providers)
            if (p.origin == origin && !replaced.contains(p.provider->name())) active.push_back(&p);
    c->replaced.assign(replaced.begin(), replaced.end());
    // A provider's name is how configuration, replacement and reports name it: unique (MC4-2-1).
    std::set<std::string_view> names;
    for (const auto* p : active)
        if (!names.insert(p->provider->name()).second)
            c->problems.push_back(std::format("two providers are named {}; name each provider uniquely (MC4-2-1)", p->provider->name()));

    std::map<std::string, std::vector<std::pair<const Feature*, const Registered*>>, std::less<>> by_id;
    std::map<std::string, std::vector<std::pair<const Profile*, const Registered*>>, std::less<>> by_profile;
    for (const auto* p : active) {
        c->providers.push_back({ p->provider.get(), p->origin });
        if (p->rule != nullptr) c->rules.push_back(p->rule);
        if (p->filter != nullptr) c->filters.push_back(p->filter);
        for (const auto& f : p->provider->features()) by_id[f.id].emplace_back(&f, p);
        for (const auto& pr : p->provider->profiles()) by_profile[pr.name].emplace_back(&pr, p);
    }
    for (const auto& [id, candidates] : by_id) {
        const std::size_t i { choose<Feature>(candidates, [](const Feature& f) { return f.replaces; }, "feature", id, c->problems) };
        Catalog::Entry entry { candidates[i].first, candidates[i].second->provider.get(), candidates[i].second->origin, {} };
        for (std::size_t j { 0 }; j < candidates.size(); ++j)
            if (j != i) entry.shadowed.emplace_back(candidates[j].second->provider->name());
        check_feature(entry, c->problems);
        c->features.push_back(std::move(entry));
    }
    for (const auto& [name, candidates] : by_profile) {
        const std::size_t i { choose<Profile>(candidates, [](const Profile& p) { return p.replaces; }, "profile", name, c->problems) };
        c->profiles.push_back({ candidates[i].first, candidates[i].second->provider.get() });
    }
    return c;
}

bool same_shape(std::string_view a, std::string_view b) {
    if (a.size() != b.size()) return false;
    for (std::size_t i { 0 }; i < a.size(); ++i)
        if ((a[i] == '\n') != (b[i] == '\n')) return false;
    return true;
}

} // namespace

void register_rule(std::unique_ptr<Rule> rule, Origin origin) {
    const Rule* raw { rule.get() };
    add({ std::move(rule), raw, nullptr, origin });
}

void register_source_filter(std::unique_ptr<SourceFilter> filter, Origin origin) {
    const SourceFilter* raw { filter.get() };
    add({ std::move(filter), nullptr, raw, origin });
}

std::shared_ptr<const Catalog> catalog() {
    auto& r = registry();
    std::lock_guard lock { r.mutex };
    if (!r.catalog) r.catalog = build(r.providers, r.generation);
    return r.catalog;
}

const Catalog::Entry* Catalog::find(std::string_view id) const {
    const auto it = std::ranges::lower_bound(features, id, {}, [](const Entry& e) { return std::string_view { e.feature->id }; });
    return it != features.end() && it->feature->id == id ? &*it : nullptr;
}

const Profile* Catalog::profile(std::string_view name) const {
    const auto it = std::ranges::lower_bound(profiles, name, {}, [](const ProfileEntry& e) { return std::string_view { e.profile->name }; });
    return it != profiles.end() && it->profile->name == name ? it->profile : nullptr;
}

Origin Catalog::origin_of(const Provider* provider) const {
    for (const auto& p : providers)
        if (p.provider == provider) return p.origin;
    return Origin::plugin;
}

const Feature* find_feature(std::string_view id) {
    const auto c = catalog();
    const Catalog::Entry* e { c->find(id) };
    return e != nullptr ? e->feature : nullptr;
}

Filtered apply_source_filters(const SourceContext& context, std::string_view text) {
    Filtered result;
    const auto c = catalog();
    if (c->filters.empty()) return result;
    std::string current { text };
    bool changed { false };
    for (const SourceFilter* filter : c->filters) {
        Filtered one { filter->filter(context, current) };
        for (auto& p : one.problems) result.problems.push_back(std::move(p));
        for (auto& f : one.findings) result.findings.push_back(std::move(f));
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
