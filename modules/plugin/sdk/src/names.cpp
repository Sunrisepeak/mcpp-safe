// The words levels and categories are written with, and a feature's macro name.
module mcxx.plugin;

import std;
import mcxx.msa;

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

} // namespace mcxx::plugin
