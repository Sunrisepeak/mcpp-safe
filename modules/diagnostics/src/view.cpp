// Choosing a view (MC5 §9).
module;

#include <unistd.h>

module mcxx.diagnostics;

import std;

namespace mcxx::diagnostics {

std::optional<View> view_named(std::string_view name) {
    if (name == "human") return View::human;
    if (name == "agent") return View::agent;
    if (name == "clang") return View::clang;
    return std::nullopt;
}

std::string_view to_string(View view) {
    switch (view) {
    case View::human: return "human";
    case View::agent: return "agent";
    case View::clang: return "clang";
    }
    return "clang";
}

std::optional<View> view_from_environment() {
    const char* named { std::getenv("MCXX_DIAGNOSTICS") };
    return named == nullptr ? std::nullopt : view_named(named);
}

bool stderr_is_terminal() { return ::isatty(2) == 1; }

bool colored(View view) { return view == View::human && stderr_is_terminal() && std::getenv("NO_COLOR") == nullptr; }

View view_for(std::optional<View> asked, bool terminal) {
    if (asked) return *asked;
    if (const auto from_environment = view_from_environment()) return *from_environment;
    return terminal ? View::human : View::clang;
}

std::string render(const Item& item, View view, bool color) {
    switch (view) {
    case View::human: return human(item, color);
    case View::agent: return agent(item);
    case View::clang: return {};
    }
    return {};
}

} // namespace mcxx::diagnostics
