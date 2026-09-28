// MC++ feature gates (MC1 v0): a finding of a plugin's rule becomes an error, a warning, a waiver or
// nothing, by the level its feature has where it was found.
//
// Configuration, in the package's mcpp.toml (mcpp keeps [package.metadata.*] and does not read it):
//
//   [package.metadata.mcxx]
//   profile = "safe"                                  # a named set of levels plugins declare
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
// A declaration waives with [[mcpp::allow("id")]] or [[mcpp::allow("id", "why")]]. Precedence, the
// most specific first: declaration > namespace > module > package > profile > the feature's default.
// Every waiver is recorded (audit); a feature that is not waivable stays what it is.
export module mcxx.features;

import std;
import mcxx.msa;
import mcxx.plugin;
import mcxx.base;

export namespace mcxx::features {

using plugin::Level;
using LevelMap = std::map<std::string, Level, std::less<>>;

struct Config {
    std::string manifest;                  // the mcpp.toml it was read from ("" = none found)
    std::string profile;                   // "" = none
    LevelMap package;
    std::map<std::string, LevelMap, std::less<>> modules;
    std::map<std::string, LevelMap, std::less<>> namespaces;
    std::vector<std::string> problems;     // what could not be read: told as warnings, never ignored
};

// The nearest mcpp.toml with a [package] table, from the file's directory upward.
std::optional<std::string> find_manifest(std::string_view source_path);
Config parse_config(std::string_view manifest_text, std::string manifest_path = {});
Config read_config(std::string_view manifest_path);
// The configuration that applies to a source file, cached by manifest and its modification time.
Config config_for(std::string_view source_path);

// The level of a feature for code in `module` and namespace `container`, before any waiver.
Level level_of(const plugin::Feature& feature, const Config& config, std::string_view module, std::string_view container);

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
    std::vector<std::pair<std::string, msa::Range>> unknown;   // [[mcpp::allow]] ids no linked rule declares, and where
};

Result evaluate(const plugin::Context& context, const Config& config);

// Appends the waivers as JSON lines to `file` (MCXX_AUDIT): one record per waiver.
void append_audit(std::string_view file, const std::vector<Waiver>& waivers);

} // namespace mcxx::features
