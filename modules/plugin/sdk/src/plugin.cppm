// The MC++ plugin SDK (MC4 v0). MC++ is itself built from providers written against it: its ISO C++
// feature controls are the built-in provider mc++.iso (mcxx.features), and a plugin is a provider
// in an mcpp package, registered at static initialization, that the program it is linked into
// (mcxx, or mcppls through libmc++) runs. A plugin can control (gate features of any category),
// extend (source filters: text before parsing; language providers: the compiler arguments a file's
// enabled language features need, MC4 0.4.0), and override: provide a feature another provider
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

export import :feature;
export import :rule;
export import :filter;
export import :language;
export import :catalog;
