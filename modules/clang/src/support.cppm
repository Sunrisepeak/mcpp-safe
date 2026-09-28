// mcxx.clang partition :support: Clang on its own stack (ClangPool), a build's arguments normalized, shared constants
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

module mcxx.clang:support;

import mcxx.msa;
import mcxx.graph;
import mcxx.base;

namespace mcxx::clang_backend {

namespace cl = ::clang;
namespace fs = std::filesystem;
using msa::Position;
using msa::Range;
using msa::Location;

inline constexpr std::string_view CLANG_VERSION { "23.1.0" };
// Bumped whenever what a cached interface depends on changes in this backend.
inline constexpr std::string_view CACHE_EPOCH { "mcxx.clang/2" };   // 2: reduced interfaces

// Every thread that runs Clang does so on a stack of its own of 16 MiB.
//
// openkal gives every thread a fixed 256 KiB stack (openkal-linux task.cpp kStack; the size a
// pthread attribute asks for is not used) and maps no guard page, so deep template or module code
// overflows silently into a neighbouring thread's descriptor. Until openkal takes a stack size, a
// thread switches to a stack this backend allocates before any Clang code runs on it.
extern "C" void mcxx_call_on_stack(void* top, void (*fn)(void*), void* arg);

#if defined(__x86_64__) && !defined(_WIN32)
asm(R"(
    .text
    .globl mcxx_call_on_stack
    .type mcxx_call_on_stack,@function
mcxx_call_on_stack:
    pushq %rbp
    movq %rsp, %rbp
    movq %rdi, %rsp
    movq %rdx, %rdi
    callq *%rsi
    movq %rbp, %rsp
    popq %rbp
    retq
    .size mcxx_call_on_stack, .-mcxx_call_on_stack
)");
inline constexpr bool STACK_SWITCH { true };
#elif defined(__aarch64__) && !defined(_WIN32)
asm(R"(
    .text
    .globl mcxx_call_on_stack
    .type mcxx_call_on_stack,%function
mcxx_call_on_stack:
    stp x29, x30, [sp, #-16]!
    mov x29, sp
    mov sp, x0
    mov x0, x2
    blr x1
    mov sp, x29
    ldp x29, x30, [sp], #16
    ret
    .size mcxx_call_on_stack, .-mcxx_call_on_stack
)");
inline constexpr bool STACK_SWITCH { true };
#else
extern "C" void mcxx_call_on_stack(void*, void (*fn)(void*), void* arg) { fn(arg); }
inline constexpr bool STACK_SWITCH { false };
#endif

inline constexpr std::size_t CLANG_STACK { std::size_t { 16 } << 20 };

// Clang measures its stack from the bottom a thread noted, and past DesiredStackSize (8 MiB) minus
// 256 KiB moves the rest of the work to a new thread "of 8 MiB" -- which openkal makes 256 KiB,
// where the work overflows. A distance larger than DesiredStackSize reads to Clang as a stack it
// does not understand, and it then never moves. So the bottom is noted once, from a small stack
// placed 9 MiB above the real one, before any Clang code runs on the thread; Clang's own later
// notes (FrontendAction, CompilerInstance) do not overwrite a bottom already noted.
inline constexpr std::size_t NOTE_DISTANCE { std::size_t { 9 } << 20 };
inline constexpr std::size_t NOTE_STACK { std::size_t { 64 } << 10 };

inline thread_local bool on_clang_stack { false };

void run_on_clang_stack(const std::function<void()>& body) {
    if constexpr (!STACK_SWITCH) {
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

// A pool of such threads for work a caller waits on.
class ClangPool {
public:
    explicit ClangPool(unsigned size) {
        for (unsigned i { 0 }; i < std::max(1u, size); ++i) threads_.push_back(clang_thread([this] { loop_(); }));
    }
    ~ClangPool() {
        {
            std::lock_guard lock { mutex_ };
            stopping_ = true;
        }
        cv_.notify_all();
        for (auto& t : threads_) t->join();
    }
    // Runs `f` on one of the pool's threads and waits; inline when the caller already runs on a
    // Clang stack (so a pool thread never waits on its own pool).
    template <class F>
    auto run(F&& f) -> decltype(f()) {
        if (on_clang_stack) return f();
        using R = decltype(f());
        std::packaged_task<R()> task { std::forward<F>(f) };
        auto result = task.get_future();
        {
            std::lock_guard lock { mutex_ };
            queue_.push_back([&task] { task(); });
        }
        cv_.notify_one();
        return result.get();
    }

private:
    std::mutex mutex_;
    std::condition_variable cv_;
    std::deque<std::function<void()>> queue_;
    bool stopping_ { false };
    std::vector<std::unique_ptr<llvm::thread>> threads_;
    void loop_() {
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
};

// ============================================================================================
// 1. commands
// ============================================================================================

std::string normalize_path(std::string_view path, std::string_view base = {}) {
    fs::path p { std::string { path } };
    if (p.is_relative() && !base.empty()) p = fs::path { std::string { base } } / p;
    return p.lexically_normal().generic_string();
}

bool is_source_argument(std::string_view arg, const msa::Command& command) {
    if (arg.empty() || arg.front() == '-') return false;
    return normalize_path(arg, command.directory) == command.file;
}

// The build's arguments with everything this backend decides for itself taken out: where outputs
// go, which interfaces are read, the input language, the resource directory, and the source.
// What remains describes the program and is what a cached interface is keyed on.
std::vector<std::string> normalize(const msa::Command& command) {
    std::vector<std::string> out;
    const auto& a = command.arguments;
    if (a.empty()) return out;
    out.push_back(a.front());
    for (std::size_t i { 1 }; i < a.size(); ++i) {
        const std::string& x { a[i] };
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

bool write_file_atomic(const std::string& path, std::string_view data) {
    const std::string tmp { path + ".tmp" + std::to_string(std::hash<std::thread::id> {}(std::this_thread::get_id())) };
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


// Diagnostics of a compile, kept as text: a module interface that fails is reported by module.
class CollectingConsumer : public cl::DiagnosticConsumer {
public:
    std::vector<std::string> errors;
    void HandleDiagnostic(cl::DiagnosticsEngine::Level level, const cl::Diagnostic& info) override {
        cl::DiagnosticConsumer::HandleDiagnostic(level, info);
        if (level < cl::DiagnosticsEngine::Error || errors.size() >= 8) return;
        llvm::SmallString<256> message;
        info.FormatDiagnostic(message);
        std::string where;
        if (info.getLocation().isValid() && info.hasSourceManager()) {
            const cl::PresumedLoc presumed { info.getSourceManager().getPresumedLoc(info.getLocation()) };
            if (presumed.isValid()) where = std::string { presumed.getFilename() } + ":" + std::to_string(presumed.getLine()) + ": ";
        }
        errors.push_back(where + std::string { message.str() });
    }
};

} // namespace mcxx::clang_backend

