// mcxx.backend.clang partition :facts: MC3 v0 facts of a unit -- what its own code declares and does.
// Declarations only; the definitions are in facts.cpp (MC5 §8).
module;

#include <clang/AST/ASTContext.h>
#include <clang/Lex/Preprocessor.h>
#include <clang/AST/ExprCXX.h>

#include <string>

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

// Shared with :flow.
// The namespace code in `dc` belongs to ("" = global).
std::string namespace_of(const ::clang::DeclContext* dc);
// Whether default-initialization leaves an object of this type indeterminate: a scalar, or an array of them.
bool indeterminate_type(const ::clang::ASTContext& ctx, ::clang::QualType type);
// A class default-initialized by a trivial default constructor: its members are left indeterminate.
bool trivially_default_constructed(const ::clang::CXXConstructExpr* construct);

} // namespace mcxx::clang_backend
