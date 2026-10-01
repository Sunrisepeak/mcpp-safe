// mcxx.plugins.lang.cpp29: C++29's core-language features (the plan's milestone ML, its list ML.1) as one
// MC4 language provider. Each paper is an MC1 feature of category standard ("c++29:reflection"), off
// unless a configuration enables it -- a package, a profile or a file glob:
//
//   [package.metadata.mcxx.features]
//   "c++29:pack-indexing-template-names" = "allow"
//
// A file where any of them is enabled is compiled as C++29 (-std=c++2d, which Clang 23.1 takes for the
// standard after C++26; gnu++2d when its command asks for a GNU dialect), unless its command already asks for
// C++29 or later. Written against the plugin
// SDK only.
export module mcxx.plugins.lang.cpp29;

import std;
import mcxx.msa;
import mcxx.plugin;
export import :table;

export namespace mcxx::plugins::lang::cpp29 {

inline constexpr int YEAR { 29 };

class Language final : public plugin::LanguageProvider {
public:
    Language();
    std::string_view name() const override { return "mcxx.plugins.lang.cpp29"; }
    std::span<const plugin::Feature> features() const override { return features_; }
    std::vector<std::string> arguments(const plugin::LanguageContext& context) const override;

private:
    std::vector<plugin::Feature> features_;
};

// The C++ standard a command asks for, by its two digits (23 for -std=c++23 or c++2b; 3 for c++98 and
// c++03), and whether in a GNU dialect; year 0 when it asks for none (or for C).
struct Asked {
    int year { 0 };
    bool gnu { false };
};
Asked asked(std::span<const std::string> arguments);

} // namespace mcxx::plugins::lang::cpp29

namespace mcxx::plugins::lang::cpp29 {

Language::Language() {
    for (const auto& p : PAPERS)
        features_.push_back(plugin::Feature { .id = std::string { p.id },
                                              .category = plugin::Category::standard,
                                              .standard = std::string { p.paper },
                                              .layer = "syntax",
                                              .summary = std::string { p.title },
                                              .default_level = plugin::Level::deny,
                                              .needs = msa::fact::Kinds::none });
}

std::vector<std::string> Language::arguments(const plugin::LanguageContext& context) const {
    if (std::ranges::none_of(features_, [&](const plugin::Feature& f) { return context.enabled(f.id); })) return {};
    const Asked a { asked(context.arguments) };
    std::vector<std::string> out;
    if (a.year < YEAR) out.emplace_back(a.gnu ? "-std=gnu++2d" : "-std=c++2d");
    // MC++'s Clang implements the papers of Status::todo and Status::partial, each behind its own option
    // (llvm-clang-dev's lang/cpp29/<feature>): once one has it, it is added here for a file where that
    // feature is enabled, e.g. `if (context.enabled("c++29:pack-indexing-template-names")) out.emplace_back(...)`.
    return out;
}

Asked asked(std::span<const std::string> arguments) {
    static constexpr std::array<std::pair<std::string_view, int>, 16> YEARS { {
        { "98", 3 }, { "03", 3 }, { "11", 11 }, { "0x", 11 }, { "14", 14 }, { "1y", 14 }, { "17", 17 }, { "1z", 17 },
        { "20", 20 }, { "2a", 20 }, { "23", 23 }, { "2b", 23 }, { "26", 26 }, { "2c", 26 }, { "29", 29 }, { "2d", 29 },
    } };
    Asked a;
    for (const auto& argument : arguments) {
        std::string_view v { argument };
        if (!v.starts_with("-std=") && !v.starts_with("--std=")) continue;
        v.remove_prefix(v.find('=') + 1);
        const auto plus = v.find("++");
        if (plus == std::string_view::npos) {   // C: the last -std is the one that counts
            a = {};
            continue;
        }
        a.gnu = v.starts_with("gnu");
        const auto version = v.substr(plus + 2);
        const auto it = std::ranges::find(YEARS, version, &std::pair<std::string_view, int>::first);
        a.year = it != YEARS.end() ? it->second : 0;
    }
    return a;
}

plugin::Registration<Language> registration;

} // namespace mcxx::plugins::lang::cpp29
