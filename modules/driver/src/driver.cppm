// The MC++ compiler driver (MC5), as a library: the mcxx program is `run` with MC++'s standard
// plugins linked in (src/main.cpp), and `mcxx compose` generates a program that is `run` with a
// package's static plugins linked in as well (MC4 §3).
//
//   mcxx c++ <compiler arguments>     a C++ compile, in process (so is mcxx named clang++, c++, g++)
//   mcxx cc <compiler arguments>      a C compile (so is mcxx named clang, cc, gcc)
//   mcxx check <compiler arguments>   the same command line, checked only (-fsyntax-only): MC++'s
//                                     feature gates and source filters, with the compiler's own diagnostics
//   mcxx features [--json]            what this program can gate: its providers (MC++'s built-in
//                                     mc++.iso and the plugins linked in), features, profiles, conflicts
//   mcxx compose [--manifest FILE]    builds the package's compiler with its static plugins
//   mcxx serve [--db DIR] ...         the semantic service over LSP's base protocol (MC6)
//   mcxx version [--json]
//
// The compiler is libmc++'s compiling facade (mcxx.backend.compiler): today clang 23.1 in process.
// The driver names no compiler: all of Clang is behind modules/backend.
export module mcxx.driver;

import std;

export namespace mcxx::driver {

inline constexpr std::string_view VERSION { "0.1.0" };

// mcxx's main. `composed`: the static plugin packages this program was composed with, and
// `composition` the key of that set (both empty for mcxx itself).
int run(int argc, char** argv, std::vector<std::string> composed = {}, std::string composition = {});

// The key of a package's static plugin set: what `mcxx compose` builds under, and what a compile
// looks for (MC4-3-3, MC4-3-4). Empty when the package declares none.
std::string composition_key(std::string_view manifest);

// Where composed compilers are built and kept: $MCXX_COMPOSE_CACHE, else $XDG_CACHE_HOME/mcxx/compose,
// else ~/.cache/mcxx/compose.
std::filesystem::path compose_cache();

} // namespace mcxx::driver
