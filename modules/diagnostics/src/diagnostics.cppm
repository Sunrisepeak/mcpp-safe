// mcxx.diagnostics -- a compile's diagnostics in the view its reader wants (specs/mc5-driver.md §9):
//
//   human   laid out as Rust lays out its errors: `error[code]: message`, ` --> file:line:column`, the
//           source line with the span underlined, then `= note:` and `= help:` (what to write instead,
//           how to allow it here, where the level is set) -- in color on a terminal;
//   agent   one JSON object per diagnostic (schema/mc5-diagnostic.schema.json): stable code, exact
//           ranges, the level and where it is set, the fix and the waiver as code, notes, fix-its;
//   clang   the compiler's own format, left as it is (a build log, an IDE's matcher).
//
// `--mcxx-diagnostics=human|agent|clang` chooses (the driver passes it on as MCXX_DIAGNOSTICS); with
// neither, human on a terminal and clang elsewhere. Nothing here knows a compiler: a backend turns its
// diagnostics into Items.
export module mcxx.diagnostics;

import std;
import mcxx.msa;

export namespace mcxx::diagnostics {

inline constexpr std::string_view AGENT_VERSION { "0.1.0" };   // the agent view's `mcxx-diagnostic`

enum class View { clang, human, agent };

std::optional<View> view_named(std::string_view name);
std::string_view to_string(View view);
// The view to render with: `asked` (the option, else $MCXX_DIAGNOSTICS), else human on a terminal and
// the compiler's own elsewhere.
View view_for(std::optional<View> asked, bool terminal);
// $MCXX_DIAGNOSTICS, when it names a view.
std::optional<View> view_from_environment();
bool stderr_is_terminal();
// Whether `view` is drawn in color on standard error: the human view on a terminal, unless $NO_COLOR.
bool colored(View view);

struct FixIt {
    msa::Range range;
    std::string text;   // what replaces the range ("" removes it)
};

// One diagnostic as a view needs it: where it is and the source lines it covers.
struct Item {
    msa::Diagnostic diagnostic;
    std::string path;                  // "" when it has no place
    std::vector<std::string> lines;    // the source lines from range.begin.line to range.end.line
    std::vector<FixIt> fixits;
    std::vector<Item> notes;           // a compiler's notes, each with its own place
};

// The human view of one diagnostic, its notes after it; lines end with '\n'.
std::string human(const Item& item, bool color);
// The agent view of one diagnostic: one line of JSON, '\n' at its end.
std::string agent(const Item& item);
// As `view` renders it; the clang view is not rendered here ("").
std::string render(const Item& item, View view, bool color);

} // namespace mcxx::diagnostics
