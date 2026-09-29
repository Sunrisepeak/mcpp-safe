// mcxx.frontend:types -- a declaration's type as text, from the tokens it is written with, as Clang's
// TypePrinter prints the type as written (its sugar kept: an alias, a qualified name, template
// arguments as spelled): "const std::string &", "char *const *", "int[3]", "void (*)(int)",
// "std::function<void (int)>", "unsigned long". MC3's `type` for a declaration (checked against the
// Clang backend's by tools/checks/declsdiff.py).
//
// What the tokens do not say is not guessed: a deduced type (`auto`, `decltype(auto)`), an array
// bound that is not a literal (Clang prints its value), a type the printer does not follow -- the text
// is then empty, "not known here".
export module mcxx.frontend:types;

import std;
import :lex;
import :preprocess;
import mcxx.msa;
import :syntax;

export namespace mcxx::frontend {

// The declared type of `d` (a variable, a field, a parameter as written, an alias's aliased type), or
// "" where it is not known from the tokens; `return_type`: a function's return type.
std::string type_text(const Syntax& syntax, const Declaration& d, bool return_type = false);

// The type [begin, end) of the file's tokens spells (a template argument, a parameter's type with its
// name taken out by the caller), or "" where it is not known.
std::string type_text(std::span<const PpToken> tokens);

} // namespace mcxx::frontend
