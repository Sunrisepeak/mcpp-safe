// The compiling facade: what the mcxx driver runs for a compile command, without naming the
// compiler. Today clang 23.1 in process (mcxx.backend.clang.compiler).
export module mcxx.backend.compiler;

import std;
import mcxx.backend.clang.compiler;

export namespace mcxx::backend::compiler {

// The compiler over argv, whose argv[0] names the mode (a C++ or a C compiler).
inline int run(int argc, char** argv) { return clang::compiler::run(argc, argv); }

// The compiler in `mode` ("c++" or "c") over `args`; `self` is this program's path.
inline int run_as(std::string_view mode, const char* self, std::vector<std::string> args) {
    return clang::compiler::run_as(mode == "c" ? "clang" : "clang++", self, std::move(args));
}

// "c++", "c" or "": whether a program named `name` is taken to be a compiler (clang++, c++, cc, ...).
inline std::string_view mode_for_name(std::string_view name) {
    const auto mode = clang::compiler::mode_for_name(name);
    return mode == "clang++" ? "c++" : mode == "clang" ? "c" : "";
}

inline std::string version() { return std::format("clang {}", clang::compiler::clang_version()); }

} // namespace mcxx::backend::compiler
