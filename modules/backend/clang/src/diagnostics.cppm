// mcxx.backend.clang:diagnostics -- a compile's diagnostics in the view asked for (MC5 §9): Clang's
// own and MC++'s feature gates' alike, each with its notes and fix-its, rendered by mcxx.diagnostics.
// A gate's finding reaches Clang as text; what it says apart (its headline, fix, waiver, level and
// where the level is set) is left here by the gate before it reports it, and taken back by place and text.
module;

#include <clang/Basic/SourceLocation.h>
#include <clang/Frontend/CompilerInstance.h>

module mcxx.backend.clang:diagnostics;

import std;
import mcxx.msa;

namespace mcxx::clang_backend {

// A gate's finding, told before it is reported at `at` with `message` as its text.
void remember_finding(clang::SourceLocation at, const std::string& message, const msa::Diagnostic& finding);

// Replaces the compile's diagnostic printer with MC++'s human or agent view when one is asked for
// (--mcxx-diagnostics, $MCXX_DIAGNOSTICS) or stderr is a terminal; Clang's own printer otherwise.
// Not for libmc++'s own parses, which keep their diagnostics. Once per compile.
void install_view(clang::CompilerInstance& ci);

} // namespace mcxx::clang_backend
