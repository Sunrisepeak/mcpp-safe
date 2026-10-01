// -std's values and __cplusplus: mcxx.frontend:standard's definitions.
module mcxx.frontend;

import std;

namespace mcxx::frontend {

std::optional<Standard> parse_standard(std::string_view name) {
    // The spellings Clang 23.1 takes for C++, by the standard's two digits.
    static constexpr std::array<std::pair<std::string_view, int>, 16> YEARS { {
        { "98", 98 }, { "03", 98 }, { "11", 11 }, { "0x", 11 }, { "14", 14 }, { "1y", 14 }, { "17", 17 }, { "1z", 17 },
        { "20", 20 }, { "2a", 20 }, { "23", 23 }, { "2b", 23 }, { "26", 26 }, { "2c", 26 }, { "29", 29 }, { "2d", 29 },
    } };
    Standard s;
    s.gnu = name.starts_with("gnu++");
    if (!s.gnu && !name.starts_with("c++")) return std::nullopt;
    const auto version = name.substr(s.gnu ? 5 : 3);
    const auto it = std::ranges::find(YEARS, version, &std::pair<std::string_view, int>::first);
    if (it == YEARS.end()) return std::nullopt;
    s.year = it->second;
    return s;
}

std::string_view cplusplus_value(Standard standard) {
    switch (standard.year) {
    case 98: return "199711L";
    case 11: return "201103L";
    case 14: return "201402L";
    case 17: return "201703L";
    case 20: return "202002L";
    case 26: return "202400L";
    case 29: return "202700L";
    default: return "202302L";
    }
}

} // namespace mcxx::frontend
