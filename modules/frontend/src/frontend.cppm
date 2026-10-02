// mcxx.frontend: MC++'s own C++ front end (plan P4), beside the Clang backend and without Clang or
// LLVM. F1: lexing (:lex, token for token Clang's raw lexer -- tools/checks/lexdiff.py), phase 4 for
// a module unit (:preprocess), the declarations and the outline (:syntax -- syntaxdiff.py), and the
// MSA facts those give (facts()).
//
//   auto tokens = mcxx::frontend::lex(text);
//   auto pp = mcxx::frontend::preprocess(text, { .file = path });
//   auto facts = mcxx::frontend::facts(pp);   // MC3: macros and includes, for the gates (MC1)
export module mcxx.frontend;

import std;
import mcxx.msa;
export import :unicode;
export import :standard;
export import :lex;
export import :preprocess;
export import :embed;
export import :syntax;
export import :types;
export import :lookup;
export import :declared;
export import :ast;
export import :bodies;

export namespace mcxx::frontend {

// MC3 facts from what the preprocessor saw: the kinds `macros` (each #define in a group taken; its
// range the name) and `includes` (each #include in a group taken, once per header as written; its
// range the directive's line up to the header's end), as the Clang backend reports them. Certainty
// is `unknown` when the preprocessing was not certain (MC3-3-2).
msa::fact::Facts facts(const Preprocessed& pp);

// MC3 facts from the parse as well: `declarations` (a function's parameters and local variables among
// them; `pointer`, `c-array`, `c-variadic`, `union` from what is written -- an alias is not seen
// through), `gotos`, `allocations` (new and delete), `casts` (the named casts; a C-style or
// functional cast needs types: none of those), `uses` (throw, try, typeid, asm, va_arg),
// `suppressions` ([[mcpp::allow]]), with `macros` and `includes`. Containers and qualified names as
// the Clang backend gives them (inline namespaces left out). What it reads, it reads without types:
// the kinds are those of a syntax-level reading, and certainty is `unknown` when preprocessing was not
// certain.
msa::fact::Facts facts(const Syntax& syntax);

// The same, with each declaration's type as :declared resolves it with the file's imports: `templates`
// and `pointer` as Clang has them (the canonical type's), an injected-class-name as Clang prints it.
// Where F1 cannot tell (declared_types() says which), what the tokens say is kept.
msa::fact::Facts facts(const Syntax& syntax, const Imported& imported);

// `import m [[mcpp::allow("id, id", "reason")]];` (and `export import`): MC++'s annotation of an import
// that brings in what the importer's dialect restricts (M1.2). No attribute appertains to an import in
// Clang, which refuses a known one there, so a host blanks the attribute-specifier (`blank`, the bytes
// [begin, end)) before Clang reads the file, and keeps what it said as a waiver over the declaration.
// Only a specifier every attribute of which is `mcpp::allow` with string literals is one.
struct ImportAnnotation {
    std::string module;             // as written: "legacy", "m:part", ":part"
    msa::Range range;               // the declaration, `export` or `import` to `;`
    std::uint32_t begin { 0 };      // the attribute-specifier-seq's bytes
    std::uint32_t end { 0 };
    std::vector<std::string> ids;
    std::string reason;
};
std::vector<ImportAnnotation> import_annotations(std::string_view text);
// `text` with the annotations' bytes as spaces (line breaks kept): every position stays.
std::string blank_import_annotations(std::string_view text, std::span<const ImportAnnotation> annotations);

} // namespace mcxx::frontend
