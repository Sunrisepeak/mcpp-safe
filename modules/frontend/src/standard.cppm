// mcxx.frontend:standard -- which C++ standard a file is read as, and which of the language features of a
// newer one are on in it (the plan's ML-F, the lexer and preprocessor papers).
//
//   Language language { .standard = *parse_standard("c++2c"), .features = { { "c++26:embed", FeatureLevel::allow } } };
//   language.on("c++26:embed");     // true: the standard has it and its level is not deny
//
// A feature is on when two things hold: the command's standard is the one its paper is in or a later one
// (the -std the language provider of plugins/lang/cpp26 and cpp29 gives a file where one of its features
// is enabled), and the file's MC1 level for the feature's id is not `deny` (the level is the host's:
// the front end sees MSA only, not the plugin SDK, so the three values are spelled here and the host
// hands over the ones that apply to the file). A feature that is not on is not an error to the lexer:
// the use of one is a gate diagnostic that names the id and the paper (Diagnostic::feature, ::paper),
// which MC1 gives its own wording and severity.
export module mcxx.frontend:standard;

import std;

export namespace mcxx::frontend {

// MC1's levels of a feature in a file (plugin::Level's three values).
enum class FeatureLevel : std::uint8_t { deny, warn, allow };

// The standard a command asks for: its year by two digits (98 for c++98 and c++03), and whether it is a
// GNU dialect.
struct Standard {
    int year { 23 };
    bool gnu { false };
};

// -std's value (`c++26`, `c++2c`, `gnu++2d`, `c++0x`): none for any other (C, an unknown name).
std::optional<Standard> parse_standard(std::string_view name);

// What __cplusplus is under a standard, as Clang 23.1 defines it: 199711L, 201103L, ... 202302L, 202400L
// (C++26), 202700L (C++29).
std::string_view cplusplus_value(Standard standard);

// A language feature of a newer standard that the lexer or the preprocessor implements.
struct LanguageFeature {
    std::string_view id;       // the MC1 feature id (plugins/lang/cpp26, cpp29)
    std::string_view paper;    // the WG21 paper that adopted it
    int year;                  // the standard whose paper it is: 26 or 29
};

inline constexpr LanguageFeature EMBED { "c++26:embed", "P1967R14", 26 };
inline constexpr LanguageFeature PREPROCESSING_NEVER_UNDEFINED { "c++26:preprocessing-never-undefined", "P2843R3", 26 };
inline constexpr LanguageFeature EMBED_OFFSET { "c++29:embed-offset-parameter", "P3540R3", 29 };
inline constexpr LanguageFeature UNICODE_IDENTIFIERS { "c++29:unicode-identifier-recommendations", "P3658R1", 29 };

// Every feature above, for a host that lists them.
inline constexpr std::array<LanguageFeature, 4> LANGUAGE_FEATURES { EMBED, PREPROCESSING_NEVER_UNDEFINED, EMBED_OFFSET, UNICODE_IDENTIFIERS };

struct Language {
    Standard standard;                                              // C++23 unless the command says otherwise
    std::map<std::string, FeatureLevel, std::less<>> features;     // MC1 level by feature id; absent means deny

    FeatureLevel level(std::string_view id) const {
        const auto it = features.find(id);
        return it == features.end() ? FeatureLevel::deny : it->second;
    }
    // The standard is the feature's paper's or a later one.
    bool has(const LanguageFeature& feature) const { return standard.year >= feature.year; }
    // Standard and level both: the feature is on.
    bool on(const LanguageFeature& feature) const { return has(feature) && level(feature.id) != FeatureLevel::deny; }
};

} // namespace mcxx::frontend
