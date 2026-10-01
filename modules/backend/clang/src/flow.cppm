// mcxx.backend.clang partition :flow: MC3 0.9.0 control flow -- each function the file defines, as
// Clang's CFG of it gives it: blocks, the events in each in evaluation order (a local declared, read,
// written, its address taken, a call, a throw, a return, a lifetime's end), and the edges. What
// analyses on MC1's flow layer read (msa::fact::Flow); MC++'s own front end gives the same from MCIR
// later. Declarations; the definitions are in flow.cpp (MC5 §8).
module;

#include <clang/AST/ASTContext.h>

#include <vector>

module mcxx.backend.clang:flow;

import mcxx.msa;

namespace mcxx::clang_backend {

// The flow of every function, method and lambda the file's own code defines. A template's pattern is
// listed with known = false (its code depends on its arguments); instantiations are not the file's code.
std::vector<msa::fact::Flow> flows_of(::clang::ASTContext& ctx);

} // namespace mcxx::clang_backend
