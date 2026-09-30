// mcxx.backend.clang implementation unit: the definitions of :support (Clang's stack, threads, commands).
module;

#include <clang/AST/ASTContext.h>
#include <clang/AST/Attr.h>
#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/DeclTemplate.h>
#include <clang/AST/Expr.h>
#include <clang/AST/PrettyPrinter.h>
#include <clang/AST/RawCommentList.h>
#include <clang/AST/Type.h>
#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/AllDiagnostics.h>
#include <clang/Basic/DiagnosticIDs.h>
#include <clang/Basic/DiagnosticOptions.h>
#include <clang/Basic/FileManager.h>
#include <clang/Basic/Module.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Basic/Stack.h>
#include <clang/Driver/CreateASTUnitFromArgs.h>
#include <clang/Driver/CreateInvocationFromArgs.h>
#include <clang/Frontend/ASTUnit.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/CompilerInvocation.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Frontend/Utils.h>
#include <clang/Index/IndexDataConsumer.h>
#include <clang/Index/IndexSymbol.h>
#include <clang/Index/IndexingAction.h>
#include <clang/Index/IndexingOptions.h>
#include <clang/Lex/Lexer.h>
#include <clang/Lex/Preprocessor.h>
#include <clang/Lex/PreprocessorOptions.h>
#include <clang/Sema/CodeCompleteConsumer.h>
#include <clang/Sema/Sema.h>
#include <clang/UnifiedSymbolResolution/USRGeneration.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/VirtualFileSystem.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/thread.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <filesystem>
#include <fstream>
#include <functional>
#include <future>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <random>
#include <set>
#include <shared_mutex>
#include <sstream>
#include <stop_token>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

module mcxx.backend.clang;

import mcxx.msa;
import mcxx.graph;
import mcxx.base;
import :support;

