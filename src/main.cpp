// mcxx: the MC++ compiler driver (MC5 v0).
//
//   mcxx c++ <clang arguments>        clang++ itself, in process (so is mcxx named clang++, c++, clang, cc)
//   mcxx cc <clang arguments>         clang
//   mcxx check <clang arguments>      the same command line, checked only (-fsyntax-only): what MC++'s
//                                     feature gates say about it, with clang's own diagnostics
//   mcxx version
//
// The clang command line is clang's, whole: the driver, cc1 and cc1as run in this process
// (llvm.clang-driver) and the link step is spawned as clang spawns it. That is what makes mcxx a
// toolchain compiler for mcpp (V0.6) and what makes `mcxx check` see exactly what a build compiles.
#include <llvm/Support/InitLLVM.h>
#include <llvm/Support/LLVMDriver.h>

#include <cstdio>
#include <cstring>
#include <string>
#include <string_view>
#include <vector>

int clang_main(int argc, char** argv, const llvm::ToolContext&);

extern "C" {
void LLVMInitializeX86TargetInfo();
void LLVMInitializeX86Target();
void LLVMInitializeX86TargetMC();
void LLVMInitializeX86AsmPrinter();
void LLVMInitializeX86AsmParser();
void LLVMInitializeAArch64TargetInfo();
void LLVMInitializeAArch64Target();
void LLVMInitializeAArch64TargetMC();
void LLVMInitializeAArch64AsmPrinter();
void LLVMInitializeAArch64AsmParser();
}

namespace {

constexpr std::string_view VERSION { "0.1.0" };

void register_targets() {
    LLVMInitializeX86TargetInfo();
    LLVMInitializeX86Target();
    LLVMInitializeX86TargetMC();
    LLVMInitializeX86AsmPrinter();
    LLVMInitializeX86AsmParser();
    LLVMInitializeAArch64TargetInfo();
    LLVMInitializeAArch64Target();
    LLVMInitializeAArch64TargetMC();
    LLVMInitializeAArch64AsmPrinter();
    LLVMInitializeAArch64AsmParser();
}

std::string_view base_name(std::string_view path) {
    const auto slash = path.find_last_of("/\\");
    return slash == std::string_view::npos ? path : path.substr(slash + 1);
}

// What clang would be called for this name: "clang++" for a C++ driver, "clang" for a C one, nothing
// when the name is mcxx's own.
std::string_view clang_name_for(std::string_view name) {
    if (name.ends_with(".exe")) name.remove_suffix(4);
    if (name == "clang++" || name == "c++" || name == "g++" || name.starts_with("clang++-")) return "clang++";
    if (name == "clang" || name == "cc" || name == "gcc" || name.starts_with("clang-")) return "clang";
    return {};
}

// clang_main with argv[0] as clang would see it: the real path (clang finds its resource directory
// and its tools beside it) under the name that selects the driver mode.
int run_clang(std::string_view mode, const char* self, std::vector<std::string> args) {
    std::vector<std::string> storage;
    storage.reserve(args.size() + 2);
    std::string argv0 { self };
    const auto slash = argv0.find_last_of('/');
    std::string dir { slash == std::string::npos ? std::string {} : argv0.substr(0, slash + 1) };
    storage.push_back(dir + std::string { mode });
    for (auto& a : args) storage.push_back(std::move(a));
    std::vector<char*> argv;
    for (auto& s : storage) argv.push_back(s.data());
    argv.push_back(nullptr);
    // clang's ToolContext: its own path, for re-invoking itself; argv[0] names the mode.
    return clang_main(static_cast<int>(storage.size()), argv.data(), { self, nullptr, false });
}

void usage() {
    std::fputs("usage: mcxx c++|cc <clang arguments>\n"
               "       mcxx check <clang arguments>      check a compile command (-fsyntax-only)\n"
               "       mcxx version\n", stderr);
}

} // namespace

int main(int argc, char** argv) {
    llvm::InitLLVM init { argc, argv };
    register_targets();
    const std::string_view name { base_name(argv[0]) };

    // Named as a compiler, or re-invoked by clang itself (-cc1, -cc1as): clang, whole.
    if (const auto mode = clang_name_for(name); !mode.empty() || (argc > 1 && std::string_view { argv[1] }.starts_with("-cc1"))) {
        return clang_main(argc, argv, { argv[0], nullptr, false });
    }
    if (argc < 2) {
        usage();
        return 2;
    }
    const std::string_view command { argv[1] };
    std::vector<std::string> rest(argv + 2, argv + argc);
    if (command == "c++" || command == "cc") return run_clang(command == "c++" ? "clang++" : "clang", argv[0], std::move(rest));
    if (command == "check") {
        rest.push_back("-fsyntax-only");
        return run_clang("clang++", argv[0], std::move(rest));
    }
    if (command == "version" || command == "--version") {
        std::printf("mcxx %.*s (clang 23.1.0, llvm.clang-driver 23.1.0.2)\n", static_cast<int>(VERSION.size()), VERSION.data());
        return 0;
    }
    usage();
    return 2;
}
