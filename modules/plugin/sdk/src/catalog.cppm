// mcxx.plugin:catalog -- registration, and the catalog every registration is resolved into once.
export module mcxx.plugin:catalog;

import std;
import mcxx.msa;
import :feature;
import :rule;
import :filter;
import :language;

export namespace mcxx::plugin {

// ---- Registration and resolution -----------------------------------------------------------------

void register_rule(std::unique_ptr<Rule> rule, Origin origin = Origin::plugin);
void register_source_filter(std::unique_ptr<SourceFilter> filter, Origin origin = Origin::plugin);
void register_language_provider(std::unique_ptr<LanguageProvider> provider, Origin origin = Origin::plugin);

// What a plugin library and the compiler that loads it must agree on (MC4 §3, `library`): the
// layouts of this SDK's types and of MSA's facts, which the two share in one process. Raised
// whenever one of them changes. A library carries its own value as `mcxx_plugin_sdk_abi`.
inline constexpr int SDK_ABI { 3 };

// A library's providers register as its static objects are constructed, while it loads, before
// the host can ask it anything. The host holds them (MC4-3-7): registrations from hold_registrations()
// on are kept apart until release_registrations(), which takes them into the catalog -- or, for a
// library built against another SDK, drops them without touching them (the objects are its layout,
// not this one's). Returns how many there were.
void hold_registrations();
std::size_t release_registrations(bool take);

template <class R, Origin O = Origin::plugin>
struct Registration {
    Registration() {
        if constexpr (std::is_base_of_v<SourceFilter, R>) register_source_filter(std::make_unique<R>(), O);
        else if constexpr (std::is_base_of_v<LanguageProvider, R>) register_language_provider(std::make_unique<R>(), O);
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
    std::vector<const LanguageProvider*> languages;   // in that order too
    std::vector<Entry> features;                 // by id
    std::vector<ProfileEntry> profiles;          // by name
    struct AttributeEntry {
        const AttributeSpec* attribute { nullptr };
        const Provider* provider { nullptr };
    };
    std::vector<AttributeEntry> attributes;      // by name
    std::vector<std::string> replaced;           // providers another stands in for
    std::vector<std::string> problems;           // conflicts: two providers of one id, neither replacing

    const Entry* find(std::string_view id) const;
    const Profile* profile(std::string_view name) const;
    const AttributeEntry* attribute(std::string_view name) const;
    Origin origin_of(const Provider* provider) const;
};

std::shared_ptr<const Catalog> catalog();
const Feature* find_feature(std::string_view id);   // the active provider's

// The facts inside a range (an attribute's declaration): what an attribute's rule reads (MC4 §2).
msa::fact::Facts subtree(const msa::fact::Facts& facts, const msa::Range& range);

// A dialect boundary (M1.2): what the file's imports bring in, as the imported modules' MC2 interfaces
// say (fact::Import), that exhibits `feature` -- one finding per import, at the import: the first such
// exported declaration and how many more. A module whose own dialect denies `feature` gated it itself
// (what it exposes anyway it waived, and its audit says so), so what it brings in does not cross. A
// module whose interface was not read may bring in anything: that is a finding too, "not known". The
// finding is `feature`'s, so its level is the importer's, and `[[mcpp::allow("feature")]]` on the
// import waives it. `what` names the thing ("a C array"); `exhibits` says whether a declaration is one.
void report_imports(const msa::fact::Facts& facts, std::string_view feature, std::string_view what,
                    const std::function<bool(const msa::fact::Declaration&)>& exhibits, std::vector<Finding>& out);

// Every active filter in order, each over the previous one's text; a replacement that changes the
// length or the line breaks is refused (reported, and that filter's output dropped).
Filtered apply_source_filters(const SourceContext& context, std::string_view text);

// Every active language provider's arguments for the file, in order (MC4 0.4.0).
std::vector<std::string> language_arguments(const LanguageContext& context);

} // namespace mcxx::plugin
