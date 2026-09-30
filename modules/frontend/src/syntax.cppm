// mcxx.frontend:syntax -- the declarations of a file, from its preprocessed tokens: namespaces,
// classes, enums and their enumerators, functions (bodies skipped by their braces), variables,
// fields, aliases, concepts, templates, with each one's name and whole range, and the tree they make.
// No types are known: where C++ needs them to tell a declaration's parts apart, the parser decides as
// the code's shape says (a name followed by a declarator is a type; a parenthesized list after a
// declarator is a parameter list unless it holds what only an expression can) -- checked against
// Clang's outline (tools/checks/syntaxdiff.py).
//
//   const auto syntax = mcxx::frontend::parse(text, { .file = path });
//   for (const auto& s : mcxx::frontend::symbols(syntax)) ... s.kind, s.name, s.range, s.selection
//
// It always gives a tree: what it cannot parse it skips (to the next `;` at its depth, or past a
// balanced block), says where (diagnostics), and goes on.
//
// What it decides, as it decides it, is traced in the category frontend.syntax at debug
// (MCXX_LOG=frontend.syntax=debug): each declaration recorded, each brace read as a block, a lambda's
// body or an initializer, each control statement's parentheses, each skip.
export module mcxx.frontend:syntax;

import std;
import mcxx.msa;
import mcxx.base;
import :lex;
import :preprocess;

export namespace mcxx::frontend {

struct Declaration {
    msa::Kind kind { msa::Kind::unknown };
    std::string name;             // as an outline names it: "f", "operator==", "~S", "operator bool"
    Where name_at;                // the name's first token
    Where at;                     // the whole declaration, from its first token to its last
    std::int32_t parent { -1 };   // index into Syntax::declarations; -1 at file scope
    bool exported { false };      // inside `export`
    bool definition { false };    // a body, a class's members, an enum's enumerators, a variable's initializer
    bool listed { true };         // in an outline: not in an unnamed namespace, class or enum, not a friend
    std::string qualifier;        // an out-of-line member's "S::", as written
    std::uint32_t name_token { 0 };    // indices into Syntax::pp.tokens
    std::uint32_t first_token { 0 };
    std::uint32_t last_token { 0 };
    bool pointer { false };            // its type holds a `*`: its declarator's, or one in a template argument
    bool c_array { false };            // its declarator is an array's (`x[3]`)
    bool c_variadic { false };         // a function with a C `...` parameter
    bool inline_namespace { false };
    bool va_list { false };            // its type is va_list: an array or a pointer, as the target has it
    bool in_lambda { false };          // declared in a lambda: a parameter, an init-capture, in its body
    // Its type's tokens (indices into Syntax::pp.tokens), for its type as text (type_text()): the
    // decl-specifiers [specifiers_begin, specifiers_end), the declarator [declarator_begin,
    // declarator_end) with the declarator-id [id_begin, id_end) among them (an empty one: abstract).
    // All zero where it has no type written (a namespace, a class, a structured binding).
    std::uint32_t specifiers_begin { 0 }, specifiers_end { 0 }, declarator_begin { 0 }, declarator_end { 0 }, id_begin { 0 }, id_end { 0 };
    // A local's, a parameter's: the last token it is visible at, the end of its block (0: its owner's
    // end -- a function's parameter, a namespace's declaration).
    std::uint32_t visible_end { 0 };
    std::uint32_t visible_begin { 0 };   // where it becomes visible, when not at its name: an init-capture's lambda body
    bool binding { false };   // a name a structured binding introduces (not a declaration of MC3's: its `[a, b]` is)
    bool scoped_enum { false };   // `enum class`, `enum struct`: its enumerators are named in it
    std::uint32_t bases_begin { 0 }, bases_end { 0 };   // a class's base-clause tokens, after its `:`
    // Its [[mcpp::allow("ids", "reason")]] waivers (MC1 §7).
    std::vector<std::pair<std::string, std::string>> allows;
};

// What code does that the gates look at, found by its tokens: inside functions' bodies and
// initializers (MC3's gotos, allocations, named casts, uses).
struct Construct {
    enum class What : std::uint8_t { goto_, new_, delete_, cast, throw_, try_, typeid_, asm_, va_arg };
    What what { What::goto_ };
    std::uint32_t first_token { 0 };
    std::uint32_t last_token { 0 };
    std::int32_t owner { -1 };    // the declaration it is in (a function, a variable), or -1
    std::string detail;           // goto: the label ("*" computed); new, delete: the type; cast: its kind; typeid: the operand
    std::string to;               // cast: the type in its <>
    bool array { false };         // new[], delete[]
};

// `using namespace n;` (a directive) and `using n::x;` (a declaration of x), for name lookup.
struct Using {
    bool directive { false };
    std::string name;                  // as written: "n", "a::b", "n::x"
    std::uint32_t at { 0 };            // its `using` (Syntax::pp.tokens)
    std::int32_t parent { -1 };        // the declaration it is in: a namespace, a function; -1 at file scope
    std::uint32_t visible_end { 0 };   // in a block: its last token; 0: to the end of its scope
    std::uint32_t name_token { 0 };    // a using-declaration's name: the last component's token
    bool exported { false };           // inside `export`
};

struct Syntax {
    Preprocessed pp;
    std::vector<std::uint32_t> line_starts;
    // In the order they are written; parameters and a function's local variables among them (a
    // function's children, not in an outline).
    std::vector<Declaration> declarations;
    std::vector<Using> usings;
    std::vector<Construct> constructs;
    std::vector<Diagnostic> diagnostics;     // where the parser could not follow the code, and skipped
};

// What a host knows that the file's text does not say: the aliases the modules it imports export,
// by name, and whether their types hold a pointer, are arrays (`export using Buf = int[16];`).
struct Known {
    struct Alias {
        bool pointer { false };
        bool c_array { false };
    };
    std::map<std::string, Alias, std::less<>> aliases;
};

// The file's declarations. The text must outlive the result (tokens view it).
Syntax parse(std::string_view text, const PreprocessOptions& options = {}, const Known& known = {});

// The aliases a module interface exports (for Known::aliases of its importers).
std::map<std::string, Known::Alias, std::less<>> exported_aliases(const Syntax& syntax);

// The outline, as MSA's Unit::symbols() has it: the listed declarations, each namespace's, class's and
// enum's own inside it.
std::vector<msa::Symbol> symbols(const Syntax& syntax);

// A position (MC3: 0-based line, UTF-8 bytes) of a byte of the text.
msa::Position position(const Syntax& syntax, std::uint32_t offset);

// The ranges Clang gives a declaration: its name, and the whole of it (symbols() uses them).
msa::Range selection_range(const Syntax& syntax, const Declaration& d);
msa::Range whole_range(const Syntax& syntax, const Declaration& d);
// The range of tokens [first, last], as Clang gives an expression's or a statement's.
msa::Range token_range(const Syntax& syntax, std::uint32_t first, std::uint32_t last);

} // namespace mcxx::frontend
