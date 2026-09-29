// The human view: a diagnostic as Rust lays out its errors.
//
//   error[raw-pointers]: `p` holds a raw pointer
//    --> src/a.cpp:3:10
//     |
//   3 |     int* p = nullptr;
//     |          ^ a raw pointer
//     |
//     = help: use std::unique_ptr, a reference or std::span
//     = help: to allow it here: [[mcpp::allow("raw-pointers", "<why>")]]
//     = note: deny, from profile `safe` (/p/mcpp.toml)
module mcxx.diagnostics;

import std;
import mcxx.msa;

namespace mcxx::diagnostics {

namespace {

struct Palette {
    bool on { false };
    std::string_view paint(std::string_view code) const { return on ? code : std::string_view {}; }
    std::string_view error() const { return paint("\x1b[1;31m"); }
    std::string_view warning() const { return paint("\x1b[1;33m"); }
    std::string_view note() const { return paint("\x1b[1;36m"); }
    std::string_view gutter() const { return paint("\x1b[1;34m"); }
    std::string_view bold() const { return paint("\x1b[1m"); }
    std::string_view reset() const { return paint("\x1b[0m"); }
};

std::string_view severity_word(msa::Severity s) {
    switch (s) {
    case msa::Severity::error: return "error";
    case msa::Severity::warning: return "warning";
    case msa::Severity::information: return "note";
    case msa::Severity::hint: return "help";
    }
    return "error";
}

// A tab as one column: the underline lines up with the text as it is printed.
std::string printable(std::string_view line) {
    std::string out { line };
    std::ranges::replace(out, '\t', ' ');
    while (!out.empty() && (out.back() == '\r' || out.back() == '\n')) out.pop_back();
    return out;
}

void excerpt(std::string& out, const Item& item, const Palette& c, std::string_view label, std::string_view underline_color) {
    const auto& r = item.diagnostic.range;
    if (item.lines.empty()) return;
    const std::uint32_t last_line { r.begin.line + static_cast<std::uint32_t>(item.lines.size()) - 1 };
    const std::size_t width { std::to_string(last_line + 1).size() };
    const std::string pad(width, ' ');
    out += std::format("{}{} |{}\n", c.gutter(), pad, c.reset());
    for (std::size_t i { 0 }; i < item.lines.size(); ++i) {
        const std::uint32_t line { r.begin.line + static_cast<std::uint32_t>(i) };
        const std::string text { printable(item.lines[i]) };
        out += std::format("{}{:>{}} |{} {}\n", c.gutter(), line + 1, width, c.reset(), text);
        // The span on this line: from the range's start (its first line) to its end (its last line).
        const std::size_t from { line == r.begin.line ? std::min<std::size_t>(r.begin.column, text.size()) : text.find_first_not_of(' ') };
        std::size_t to { line == r.end.line ? std::min<std::size_t>(r.end.column, text.size()) : text.size() };
        if (from == std::string::npos) continue;
        if (to <= from) to = from + 1;
        out += std::format("{}{} |{} {}{}{}{}{}\n", c.gutter(), pad, c.reset(), std::string(from, ' '), underline_color, std::string(to - from, '^'),
                           line == last_line && !label.empty() ? std::format(" {}", label) : std::string {}, c.reset());
    }
}

void one(std::string& out, const Item& item, const Palette& c, bool note) {
    const auto& d = item.diagnostic;
    const std::string_view word { note ? std::string_view { "note" } : severity_word(d.severity) };
    const std::string_view color { note ? c.note() : d.severity == msa::Severity::error ? c.error() : d.severity == msa::Severity::warning ? c.warning() : c.note() };
    const std::string headline { d.headline.empty() ? d.message : d.headline };
    out += std::format("{}{}{}{}{}: {}{}\n", color, word, d.code.empty() ? std::string {} : std::format("[{}]", d.code), c.reset(), c.bold(), headline, c.reset());
    if (!item.path.empty()) out += std::format("{} --> {}{}:{}:{}\n", c.gutter(), c.reset(), item.path, d.range.begin.line + 1, d.range.begin.column + 1);
    excerpt(out, item, c, {}, color);
    const std::string pad { std::string(item.lines.empty() ? 1 : std::to_string(d.range.begin.line + item.lines.size()).size(), ' ') };
    bool any { false };
    const auto say = [&](std::string_view kind, std::string_view text) {
        if (text.empty()) return;
        if (!any && !item.lines.empty()) out += std::format("{}{} |{}\n", c.gutter(), pad, c.reset());
        any = true;
        out += std::format("{}{} = {}{}: {}\n", c.gutter(), pad, c.bold(), kind, std::string { text } + std::string { c.reset() });
    };
    say("help", d.fix);
    if (!d.waiver.empty()) say("help", std::format("to allow it here: {}", d.waiver));
    for (const auto& f : item.fixits)
        say("help", f.text.empty() ? std::format("remove {}:{}-{}", f.range.begin.line + 1, f.range.begin.column + 1, f.range.end.column + 1)
                                   : std::format("replace {}:{}-{} with `{}`", f.range.begin.line + 1, f.range.begin.column + 1, f.range.end.column + 1, f.text));
    if (!d.level.empty()) say("note", std::format("{}, from {}", d.level, d.level_from));
    for (const auto& n : d.notes) say("note", n.message);
}

} // namespace

std::string human(const Item& item, bool color) {
    const Palette c { color };
    std::string out;
    one(out, item, c, false);
    for (const auto& n : item.notes) one(out, n, c, true);
    out += '\n';
    return out;
}

} // namespace mcxx::diagnostics
