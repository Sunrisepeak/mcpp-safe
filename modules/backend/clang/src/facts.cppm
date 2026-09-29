// mcxx.backend.clang partition :facts: MC3 v0 facts of a unit -- what its own code declares and does.
// Declarations only; the definitions are in facts.cpp (MC5 §8).
module;

#include <clang/AST/ASTContext.h>
#include <clang/Lex/Preprocessor.h>

#include <vector>

module mcxx.backend.clang:facts;

import mcxx.msa;

namespace mcxx::clang_backend {

// What the file's own code declares and does: the kinds in `needs` (a gate asks for what its
// features are decided from; nothing from the AST, no walk).
msa::fact::Facts facts_of(::clang::ASTContext& ctx, const ::clang::Preprocessor* pp, msa::fact::Kinds needs = msa::fact::Kinds::all);

// MC2 1.2.0: what an importer of this module unit can name that the unit's own code does not declare
// -- the declarations its exported using-declarations name (libc++'s std module is nothing else), the
// public members of the classes among them, recursively, and the enumerators of every enumeration an
// importer reaches, the unit's own exported ones included (MC3 has no enumerators among its T1
// declarations). A template is its pattern. Each once, with its type; no ranges (they are in other
// files), and exported: the export is what reaches it.
std::vector<msa::fact::Declaration> reachable_of(::clang::ASTContext& ctx);

} // namespace mcxx::clang_backend
