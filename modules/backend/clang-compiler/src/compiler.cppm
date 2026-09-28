// clang itself, in this process: what the mcxx driver runs for a compile command.
export module mcxx.backend.clang.compiler;

import std;

export namespace mcxx::backend::clang::compiler {

// clang over this command line; argv[0] names the mode as clang reads it (…/clang++ is C++).
int run(int argc, char** argv);

// clang in `mode` ("clang++" or "clang") over `args`; `self` is this program's path (clang finds
// its tools and its resource directory beside it, and re-invokes itself for -cc1 when it must).
int run_as(std::string_view mode, const char* self, std::vector<std::string> args);

// Whether a program named `name` is clang in disguise: "clang++" for clang++, c++, g++, clang++-N;
// "clang" for clang, cc, gcc, clang-N; "" otherwise.
std::string_view mode_for_name(std::string_view name);

std::string_view clang_version();

} // namespace mcxx::backend::clang::compiler
