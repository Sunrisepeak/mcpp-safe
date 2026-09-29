// mcxx.frontend:preprocess -- translation phase 4 over one file, as far as a module unit needs it:
// the groups of #if and its kin taken, the file's own macros defined and expanded (#, ##,
// __VA_ARGS__, __VA_OPT__), the #include directives recorded, the module declaration and the imports
// recognized. What it does not do is read a header: an #include is recorded, not entered (in a module
// unit it belongs in the global module fragment, whose declarations are not the unit's own).
//
//   const auto pp = mcxx::frontend::preprocess(text, { .file = path, .target = "x86_64-unknown-linux-gnu" });
//   for (const auto& t : pp.tokens) ... t.kind, t.spelling, t.begin
//
// Where it cannot know what the compiler would do, it says so (`certain` false, with a note why) and
// makes the choice the compiler makes when nothing is known: a condition on a name a header included
// before may define as a macro, __has_include, a directive it does not cover (#line, #embed, ...).
// Names another target predefines and this one does not (`_WIN32` on Linux) are known to be undefined:
// headers do not define them.
export module mcxx.frontend:preprocess;

import std;
import :lex;
import :predefined;

export namespace mcxx::frontend {

// Bytes of the file and where they start.
struct Where {
    std::uint32_t begin { 0 };
    std::uint32_t end { 0 };
    std::uint32_t line { 1 };     // 1-based
    std::uint32_t column { 1 };   // 1-based, in bytes
};

// A token after preprocessing. `spelling` is its text with splices taken out: a view into the file,
// or into Preprocessed::storage for one # or ## made. `at` is the file's bytes it stands for: its own,
// or, for one a macro expansion gave (`expanded`), the invocation's.
struct PpToken {
    Kind kind { Kind::unknown };
    std::string_view spelling;
    Where at;
    bool start_of_line { false };
    bool leading_space { false };
    bool expanded { false };
    std::uint32_t macro_end { 0 };   // expanded: where the outermost invocation's macro name ends
};

struct IncludeDirective {
    std::string header;                  // as written, with its brackets or quotes
    Where at;                            // the directive: from its line's start to the header's end
    bool global_module_fragment { false };
};

struct MacroDefinition {
    std::string name;
    Where at;                            // the name
    bool function_like { false };
};

struct MacroUse {
    std::string name;
    Where at;                            // the invocation, as written in the file
};

struct Import {
    std::string name;                    // "std", "m:p", ":p", or a header unit as written ("<vector>")
    Where at;
    bool exported { false };
};

struct ModuleDeclaration {
    bool present { false };              // a module unit
    std::string name;                    // "m"
    std::string partition;               // "p" for `module m:p;`
    bool exported { false };
    bool global_module_fragment { false };   // it starts with `module;`
    bool private_fragment { false };     // it has `module :private;`
    Where at;
};

struct Diagnostic {
    enum class Severity : std::uint8_t { note, warning, error };
    Severity severity { Severity::error };
    std::string message;
    Where at;
};

struct Preprocessed {
    std::vector<PpToken> tokens;
    std::vector<IncludeDirective> includes;
    std::vector<MacroDefinition> macros;     // every #define of the file in a group taken
    std::vector<MacroUse> expansions;        // every invocation of a macro in the file's text
    std::vector<Import> imports;
    ModuleDeclaration module;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> skipped;   // the lines of each group not taken
    std::vector<Diagnostic> diagnostics;     // the reasons for `certain` false are notes here
    bool certain { true };
    std::string target;                      // whose predefined macros (PreprocessOptions::target)
    std::deque<std::string> storage;         // spellings no file byte has
};

struct PreprocessOptions {
    std::string file;                                  // what __FILE__ names
    std::string target { "x86_64-unknown-linux-gnu" }; // whose predefined macros (targets())
    std::vector<std::string> defines;                  // as -D takes them: NAME, NAME=VALUE, NAME(a,b)=VALUE
    std::vector<std::string> undefines;                // as -U takes them
    // The macros the headers the file includes define, as `-dM -E` prints them ("#define X 1"), when
    // the host has them (from the compiler): defined from the file's first #include on, and, when
    // complete, a name not among them is known not to be a macro.
    std::vector<std::string> header_macros;
    bool header_macros_complete { false };
};

Preprocessed preprocess(std::string_view text, const PreprocessOptions& options = {});

// The targets whose predefined macros are known.
std::vector<std::string_view> targets();

} // namespace mcxx::frontend
