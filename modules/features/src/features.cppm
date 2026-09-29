// MC++ feature gates (MC1 v0): a finding of a provider's rule -- MC++'s built-in ISO controls
// (mc++.iso, mcxx.features.iso) or a plugin's -- becomes an error, a warning, a waiver or nothing, by
// the level its feature has where it was found.
//
// Configuration, in the package's mcpp.toml (mcpp keeps [package.metadata.*] and does not read it):
//
//   [package.metadata.mcxx]
//   profile = "safe"                                  # or several: ["safe", "modules"]
//
//   [package.metadata.mcxx.features]                  # the package
//   "json-brace-init" = "deny"
//   goto = "warn"
//
//   [package.metadata.mcxx.modules."app.legacy"]      # a module (a partition takes its module's)
//   goto = "allow"
//
//   [package.metadata.mcxx.namespaces."app::detail"]  # a namespace and those inside it
//   reinterpret-cast = "allow"
//
//   [package.metadata.mcxx.imports."legacy"]          # what an import may bring in (M1.2)
//   allow = ["c-array", "raw-pointers"]
//   reason = "the C library, wrapped"
//
// A declaration waives with [[mcpp::allow("id")]] or [[mcpp::allow("id", "why")]]. Precedence, the
// most specific first: declaration > namespace > module > package > profiles > the feature's
// default. Every waiver is recorded (audit); a feature that is not waivable stays what it is.
//
// Cost: a configuration is resolved once against the catalog into a Plan (cached by manifest, its
// modification time and the catalog); a file is then only walked for the facts its gated features
// need, and not at all when nothing is gated.
export module mcxx.features;

import std;
import mcxx.msa;
import mcxx.plugin;
import mcxx.base;
export import mcxx.features.iso;

export namespace mcxx::features {

using plugin::Level;
using LevelMap = std::map<std::string, Level, std::less<>>;

// A plugin a package's code is compiled with (MC4 §3): static (`path` or `version`: an mcpp package
// linked into the package's compiler by `mcxx compose`) or out of process (`command`).
struct PluginEntry {
    std::string name;
    std::string path;                      // as written: relative to the manifest's directory
    std::string version;
    std::vector<std::string> command;
    std::chrono::milliseconds timeout { 10000 };

    bool is_static() const { return command.empty(); }
};

// `[package.metadata.mcxx.imports."legacy"]  allow = ["c-array"]  reason = "..."`: what an import of the
// module may bring in across the dialect boundary (M1.2), in every file of the package. The manifest's
// form of `import legacy [[mcpp::allow("c-array")]];`, for a build tool whose own scanner does not take
// an attribute on an import (mcpp's). Recorded in the audit like any waiver.
struct ImportAllowance {
    std::vector<std::string> ids;
    std::string reason;
};

struct Config {
    std::string manifest;                  // the mcpp.toml it was read from ("" = none found)
    std::vector<PluginEntry> plugins;
    std::vector<std::string> profiles;     // `profile = "safe"` or `profile = ["safe", "modules"]`
    LevelMap package;
    std::map<std::string, LevelMap, std::less<>> modules;
    std::map<std::string, LevelMap, std::less<>> namespaces;
    // `[package.metadata.mcxx.files."src/legacy/**"]`: the levels of the package's files a glob matches,
    // relative to the manifest's directory (MC1 0.4.0).
    std::map<std::string, LevelMap, std::less<>> files;
    std::map<std::string, ImportAllowance, std::less<>> imports;   // by module name, as imported ("m", "m:p")
    std::vector<std::string> problems;     // what could not be read: told as warnings, never ignored
};

// The nearest mcpp.toml with a [package] table, from the file's directory upward.
std::optional<std::string> find_manifest(std::string_view source_path);
Config parse_config(std::string_view manifest_text, std::string manifest_path = {});
Config read_config(std::string_view manifest_path);

// The level a feature has under the named profiles, if any of them says: the strictest.
std::optional<Level> profile_level(const plugin::Catalog& catalog, const plugin::Feature& feature, std::span<const std::string> profiles);

// A configuration resolved against the catalog.
struct Plan {
    struct Gate {
        const plugin::Catalog::Entry* entry { nullptr };
        Level base { Level::allow };   // profiles, then the package
        bool maybe { false };          // not `allow` somewhere: base, a module or a namespace
    };
    Config config;
    std::shared_ptr<const plugin::Catalog> catalog;
    std::vector<Gate> gates;                               // parallel to catalog->features
    std::vector<std::vector<std::string>> wanted;          // per catalog->rules: the features to ask it for
    bool gated { false };                                  // some feature is not `allow` somewhere
    msa::fact::Kinds needs { msa::fact::Kinds::none };     // the facts those features are decided from
    std::vector<std::string> problems;                     // the config's, unknown ids and profiles, catalog conflicts

    bool idle() const { return !gated && problems.empty(); }
    const Gate* gate(std::string_view id) const;
    // The level of a gated feature for code in `module` and namespace `container`, in `file` (its
    // path relative to the manifest's directory, '/'-separated; "" when not known), before any waiver.
    Level level(const Gate& gate, std::string_view module, std::string_view container, std::string_view file = {}) const;
    // A source file's path as `files` patterns match it: relative to the manifest's directory ("" outside it).
    std::string relative(std::string_view path) const;
};

Plan make_plan(Config config);

// What to ask of one file: the plan's wanted features less those whose Feature::requires_declaration
// the file does not declare (`declared(name)`), and the facts the rest need.
struct Selection {
    std::vector<std::vector<std::string>> wanted;   // per catalog->rules
    msa::fact::Kinds needs { msa::fact::Kinds::none };
    bool gated { false };
};
Selection select(const Plan& plan, const std::function<bool(std::string_view)>& declared);
// The plan for a source file's package, cached.
std::shared_ptr<const Plan> plan_for(std::string_view source_path);

struct Waiver {
    std::string feature;
    std::string path;
    msa::Range range;          // the finding it waived
    std::string entity;        // the declaration carrying [[mcpp::allow]]
    std::string reason;
};

struct Result {
    std::vector<msa::Diagnostic> diagnostics;   // deny: error; warn: warning
    std::vector<Waiver> waived;
    std::vector<std::pair<std::string, msa::Range>> unknown;   // [[mcpp::allow]] ids no provider declares, and where
};

// The active rules over `context.facts`, plus findings a host already has (a source filter's), at
// the plan's levels.
Result evaluate(const plugin::Context& context, const Plan& plan, std::span<const plugin::Finding> prior = {});
Result evaluate(const plugin::Context& context, const Plan& plan, const Selection& selection, std::span<const plugin::Finding> prior = {});
Result evaluate(const plugin::Context& context, const Config& config);

// The features a profile does not leave at `allow`.
std::vector<std::string> profile_members(const plugin::Catalog& catalog, std::string_view profile);

// The catalog as MC1 §10 writes it (specs/schema/mc1-catalog.schema.json): what `mcxx features
// --json` prints and `mcxx serve` answers mcxx/catalog with.
std::string catalog_json(const plugin::Catalog& catalog);

// Appends the waivers as JSON lines to `file` (MCXX_AUDIT): one record per waiver.
void append_audit(std::string_view file, const std::vector<Waiver>& waivers);

} // namespace mcxx::features
