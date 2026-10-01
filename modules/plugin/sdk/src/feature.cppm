// mcxx.plugin:feature -- what a provider declares: its features (a level, a category, what the
// facts it is decided from are), profiles, the attributes it claims.
export module mcxx.plugin:feature;

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
    standard,    // a core-language feature of a newer C++ standard ("c++26:reflection"): off unless enabled, and
                 // enabling it turns it on in the compiler (its provider's language arguments, MC4 0.4.0)
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
    std::string standard;       // iso: the ISO C++ stable names it controls, "[stmt.goto]"; standard: its paper, "P2996R13"
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
    std::vector<std::pair<std::string, Level>> features;  // a level for features by id, any provider's
};

// An attribute a provider claims (MC4 §2): `[[acme::hot]]` is accepted by the compiler (no unknown-
// attribute warning), and each use is an MC3 fact (fact::Attribute) the provider's rule reads with
// the facts of the declaration it is on (plugin::subtree). A region names a profile: within the
// declaration, that profile's levels apply where they are stricter than what applies otherwise.
struct AttributeSpec {
    std::string name;             // "acme::hot": a namespace and a name
    std::string summary;
    std::string region;           // a profile's name: the attribute is a region; "" : it is not
};

} // namespace mcxx::plugin