namespace mcxx::clang_backend {

namespace cl = ::clang;
namespace fs = std::filesystem;

void run_on_clang_stack(const std::function<void()>& body) {
    if constexpr (!STACK_SWITCH) {
        // The thread's own stack (clang_thread gives it DesiredStackSize): still libmc++'s Clang work.
        on_clang_stack = true;
        body();
        return;
    }
    const std::size_t total { CLANG_STACK + NOTE_DISTANCE + NOTE_STACK };
    std::unique_ptr<std::byte[]> memory { new std::byte[total] };
    const auto base = reinterpret_cast<std::uintptr_t>(memory.get());
    const std::uintptr_t top { (base + CLANG_STACK) & ~std::uintptr_t { 15 } };
    const std::uintptr_t noteTop { (base + total) & ~std::uintptr_t { 15 } };
    mcxx_call_on_stack(reinterpret_cast<void*>(noteTop), [](void*) { cl::noteBottomOfStack(true); }, nullptr);
    mcxx_call_on_stack(reinterpret_cast<void*>(top),
                       [](void* p) {
                           // Nothing unwinds across the switch: an exception ends here.
                           on_clang_stack = true;
                           try {
                               (*static_cast<const std::function<void()>*>(p))();
                           } catch (...) {
                           }
                       },
                       const_cast<std::function<void()>*>(&body));
}

std::unique_ptr<llvm::thread> clang_thread(std::function<void()> body) {
    return std::make_unique<llvm::thread>(std::optional<unsigned> { static_cast<unsigned>(cl::DesiredStackSize) },
                                          [body = std::move(body)] { run_on_clang_stack(body); });
}

ClangPool::ClangPool(unsigned size) {
    for (unsigned i { 0 }; i < std::max(1u, size); ++i) threads_.push_back(clang_thread([this] { loop_(); }));
}

ClangPool::~ClangPool() {
    {
        std::lock_guard lock { mutex_ };
        stopping_ = true;
    }
    cv_.notify_all();
    for (auto& t : threads_) t->join();
}

void ClangPool::loop_() {
    while (true) {
        std::function<void()> job;
        {
            std::unique_lock lock { mutex_ };
            cv_.wait(lock, [&] { return stopping_ || !queue_.empty(); });
            if (queue_.empty()) return;
            job = std::move(queue_.front());
            queue_.pop_front();
        }
        job();
    }
}

// ============================================================================================
// 1. commands
// ============================================================================================

// Paths by mcxx.base's rules, not std::filesystem's: beneath this program is a POSIX C library on every
// system, to which `C:\\Users\\x\\a.cpp` is a relative name; mcxx.base knows it for an absolute one
// on Windows, and keeps every path with '/' separators and its drive.
std::string normalize_path(std::string_view path, std::string_view base) {
    if (!base.empty() && !base::is_absolute_path(path)) return base::join_path(base, path);
    return base::normalize_path(path);
}

// `path` made absolute against the working directory, when it is not.
std::string absolute_path(std::string_view path) {
    if (base::is_absolute_path(path)) return base::normalize_path(path);
    std::error_code ec;
    return normalize_path(path, fs::current_path(ec).generic_string());
}

bool is_source_argument(std::string_view arg, const msa::Command& command) {
    if (arg.empty() || arg.front() == '-') return false;
    return normalize_path(arg, command.directory) == command.file;
}

// The build's arguments with everything this backend decides for itself taken out: where outputs
// go, which interfaces are read, the input language, the resource directory, and the source.
// What remains describes the program and is what a cached interface is keyed on.
// Whether a command's compiler is GCC, by its program's name (g++, gcc, x86_64-linux-gnu-g++-16, ...).
bool gcc_command(const msa::Command& command) {
    if (command.arguments.empty()) return false;
    const std::string program { fs::path { command.arguments.front() }.filename().string() };
    return program.find("clang") == std::string::npos && (program.find("g++") != std::string::npos || program.find("gcc") != std::string::npos);
}

std::vector<std::string> normalize(const msa::Command& command) {
    std::vector<std::string> out;
    const auto& a = command.arguments;
    if (a.empty()) return out;
    out.push_back(a.front());
    const bool gcc { gcc_command(command) };
    for (std::size_t i { 1 }; i < a.size(); ++i) {
        const std::string& x { a[i] };
        // GCC's C++20 modules switches (MC5-6-1): its -fmodules is Clang's header modules.
        if (gcc && (x == "-fmodules" || x == "-fmodule-only" || x == "-fmodule-header" || x.starts_with("-fmodule-header=") || x == "-fmodule-lazy" ||
                    x == "-fno-module-lazy" || x == "-fmodule-implicit-inline" || x.starts_with("-fdeps-")))
            continue;
        if (x == "-c" || x == "-MD" || x == "-MMD" || x == "-MP" || x == "--precompile" || x == "-fsyntax-only" ||
            x == "-fmodules-reduced-bmi" || x == "-fmodules-ts" || x == "-fdiagnostics-color" || x == "-fcolor-diagnostics")
            continue;
        if (x == "-o" || x == "-MF" || x == "-MT" || x == "-MQ" || x == "-x" || x == "-resource-dir" || x == "-fmodule-file" ||
            x == "-fmodule-output") {
            ++i;
            continue;
        }
        if ((x.starts_with("-o") && x.size() > 2 && x.find('=') == std::string::npos && !x.starts_with("-objc")) ||
            (x.starts_with("-x") && x.size() > 2 && !x.starts_with("-xarch")) || x.starts_with("-MF") || x.starts_with("-MT") ||
            x.starts_with("-MQ") || x.starts_with("-fmodule-file=") || x.starts_with("-fmodule-output") ||
            x.starts_with("-fprebuilt-module-path=") || x.starts_with("-resource-dir=") || x.starts_with("-fmodule-mapper=") ||
            x.starts_with("-fdiagnostics-color="))
            continue;
        if (is_source_argument(x, command)) continue;
        out.push_back(x);
    }
    return out;
}

// A command for a file the build does not list: the listed command of the nearest file (longest
// common directory, then the same extension), with the file swapped in.
std::optional<msa::Command> infer_command(const std::map<std::string, msa::Command, std::less<>>& commands,
                                          const std::string& file) {
    const msa::Command* best { nullptr };
    std::size_t bestScore { 0 };
    const fs::path target { file };
    for (const auto& [path, command] : commands) {
        const fs::path candidate { path };
        std::size_t common { 0 };
        auto a = target.begin();
        auto b = candidate.begin();
        for (; a != target.end() && b != candidate.end() && *a == *b; ++a, ++b) ++common;
        const std::size_t score { common * 4 + (candidate.extension() == target.extension() ? 2 : 0) +
                                  (candidate.extension() == ".cppm" || candidate.extension() == ".cpp" ? 1 : 0) };
        if (!best || score > bestScore) {
            best = &command;
            bestScore = score;
        }
    }
    if (!best) return std::nullopt;
    msa::Command inferred { best->directory, file, normalize(*best) };
    inferred.arguments.push_back(file);
    return inferred;
}

std::string hex_digest(std::string_view data) { return mcxx::base::sha256_hex(data); }

std::optional<std::string> read_file(const std::string& path) {
    std::ifstream in { path, std::ios::binary };
    if (!in) return std::nullopt;
    std::ostringstream text;
    text << in.rdbuf();
    return std::move(text).str();
}

std::string unique_suffix() {
    static const std::string process { [] {
        std::random_device device;
        return std::format("{:08x}{:08x}", device(), device());
    }() };
    return process + "-" + std::to_string(std::hash<std::thread::id> {}(std::this_thread::get_id()));
}

bool write_file_atomic(const std::string& path, std::string_view data) {
    const std::string tmp { path + ".tmp" + unique_suffix() };
    {
        std::ofstream out { tmp, std::ios::binary | std::ios::trunc };
        if (!out) return false;
        out.write(data.data(), static_cast<std::streamsize>(data.size()));
        if (!out) return false;
    }
    std::error_code ec;
    fs::rename(tmp, path, ec);
    return !ec;
}

// Arguments the backend adds to every compile: its own builtin headers, and every comment kept
// (hover shows plain `//` comments too).
std::vector<std::string> backend_arguments(const std::string& resourceDirectory) {
    std::vector<std::string> extra { "-fparse-all-comments", "-Wno-unknown-warning-option", "-Wno-unused-command-line-argument",
                                     "-ferror-limit=0" };
    if (!resourceDirectory.empty()) extra.push_back("-resource-dir=" + resourceDirectory);
    return extra;
}


void CollectingConsumer::HandleDiagnostic(cl::DiagnosticsEngine::Level level, const cl::Diagnostic& info) {
    cl::DiagnosticConsumer::HandleDiagnostic(level, info);
    const bool note { level == cl::DiagnosticsEngine::Note && errors.size() == 1 && notes.size() < 8 };
    if ((level < cl::DiagnosticsEngine::Error && !note) || errors.size() >= 8) return;
    llvm::SmallString<256> message;
    info.FormatDiagnostic(message);
    std::string where;
    if (info.getLocation().isValid() && info.hasSourceManager()) {
        const cl::PresumedLoc presumed { info.getSourceManager().getPresumedLoc(info.getLocation()) };
        if (presumed.isValid()) where = std::string { presumed.getFilename() } + ":" + std::to_string(presumed.getLine()) + ": ";
    }
    if (note && where.empty()) where = "(no location: an implicit declaration): ";
    (note ? notes : errors).push_back(where + std::string { message.str() });
}

std::string plain_name(const cl::NamedDecl* d) {
    // An unnamed namespace is `(anonymous namespace)`, as in its members' names: the plain style would
    // give it `(anonymous)` as its own name, and so to everything's container in it.
    if (const auto* ns = llvm::dyn_cast<cl::NamespaceDecl>(d); ns != nullptr && ns->isAnonymousNamespace()) {
        const auto* parent = llvm::dyn_cast<cl::NamedDecl>(cl::Decl::castFromDeclContext(ns->getParent()));
        const std::string prefix { parent != nullptr ? plain_name(parent) : std::string {} };
        return prefix.empty() ? std::string { "(anonymous namespace)" } : prefix + "::(anonymous namespace)";
    }
    cl::PrintingPolicy policy { d->getASTContext().getLangOpts() };
    policy.SuppressInlineNamespace = llvm::to_underlying(cl::PrintingPolicy::SuppressInlineNamespaceMode::All);
    policy.AnonymousTagNameStyle = llvm::to_underlying(cl::PrintingPolicy::AnonymousTagMode::Plain);
    std::string out;
    llvm::raw_string_ostream os { out };
    d->printQualifiedName(os, policy);
    return out;
}

} // namespace mcxx::clang_backend
