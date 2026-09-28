// mcxx: the MC++ compiler driver (MC5 v0).
//
//   mcxx c++ <compiler arguments>     a C++ compile, in process (so is mcxx named clang++, c++, g++)
//   mcxx cc <compiler arguments>      a C compile (so is mcxx named clang, cc, gcc)
//   mcxx check <compiler arguments>   the same command line, checked only (-fsyntax-only): MC++'s
//                                     feature gates and source filters, with the compiler's own diagnostics
//   mcxx version
//
// The compiler is libmc++'s compiling facade (mcxx.backend.compiler): today clang 23.1 in process,
// with the plugins linked into this program (plugins/std, plugins/libs) in every compilation. The
// driver itself names no compiler: all of Clang is behind modules/backend.
import std;
import mcxx.backend.compiler;

namespace {

constexpr std::string_view VERSION { "0.1.0" };

std::string_view base_name(std::string_view path) {
    const auto slash = path.find_last_of("/\\");
    return slash == std::string_view::npos ? path : path.substr(slash + 1);
}

void usage() {
    std::print(std::cerr, "usage: mcxx c++|cc <compiler arguments>\n"
                          "       mcxx check <compiler arguments>      check a compile command (-fsyntax-only)\n"
                          "       mcxx version\n");
}

} // namespace

int main(int argc, char** argv) {
    namespace compiler = mcxx::backend::compiler;
    // Named as a compiler, or re-invoked by the compiler itself (-cc1, -cc1as): the compiler, whole.
    if (!compiler::mode_for_name(base_name(argv[0])).empty() || (argc > 1 && std::string_view { argv[1] }.starts_with("-cc1")))
        return compiler::run(argc, argv);
    if (argc < 2) {
        usage();
        return 2;
    }
    const std::string_view command { argv[1] };
    std::vector<std::string> rest(argv + 2, argv + argc);
    if (command == "c++" || command == "cc") return compiler::run_as(command == "c++" ? "c++" : "c", argv[0], std::move(rest));
    if (command == "check") {
        rest.push_back("-fsyntax-only");
        return compiler::run_as("c++", argv[0], std::move(rest));
    }
    if (command == "version" || command == "--version") {
        std::println("mcxx {} ({})", VERSION, compiler::version());
        return 0;
    }
    usage();
    return 2;
}
