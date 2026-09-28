// mcxx.clang: MSA over Clang 23.1. Everything Clang-typed stays in this unit.
//
// Parts, in order:
//   1. commands     normalizing a build's arguments, and a command for a file the build omits
//   2. modules      building module interfaces (BMIs) in dependency order, cached by content
//   3. units        parsing a file: diagnostics, occurrences, symbols, entities
//   4. completion   code completion and signature help
//   5. index        the program index, built in the background
//   6. workspace    msa::Workspace over the parts
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

module mcxx.clang;

import mcxx.msa;
import mcxx.graph;
import mcxx.base;

namespace mcxx::clang_backend {

namespace cl = ::clang;
namespace fs = std::filesystem;
using msa::Position;
using msa::Range;
using msa::Location;

constexpr std::string_view CLANG_VERSION { "23.1.0" };
// Bumped whenever what a cached interface depends on changes in this backend.
constexpr std::string_view CACHE_EPOCH { "mcxx.clang/2" };   // 2: reduced interfaces

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
constexpr bool STACK_SWITCH { true };
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
constexpr bool STACK_SWITCH { true };
#else
extern "C" void mcxx_call_on_stack(void*, void (*fn)(void*), void* arg) { fn(arg); }
constexpr bool STACK_SWITCH { false };
#endif

constexpr std::size_t CLANG_STACK { std::size_t { 16 } << 20 };

// Clang measures its stack from the bottom a thread noted, and past DesiredStackSize (8 MiB) minus
// 256 KiB moves the rest of the work to a new thread "of 8 MiB" -- which openkal makes 256 KiB,
// where the work overflows. A distance larger than DesiredStackSize reads to Clang as a stack it
// does not understand, and it then never moves. So the bottom is noted once, from a small stack
// placed 9 MiB above the real one, before any Clang code runs on the thread; Clang's own later
// notes (FrontendAction, CompilerInstance) do not overwrite a bottom already noted.
constexpr std::size_t NOTE_DISTANCE { std::size_t { 9 } << 20 };
constexpr std::size_t NOTE_STACK { std::size_t { 64 } << 10 };

thread_local bool on_clang_stack { false };

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

// ============================================================================================
// 2. modules
// ============================================================================================

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

class ModuleStore {
public:
    struct Result {
        std::map<std::string, std::string> pcms;   // module -> interface file, dependencies first
        std::vector<std::string> failed;           // modules that could not be built (or depend on one)
        std::vector<std::string> missing;          // imported, provided by no unit
    };

    ModuleStore(std::string cacheDirectory, std::string resourceDirectory, unsigned workers, std::function<void(std::string_view)> log,
                std::function<void()> changed)
        : cacheDirectory_ { std::move(cacheDirectory) }, resourceDirectory_ { std::move(resourceDirectory) }, log_ { std::move(log) },
          changed_ { std::move(changed) } {
        std::error_code ec;
        fs::create_directories(cacheDirectory_ + "/modules", ec);
        for (unsigned i { 0 }; i < std::max(1u, workers); ++i)
            workers_.push_back(clang_thread([this] { work_(stop_.get_token()); }));
    }

    ~ModuleStore() {
        {
            std::lock_guard lock { mutex_ };
            stopping_ = true;
        }
        stop_.request_stop();
        cv_.notify_all();
        for (auto& w : workers_) w->join();
        workers_.clear();
    }

    // The program as the graph and the commands describe it now. Interfaces whose inputs changed are
    // rebuilt the next time they are required.
    void set_program(const mcxx::graph::Graph& graph, const std::map<std::string, msa::Command, std::less<>>& commands) {
        std::lock_guard lock { mutex_ };
        graph_ = graph;
        commands_ = commands;
        for (auto& [module, entry] : entries_) {
            if (entry.state == State::ready || entry.state == State::failed) entry.state = State::stale;
        }
    }

    void file_changed(const std::string& path) {
        std::lock_guard lock { mutex_ };
        for (auto& [module, entry] : entries_) {
            if (entry.state != State::ready && entry.state != State::failed) continue;
            if (entry.source == path || std::ranges::find(entry.inputs, path) != entry.inputs.end()) entry.state = State::stale;
        }
    }

    // Builds (or finds) the interfaces of `roots` and everything they import, and waits for them.
    Result require(const std::vector<std::string>& roots, std::stop_token cancel = {}) {
        Result result;
        std::unique_lock lock { mutex_ };
        const std::vector<std::string> order { graph_.closure(roots, &result.missing) };
        for (const auto& module : order) schedule_(module);
        dispatch_();
        cv_.wait(lock, [&] {
            if (cancel.stop_requested() || stopping_) return true;
            return std::ranges::all_of(order, [&](const std::string& m) {
                const auto& e = entries_[m];
                return e.state == State::ready || e.state == State::failed;
            });
        });
        for (const auto& module : order) {
            const auto& e = entries_[module];
            if (e.state == State::ready) result.pcms.emplace(module, e.pcm);
            else result.failed.push_back(module);
        }
        return result;
    }

    // Every module the program provides, built in the background.
    void prepare_all() {
        std::unique_lock lock { mutex_ };
        const std::vector<std::string> all { graph_.modules() };
        for (const auto& module : graph_.closure(all)) schedule_(module);
        dispatch_();
    }

    msa::Status status() const {
        std::lock_guard lock { mutex_ };
        msa::Status s;
        s.modules = graph_.modules().size();
        for (const auto& [module, e] : entries_) {
            if (e.state == State::ready) ++s.modules_ready;
            if (e.state == State::failed) {
                ++s.modules_failed;
                s.failures.emplace_back(module, e.error);
            }
            if (e.state == State::queued || e.state == State::building) s.busy = true;
        }
        return s;
    }

    std::string pcm_of(const std::string& module) const {
        std::lock_guard lock { mutex_ };
        const auto it = entries_.find(module);
        return it != entries_.end() && it->second.state == State::ready ? it->second.pcm : std::string {};
    }

private:
    enum class State { unknown, stale, queued, building, ready, failed };
    struct Entry {
        State state { State::unknown };
        std::string pcm;
        std::string key;
        std::string error;
        std::string source;
        std::vector<std::string> inputs;   // files the last build read
    };

    std::string cacheDirectory_;
    std::string resourceDirectory_;
    std::function<void(std::string_view)> log_;
    std::function<void()> changed_;
    mutable std::mutex mutex_;
    std::condition_variable_any cv_;
    mcxx::graph::Graph graph_;
    std::map<std::string, msa::Command, std::less<>> commands_;
    std::map<std::string, Entry, std::less<>> entries_;
    std::deque<std::string> ready_;   // scheduled modules whose dependencies are all done
    std::vector<std::string> waiting_;
    bool stopping_ { false };
    std::stop_source stop_;
    std::vector<std::unique_ptr<llvm::thread>> workers_;

    void schedule_(const std::string& module) {
        auto& e = entries_[module];
        if (e.state == State::unknown || e.state == State::stale) {
            e.state = State::queued;
            waiting_.push_back(module);
        }
    }

    // Moves queued modules whose dependencies are done to the ready queue. Holds mutex_.
    void dispatch_() {
        bool moved { false };
        for (auto it = waiting_.begin(); it != waiting_.end();) {
            bool done { true };
            for (const auto& dependency : graph_.requires_of(*it)) {
                if (graph_.provider(dependency).empty()) continue;
                const auto& d = entries_[dependency];
                if (d.state != State::ready && d.state != State::failed) {
                    if (d.state == State::unknown || d.state == State::stale) schedule_(dependency);
                    done = false;
                }
            }
            if (done) {
                ready_.push_back(*it);
                it = waiting_.erase(it);
                moved = true;
            } else {
                ++it;
            }
        }
        if (moved) cv_.notify_all();
    }

    void work_(std::stop_token stop) {
        while (true) {
            std::string name;
            {
                std::unique_lock lock { mutex_ };
                cv_.wait(lock, stop, [&] { return !ready_.empty() || stopping_; });
                if (stop.stop_requested() || stopping_) return;
                name = ready_.front();
                ready_.pop_front();
                entries_[name].state = State::building;
            }
            build_(name);
            {
                std::lock_guard lock { mutex_ };
                dispatch_();
            }
            cv_.notify_all();
            if (changed_) changed_();
        }
    }

    void build_(const std::string& module) {
        std::string file;
        msa::Command command;
        std::vector<std::pair<std::string, std::string>> dependencies;   // module, pcm (transitive)
        std::string dependencyKeys;
        bool dependencyFailed { false };
        std::string failedDependency;
        {
            std::lock_guard lock { mutex_ };
            file = graph_.provider(module);
            const auto it = commands_.find(file);
            if (it != commands_.end()) command = it->second;
            const std::vector<std::string> roots { graph_.requires_of(module) };
            for (const auto& dependency : graph_.closure(roots)) {
                const auto& d = entries_[dependency];
                if (d.state == State::ready) {
                    dependencies.emplace_back(dependency, d.pcm);
                    dependencyKeys += dependency + "=" + d.key + ";";
                } else {
                    dependencyFailed = true;
                    failedDependency = dependency;
                }
            }
        }
        auto finish = [&](bool ok, std::string pcm, std::string key, std::string error, std::vector<std::string> inputs) {
            std::lock_guard lock { mutex_ };
            auto& e = entries_[module];
            e.state = ok ? State::ready : State::failed;
            e.pcm = std::move(pcm);
            e.key = std::move(key);
            e.error = std::move(error);
            e.source = file;
            e.inputs = std::move(inputs);
        };
        if (dependencyFailed) {
            finish(false, {}, {}, "depends on " + failedDependency + ", which failed", {});
            log_(std::format("Failed to build module {}; due to its dependency {} failing", module, failedDependency));
            return;
        }
        if (command.arguments.empty()) {
            finish(false, {}, {}, "no compile command for " + file, {});
            return;
        }
        const auto source = read_file(file);
        if (!source) {
            finish(false, {}, {}, "cannot read " + file, {});
            return;
        }
        std::vector<std::string> args { normalize(command) };
        std::string keyText { std::string { CACHE_EPOCH } + "\n" };
        for (const auto& a : args) keyText += a + '\0';
        keyText += "\n" + file + "\n" + hex_digest(*source) + "\n" + dependencyKeys;
        const std::string key { hex_digest(keyText) };
        std::string safeName { module };
        std::ranges::replace(safeName, ':', '-');
        const std::string pcm { cacheDirectory_ + "/modules/" + safeName + "-" + key.substr(0, 16) + ".pcm" };
        const std::string manifest { pcm + ".inputs" };

        // A cached interface is reused when every file it read is as it was.
        if (fs::exists(pcm) && fs::exists(manifest)) {
            if (auto inputs = valid_inputs_(manifest)) {
                finish(true, pcm, key, {}, std::move(*inputs));
                log_(std::format("Reusing persistent module {} from {}", module, pcm));
                return;
            }
        }

        for (const auto& extra : backend_arguments(resourceDirectory_)) args.push_back(extra);
        for (const auto& [name, path] : dependencies) args.push_back("-fmodule-file=" + name + "=" + path);
        const std::string tmp { pcm + ".building" + std::to_string(std::hash<std::thread::id> {}(std::this_thread::get_id())) };
        args.insert(args.end(), { "-x", "c++-module", "--precompile", file, "-o", tmp });

        std::vector<const char*> argv;
        for (const auto& a : args) argv.push_back(a.c_str());

        auto vfs = llvm::vfs::getRealFileSystem();
        cl::DiagnosticOptions diagOptions;
        CollectingConsumer consumer;
        auto diags = cl::CompilerInstance::createDiagnostics(*vfs, diagOptions, &consumer, false);
        cl::CreateInvocationOptions options;
        options.Diags = diags;
        options.VFS = vfs;
        std::shared_ptr<cl::CompilerInvocation> invocation { cl::createInvocation(argv, options) };
        if (!invocation) {
            finish(false, {}, key, "the command could not be understood: " + (consumer.errors.empty() ? std::string {} : consumer.errors.front()), {});
            log_(std::format("Failed to build module {}; due to Failed to compile {}", module, file));
            return;
        }
        invocation->getFrontendOpts().OutputFile = tmp;
        cl::CompilerInstance instance { invocation };
        instance.setVirtualFileSystem(vfs);
        instance.createDiagnostics(&consumer, false);
        auto collector = std::make_shared<cl::DependencyCollector>();
        instance.addDependencyCollector(collector);
        cl::GenerateReducedModuleInterfaceAction action;
        const auto started = std::chrono::steady_clock::now();
        const bool ok { instance.ExecuteAction(action) && !instance.getDiagnostics().hasErrorOccurred() };
        const auto seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count();
        if (!ok) {
            std::error_code ec;
            fs::remove(tmp, ec);
            std::string reason { consumer.errors.empty() ? std::string { "errors" } : consumer.errors.front() };
            finish(false, {}, key, reason, {});
            log_(std::format("Failed to build module {}; due to Failed to compile {}: {}", module, file, reason));
            return;
        }
        std::vector<std::string> inputs;
        std::string listing;
        for (const auto& dependency : collector->getDependencies()) {
            const std::string path { normalize_path(dependency, command.directory) };
            std::error_code ec;
            const auto size = fs::file_size(path, ec);
            if (ec) continue;
            const auto time = fs::last_write_time(path, ec);
            if (ec) continue;
            inputs.push_back(path);
            listing += std::format("{}\t{}\t{}\n", path, size, time.time_since_epoch().count());
        }
        std::error_code ec;
        fs::rename(tmp, pcm, ec);
        if (ec) {
            finish(false, {}, key, "cannot place the interface: " + ec.message(), {});
            return;
        }
        write_file_atomic(manifest, listing);
        finish(true, pcm, key, {}, std::move(inputs));
        log_(std::format("Built module {} to {} in {:.2f} s", module, pcm, seconds));
    }

    static std::optional<std::vector<std::string>> valid_inputs_(const std::string& manifest) {
        const auto text = read_file(manifest);
        if (!text) return std::nullopt;
        std::vector<std::string> inputs;
        std::istringstream lines { *text };
        std::string line;
        while (std::getline(lines, line)) {
            const auto a = line.find('\t');
            const auto b = line.find('\t', a + 1);
            if (a == std::string::npos || b == std::string::npos) return std::nullopt;
            const std::string path { line.substr(0, a) };
            std::error_code ec;
            const auto size = fs::file_size(path, ec);
            if (ec || std::to_string(size) != line.substr(a + 1, b - a - 1)) return std::nullopt;
            const auto time = fs::last_write_time(path, ec);
            if (ec || std::format("{}", time.time_since_epoch().count()) != line.substr(b + 1)) return std::nullopt;
            inputs.push_back(path);
        }
        return inputs;
    }
};

// ============================================================================================
// 3. units
// ============================================================================================

std::string path_of(const cl::SourceManager& sm, cl::FileID fid) {
    if (const auto ref = sm.getFileEntryRefForID(fid)) return normalize_path(ref->getName());
    return {};
}

Position position_of(const cl::SourceManager& sm, cl::SourceLocation loc) {
    const auto [fid, offset] = sm.getDecomposedLoc(loc);
    bool invalid { false };
    const unsigned line { sm.getLineNumber(fid, offset, &invalid) };
    const unsigned column { sm.getColumnNumber(fid, offset, &invalid) };
    if (invalid || line == 0 || column == 0) return {};
    return { line - 1, column - 1 };
}

// The name token at `loc`, as a location in the file it is written in.
std::optional<Location> token_location(const cl::SourceManager& sm, const cl::LangOptions& lo, cl::SourceLocation loc) {
    if (loc.isInvalid()) return std::nullopt;
    const cl::SourceLocation file { sm.getFileLoc(loc) };
    const std::string path { path_of(sm, sm.getFileID(file)) };
    if (path.empty()) return std::nullopt;
    const Position begin { position_of(sm, file) };
    const unsigned length { cl::Lexer::MeasureTokenLength(sm.getSpellingLoc(loc), sm, lo) };
    return Location { path, { begin, { begin.line, begin.column + std::max(1u, length) } } };
}

std::optional<Range> source_range(const cl::SourceManager& sm, const cl::LangOptions& lo, cl::SourceRange range, cl::FileID main) {
    const cl::SourceLocation b { sm.getFileLoc(range.getBegin()) };
    const cl::SourceLocation e { sm.getFileLoc(range.getEnd()) };
    if (b.isInvalid() || e.isInvalid() || sm.getFileID(b) != main || sm.getFileID(e) != main) return std::nullopt;
    const Position end { position_of(sm, e) };
    return Range { position_of(sm, b), { end.line, end.column + cl::Lexer::MeasureTokenLength(e, sm, lo) } };
}

std::string usr_of(const cl::Decl* d) {
    llvm::SmallString<128> buffer;
    if (!d || cl::index::generateUSRForDecl(d, buffer)) return {};
    return std::string { buffer.str() };
}

msa::Kind kind_of(const cl::Decl* d) {
    if (!d) return msa::Kind::unknown;
    if (llvm::isa<cl::ConceptDecl>(d)) return msa::Kind::concept_;
    if (llvm::isa<cl::TypeAliasTemplateDecl>(d)) return msa::Kind::type_alias;
    if (const auto* t = llvm::dyn_cast<cl::TemplateDecl>(d)) {
        if (const auto* templated = t->getTemplatedDecl()) d = templated;
    }
    if (llvm::isa<cl::NamespaceDecl>(d)) return msa::Kind::namespace_;
    if (llvm::isa<cl::NamespaceAliasDecl>(d)) return msa::Kind::namespace_alias;
    if (const auto* r = llvm::dyn_cast<cl::RecordDecl>(d)) return r->isUnion() ? msa::Kind::union_ : r->isStruct() ? msa::Kind::struct_ : msa::Kind::class_;
    if (llvm::isa<cl::EnumDecl>(d)) return msa::Kind::enum_;
    if (llvm::isa<cl::EnumConstantDecl>(d)) return msa::Kind::enumerator;
    if (llvm::isa<cl::TypedefNameDecl>(d)) return msa::Kind::type_alias;
    if (llvm::isa<cl::CXXConstructorDecl>(d)) return msa::Kind::constructor;
    if (llvm::isa<cl::CXXDestructorDecl>(d)) return msa::Kind::destructor;
    if (llvm::isa<cl::CXXConversionDecl>(d)) return msa::Kind::conversion;
    if (llvm::isa<cl::CXXMethodDecl>(d)) return msa::Kind::method;
    if (llvm::isa<cl::FunctionDecl>(d)) return msa::Kind::function;
    if (llvm::isa<cl::FieldDecl>(d)) return msa::Kind::field;
    if (llvm::isa<cl::ParmVarDecl>(d)) return msa::Kind::parameter;
    if (llvm::isa<cl::VarDecl>(d) || llvm::isa<cl::BindingDecl>(d)) return msa::Kind::variable;
    if (llvm::isa<cl::TemplateTypeParmDecl>(d) || llvm::isa<cl::NonTypeTemplateParmDecl>(d) || llvm::isa<cl::TemplateTemplateParmDecl>(d))
        return msa::Kind::template_parameter;
    if (llvm::isa<cl::LabelDecl>(d)) return msa::Kind::label;
    return msa::Kind::unknown;
}

std::uint32_t roles_of(cl::index::SymbolRoleSet roles, llvm::ArrayRef<cl::index::SymbolRelation> relations) {
    using R = cl::index::SymbolRole;
    std::uint32_t out { 0 };
    if (roles & static_cast<unsigned>(R::Declaration)) out |= msa::role::declaration;
    if (roles & static_cast<unsigned>(R::Definition)) out |= msa::role::definition;
    if (roles & static_cast<unsigned>(R::Reference)) out |= msa::role::reference;
    if (roles & static_cast<unsigned>(R::Read)) out |= msa::role::read;
    if (roles & static_cast<unsigned>(R::Write)) out |= msa::role::write;
    if (roles & static_cast<unsigned>(R::Call)) out |= msa::role::call;
    if (roles & static_cast<unsigned>(R::Implicit)) out |= msa::role::implicit;
    for (const auto& relation : relations)
        if (relation.Roles & static_cast<unsigned>(R::RelationOverrideOf)) out |= msa::role::overrides;
    return out;
}

cl::PrintingPolicy printing_policy(const cl::ASTContext& ctx) {
    cl::PrintingPolicy p { ctx.getLangOpts() };
    p.TerseOutput = true;
    p.PolishForDeclaration = true;
    p.SuppressUnwrittenScope = true;
    p.SuppressTemplateArgsInCXXConstructors = true;
    return p;
}

std::string one_line(std::string text) {
    std::string out;
    bool space { false };
    for (char c : text) {
        if (c == '\n' || c == '\r' || c == '\t' || c == ' ') {
            space = !out.empty();
            continue;
        }
        if (space) out += ' ';
        space = false;
        out += c;
    }
    if (const auto brace = out.find(" {"); brace != std::string::npos && out.ends_with("}")) out.erase(brace);
    return out;
}

const cl::Decl* definition_of(const cl::Decl* d) {
    if (const auto* t = llvm::dyn_cast<cl::TemplateDecl>(d)) {
        if (const auto* templated = t->getTemplatedDecl()) d = templated;
    }
    if (const auto* f = llvm::dyn_cast<cl::FunctionDecl>(d)) return f->getDefinition();
    if (const auto* t = llvm::dyn_cast<cl::TagDecl>(d)) return t->getDefinition();
    if (const auto* v = llvm::dyn_cast<cl::VarDecl>(d)) return v->getDefinition();
    return nullptr;
}

std::optional<Location> name_location(const cl::Decl* d) {
    if (!d) return std::nullopt;
    const auto& ctx = d->getASTContext();
    return token_location(ctx.getSourceManager(), ctx.getLangOpts(), d->getLocation());
}

const cl::NamedDecl* type_decl_of(cl::QualType type) {
    if (type.isNull()) return nullptr;
    type = type.getNonReferenceType();
    while (true) {
        if (const auto* p = type->getAs<cl::PointerType>()) type = p->getPointeeType();
        else if (const auto* a = type->getAsArrayTypeUnsafe()) type = a->getElementType();
        else break;
    }
    if (const auto* typedefType = type->getAs<cl::TypedefType>()) return typedefType->getDecl();
    if (const auto* tag = type->getAsTagDecl()) return tag;
    if (const auto* specialization = type->getAs<cl::TemplateSpecializationType>()) {
        if (const auto* decl = specialization->getTemplateName().getAsTemplateDecl()) return decl;
    }
    return nullptr;
}

msa::Entity build_entity(const cl::Decl* d) {
    msa::Entity e;
    const auto& ctx = d->getASTContext();
    const auto& sm = ctx.getSourceManager();
    const cl::PrintingPolicy policy { printing_policy(ctx) };
    e.id = usr_of(d);
    e.kind = kind_of(d);
    if (const auto* nd = llvm::dyn_cast<cl::NamedDecl>(d)) {
        e.name = nd->getNameAsString();
        e.qualified_name = nd->getQualifiedNameAsString();
    }
    {
        std::string text;
        llvm::raw_string_ostream os { text };
        d->print(os, policy);
        e.signature = one_line(std::move(text));
    }
    const cl::Decl* subject { d };
    if (const auto* t = llvm::dyn_cast<cl::TemplateDecl>(d)) {
        if (const auto* templated = t->getTemplatedDecl()) subject = templated;
    }
    cl::QualType valueType;
    if (const auto* v = llvm::dyn_cast<cl::ValueDecl>(subject)) {
        valueType = v->getType();
        e.type = valueType.getAsString(policy);
    }
    if (const auto* t = llvm::dyn_cast<cl::TypedefNameDecl>(subject)) {
        valueType = t->getUnderlyingType();
        e.type = valueType.getAsString(policy);
    }
    if (const auto* f = subject->getAsFunction()) {
        e.return_type = f->getReturnType().getAsString(policy);
        valueType = f->getReturnType();
        for (const auto* p : f->parameters()) {
            msa::Parameter parameter { p->getNameAsString(), p->getType().getAsString(policy), {} };
            if (p->hasDefaultArg() && !p->hasUninstantiatedDefaultArg() && !p->hasUnparsedDefaultArg()) {
                if (const auto* init = p->getDefaultArg()) {
                    std::string text;
                    llvm::raw_string_ostream os { text };
                    init->printPretty(os, nullptr, policy);
                    parameter.default_value = text;
                }
            }
            e.parameters.push_back(std::move(parameter));
        }
    }
    if (const auto* c = llvm::dyn_cast<cl::EnumConstantDecl>(subject)) {
        llvm::SmallString<32> digits;
        c->getInitVal().toString(digits, 10);
        e.value = std::string { digits.str() };
    }
    if (const auto* comment = ctx.getRawCommentForAnyRedecl(d)) e.documentation = comment->getFormattedText(sm, ctx.getDiagnostics());
    if (const auto* m = d->getOwningModule(); m && m->isNamedModule()) e.module = m->getPrimaryModuleInterfaceName().str();
    for (const cl::DeclContext* dc { d->getDeclContext() }; dc; dc = dc->getParent()) {
        if (const auto* named = llvm::dyn_cast<cl::NamedDecl>(dc)) {
            e.container = named->getQualifiedNameAsString();
            break;
        }
    }
    switch (d->getAccess()) {
    case cl::AS_public: e.access = "public"; break;
    case cl::AS_protected: e.access = "protected"; break;
    case cl::AS_private: e.access = "private"; break;
    default: break;
    }
    e.declaration = name_location(d->getCanonicalDecl());
    if (const auto* def = definition_of(d)) e.definition = name_location(def);
    else if (const auto* nd = llvm::dyn_cast<cl::NamespaceDecl>(d)) e.definition = name_location(nd);
    if (const auto* typeDecl = type_decl_of(valueType); typeDecl && typeDecl != d) {
        e.type_entity = usr_of(typeDecl);
        e.type_location = name_location(typeDecl);
    }
    return e;
}

// Occurrences of named entities, as Clang's indexer reports them.
class OccurrenceConsumer : public cl::index::IndexDataConsumer {
public:
    using Handler = std::function<void(const cl::Decl*, cl::index::SymbolRoleSet, llvm::ArrayRef<cl::index::SymbolRelation>, cl::SourceLocation)>;
    explicit OccurrenceConsumer(Handler handler) : handler_ { std::move(handler) } {}
    bool handleDeclOccurrence(const cl::Decl* d, cl::index::SymbolRoleSet roles, llvm::ArrayRef<cl::index::SymbolRelation> relations,
                              cl::SourceLocation loc, ASTNodeInfo) override {
        if (d) handler_(d, roles, relations, loc);
        return true;
    }

private:
    Handler handler_;
};

cl::index::IndexingOptions indexing_options(bool locals) {
    cl::index::IndexingOptions o;
    o.SystemSymbolFilter = cl::index::IndexingOptions::SystemSymbolFilterKind::All;
    o.IndexFunctionLocals = locals;
    o.IndexParametersInDeclarations = locals;
    o.IndexTemplateParameters = locals;
    o.IndexImplicitInstantiation = false;
    o.IndexMacros = false;
    o.IndexMacrosInPreprocessor = false;
    return o;
}

msa::Severity severity_of(cl::DiagnosticsEngine::Level level) {
    switch (level) {
    case cl::DiagnosticsEngine::Error:
    case cl::DiagnosticsEngine::Fatal: return msa::Severity::error;
    case cl::DiagnosticsEngine::Warning: return msa::Severity::warning;
    case cl::DiagnosticsEngine::Remark: return msa::Severity::information;
    default: return msa::Severity::hint;
    }
}

std::vector<msa::Diagnostic> diagnostics_of(cl::ASTUnit& ast) {
    std::vector<msa::Diagnostic> out;
    const auto& sm = ast.getSourceManager();
    const auto& lo = ast.getLangOpts();
    const cl::FileID main { sm.getMainFileID() };
    auto& ids = *ast.getDiagnostics().getDiagnosticIDs();
    for (auto it = ast.stored_diag_begin(); it != ast.stored_diag_end(); ++it) {
        const cl::StoredDiagnostic& d { *it };
        if (d.getLevel() == cl::DiagnosticsEngine::Ignored) continue;
        const cl::FullSourceLoc loc { d.getLocation() };
        std::optional<Location> where;
        if (loc.isValid()) {
            if (auto token = token_location(sm, lo, loc)) {
                where = std::move(token);
                for (const auto& range : d.getRanges()) {
                    const cl::SourceRange r { range.getAsRange() };
                    if (auto full = source_range(sm, lo, r, main); full && where->path == path_of(sm, main) && full->contains(where->range.begin)) {
                        where->range = *full;
                        break;
                    }
                }
            }
        }
        if (d.getLevel() == cl::DiagnosticsEngine::Note) {
            if (!out.empty() && where) out.back().notes.push_back({ *where, std::string { d.getMessage() } });
            continue;
        }
        msa::Diagnostic diagnostic;
        diagnostic.severity = severity_of(d.getLevel());
        diagnostic.message = std::string { d.getMessage() };
        diagnostic.code = ids.getWarningOptionForDiag(d.getID()).str();
        diagnostic.category = cl::DiagnosticIDs::getCategoryNameFromID(cl::DiagnosticIDs::getCategoryNumberForDiag(d.getID())).str();
        if (where && where->path == path_of(sm, main)) {
            diagnostic.range = where->range;
        } else if (where) {
            // In a file the unit includes: report it at the top, naming the place.
            diagnostic.message = std::format("In included file {}:{}: {}", where->path, where->range.begin.line + 1, diagnostic.message);
            diagnostic.notes.push_back({ *where, "the error is here" });
        }
        out.push_back(std::move(diagnostic));
    }
    return out;
}

void collect_symbols(const cl::DeclContext* dc, const cl::SourceManager& sm, const cl::LangOptions& lo, cl::FileID main,
                     const cl::PrintingPolicy& policy, std::vector<msa::Symbol>& out) {
    for (const cl::Decl* d : dc->decls()) {
        if (const auto* x = llvm::dyn_cast<cl::ExportDecl>(d)) {
            collect_symbols(x, sm, lo, main, policy, out);
            continue;
        }
        if (const auto* l = llvm::dyn_cast<cl::LinkageSpecDecl>(d)) {
            collect_symbols(l, sm, lo, main, policy, out);
            continue;
        }
        const auto* nd = llvm::dyn_cast<cl::NamedDecl>(d);
        if (!nd || nd->isImplicit() || nd->getDeclName().isEmpty()) continue;
        const cl::SourceLocation nameLoc { sm.getFileLoc(nd->getLocation()) };
        if (nameLoc.isInvalid() || sm.getFileID(nameLoc) != main) continue;
        const msa::Kind kind { kind_of(nd) };
        if (kind == msa::Kind::unknown || kind == msa::Kind::parameter || kind == msa::Kind::template_parameter) continue;
        msa::Symbol symbol;
        symbol.name = nd->getNameAsString();
        symbol.kind = kind;
        const auto name = token_location(sm, lo, nd->getLocation());
        if (!name) continue;
        symbol.selection = name->range;
        symbol.range = source_range(sm, lo, nd->getSourceRange(), main).value_or(name->range);
        const cl::Decl* subject { nd };
        if (const auto* t = llvm::dyn_cast<cl::TemplateDecl>(nd)) {
            if (const auto* templated = t->getTemplatedDecl()) subject = templated;
        }
        if (const auto* f = subject->getAsFunction()) symbol.detail = cl::QualType { f->getType() }.getAsString(policy);
        else if (const auto* v = llvm::dyn_cast<cl::ValueDecl>(subject)) symbol.detail = v->getType().getAsString(policy);
        if (const auto* inner = llvm::dyn_cast<cl::DeclContext>(subject);
            inner && (llvm::isa<cl::NamespaceDecl>(subject) || llvm::isa<cl::TagDecl>(subject)))
            collect_symbols(inner, sm, lo, main, policy, symbol.children);
        out.push_back(std::move(symbol));
    }
}

class UnitImpl final : public msa::Unit {
public:
    UnitImpl(std::unique_ptr<cl::ASTUnit> ast, std::string path, std::string text, std::int64_t version, std::string module,
             std::vector<msa::Diagnostic> extra, ClangPool* pool)
        : pool_ { pool }, ast_ { std::move(ast) }, path_ { std::move(path) }, text_ { std::move(text) }, version_ { version },
          module_ { std::move(module) } {
        diagnostics_ = std::move(extra);
        if (!ast_) return;
        for (auto& d : diagnostics_of(*ast_)) diagnostics_.push_back(std::move(d));
        auto& sm = ast_->getSourceManager();
        const auto& lo = ast_->getLangOpts();
        const cl::FileID main { sm.getMainFileID() };
        OccurrenceConsumer consumer { [&](const cl::Decl* d, cl::index::SymbolRoleSet roles, llvm::ArrayRef<cl::index::SymbolRelation> relations,
                                          cl::SourceLocation loc) {
            const cl::SourceLocation file { sm.getFileLoc(loc) };
            if (file.isInvalid() || sm.getFileID(file) != main) return;
            const auto token = token_location(sm, lo, loc);
            if (!token) return;
            const cl::Decl* canonical { d->getCanonicalDecl() };
            std::string id { usr_of(canonical) };
            if (id.empty()) return;
            decls_.try_emplace(id, canonical);
            const auto* nd = llvm::dyn_cast<cl::NamedDecl>(d);
            occurrences_.push_back({ token->range, id, nd ? nd->getNameAsString() : std::string {}, kind_of(d), roles_of(roles, relations) });
        } };
        cl::index::indexASTUnit(*ast_, consumer, indexing_options(true));
        std::ranges::sort(occurrences_, [](const msa::Occurrence& a, const msa::Occurrence& b) {
            return std::tie(a.range.begin, a.range.end) < std::tie(b.range.begin, b.range.end);
        });
    }

    const std::string& path() const override { return path_; }
    std::int64_t version() const override { return version_; }
    std::string_view text() const override { return text_; }
    std::string_view module_name() const override { return module_; }
    std::span<const msa::Diagnostic> diagnostics() const override { return diagnostics_; }
    std::span<const msa::Occurrence> occurrences() const override { return occurrences_; }

    std::vector<msa::Symbol> symbols() const override {
        return pool_->run([&] { return symbols_(); });
    }

    std::vector<msa::Symbol> symbols_() const {
        std::lock_guard lock { mutex_ };
        std::vector<msa::Symbol> out;
        if (!ast_) return out;
        auto& ctx = ast_->getASTContext();
        collect_symbols(ctx.getTranslationUnitDecl(), ast_->getSourceManager(), ast_->getLangOpts(), ast_->getSourceManager().getMainFileID(),
                        printing_policy(ctx), out);
        return out;
    }

    std::optional<msa::Entity> entity_at(Position at) const override {
        const msa::Occurrence* best { nullptr };
        for (const auto& o : occurrences_) {
            if (at < o.range.begin) break;
            if (o.range.contains(at) && (!best || best->range.begin < o.range.begin)) best = &o;
        }
        if (!best) return std::nullopt;
        return entity(best->entity);
    }

    std::optional<msa::Entity> entity(std::string_view id) const override {
        return pool_->run([&] { return entity_(id); });
    }

    std::optional<msa::Entity> entity_(std::string_view id) const {
        std::lock_guard lock { mutex_ };
        const auto it = decls_.find(std::string { id });
        if (it == decls_.end()) return std::nullopt;
        return build_entity(it->second);
    }

    std::vector<Location> overriders(std::string_view id) const override {
        return pool_->run([&] { return overriders_(id); });
    }

    std::vector<Location> overriders_(std::string_view id) const {
        std::lock_guard lock { mutex_ };
        std::vector<Location> out;
        const auto it = decls_.find(std::string { id });
        if (it == decls_.end()) return out;
        const auto* method = llvm::dyn_cast<cl::CXXMethodDecl>(it->second);
        if (!method) return out;
        for (const auto& [otherId, decl] : decls_) {
            const auto* other = llvm::dyn_cast<cl::CXXMethodDecl>(decl);
            if (!other) continue;
            for (const auto* overridden : other->overridden_methods()) {
                if (overridden->getCanonicalDecl() == method->getCanonicalDecl()) {
                    if (auto loc = name_location(other)) out.push_back(*loc);
                }
            }
        }
        return out;
    }

    // For completion: the parse this snapshot came from, used exclusively.
    template <class F>
    auto with_ast(F&& f) const {
        std::lock_guard lock { mutex_ };
        return f(ast_.get());
    }

private:
    ClangPool* pool_;            // the Workspace's: AST access runs on a Clang stack
    mutable std::mutex mutex_;   // the AST loads declarations from interfaces lazily; one reader at a time
    std::unique_ptr<cl::ASTUnit> ast_;
    std::string path_;
    std::string text_;
    std::int64_t version_;
    std::string module_;
    std::vector<msa::Diagnostic> diagnostics_;
    std::vector<msa::Occurrence> occurrences_;
    std::unordered_map<std::string, const cl::Decl*> decls_;
};

struct ParseRequest {
    msa::Command command;
    std::string path;
    std::string text;
    bool remap { true };
    bool module_unit { false };
    std::map<std::string, std::string> modules;   // module -> interface file
    std::string resource_directory;
};

std::unique_ptr<cl::ASTUnit> parse_ast(const ParseRequest& request) {
    std::vector<std::string> args { normalize(request.command) };
    for (const auto& extra : backend_arguments(request.resource_directory)) args.push_back(extra);
    for (const auto& [name, pcm] : request.modules) args.push_back("-fmodule-file=" + name + "=" + pcm);
    if (request.module_unit) args.insert(args.end(), { "-x", "c++-module" });
    args.insert(args.end(), { "-fsyntax-only", request.path });
    std::vector<const char*> argv;
    for (const auto& a : args) argv.push_back(a.c_str());
    auto options = std::make_shared<cl::DiagnosticOptions>();
    auto vfs = llvm::vfs::getRealFileSystem();
    auto diags = cl::CompilerInstance::createDiagnostics(*vfs, *options);
    std::vector<cl::ASTUnit::RemappedFile> remapped;
    if (request.remap) remapped.emplace_back(request.path, llvm::MemoryBuffer::getMemBufferCopy(request.text, request.path).release());
    return cl::CreateASTUnitFromCommandLine(argv.data(), argv.data() + argv.size(), std::make_shared<cl::PCHContainerOperations>(), options, diags,
                                            request.resource_directory, false, {}, false, cl::CaptureDiagsKind::All, remapped, true, 0,
                                            cl::TU_Complete, false, true, false, cl::SkipFunctionBodiesScope::None, false, true);
}

// ============================================================================================
// 4. completion
// ============================================================================================

class CompletionCollector : public cl::CodeCompleteConsumer {
public:
    explicit CompletionCollector(const cl::CodeCompleteOptions& options)
        : cl::CodeCompleteConsumer(options), info_ { std::make_shared<cl::GlobalCodeCompletionAllocator>() } {}

    std::vector<msa::CompletionItem> items;
    msa::SignatureHelp signatures;

    void ProcessCodeCompleteResults(cl::Sema& sema, cl::CodeCompletionContext context, cl::CodeCompletionResult* results, unsigned count) override {
        for (unsigned i { 0 }; i < count; ++i) {
            cl::CodeCompletionResult& r { results[i] };
            if (r.Availability == CXAvailability_NotAvailable || r.Hidden) continue;
            const cl::CodeCompletionString* ccs { r.CreateCodeCompletionString(sema, context, getAllocator(), getCodeCompletionTUInfo(), true) };
            if (!ccs) continue;
            msa::CompletionItem item;
            std::string arguments;
            for (const auto& chunk : *ccs) {
                switch (chunk.Kind) {
                case cl::CodeCompletionString::CK_TypedText: item.label += chunk.Text; item.insert_text += chunk.Text; break;
                case cl::CodeCompletionString::CK_ResultType: item.detail = chunk.Text; break;
                case cl::CodeCompletionString::CK_Optional:
                case cl::CodeCompletionString::CK_Informative: break;
                case cl::CodeCompletionString::CK_Placeholder:
                case cl::CodeCompletionString::CK_CurrentParameter:
                case cl::CodeCompletionString::CK_Text:
                case cl::CodeCompletionString::CK_LeftParen:
                case cl::CodeCompletionString::CK_RightParen:
                case cl::CodeCompletionString::CK_LeftAngle:
                case cl::CodeCompletionString::CK_RightAngle:
                case cl::CodeCompletionString::CK_Comma:
                case cl::CodeCompletionString::CK_Colon:
                case cl::CodeCompletionString::CK_Equal:
                case cl::CodeCompletionString::CK_HorizontalSpace:
                    if (chunk.Text) arguments += chunk.Text;
                    break;
                default: break;
                }
            }
            if (item.label.empty()) continue;
            item.filter_text = item.label;
            if (!arguments.empty()) item.detail = item.detail.empty() ? arguments : item.detail + " " + item.label + arguments;
            if (const char* brief = ccs->getBriefComment()) item.documentation = brief;
            item.priority = ccs->getPriority();
            switch (r.Kind) {
            case cl::CodeCompletionResult::RK_Declaration: item.kind = kind_of(r.Declaration); break;
            case cl::CodeCompletionResult::RK_Macro: item.kind = msa::Kind::macro; break;
            default: item.kind = msa::Kind::unknown; break;
            }
            items.push_back(std::move(item));
        }
    }

    void ProcessOverloadCandidates(cl::Sema& sema, unsigned current, OverloadCandidate* candidates, unsigned count, cl::SourceLocation,
                                   bool braced) override {
        signatures.active_parameter = current;
        for (unsigned i { 0 }; i < count; ++i) {
            const cl::CodeCompletionString* ccs {
                candidates[i].CreateSignatureString(current, sema, getAllocator(), getCodeCompletionTUInfo(), true, braced)
            };
            if (!ccs) continue;
            msa::Signature signature;
            std::string resultType;
            for (const auto& chunk : *ccs) {
                if (!chunk.Text) continue;
                if (chunk.Kind == cl::CodeCompletionString::CK_ResultType) {
                    resultType = chunk.Text;
                    continue;
                }
                if (chunk.Kind == cl::CodeCompletionString::CK_Optional || chunk.Kind == cl::CodeCompletionString::CK_Informative) continue;
                const std::uint32_t begin { static_cast<std::uint32_t>(signature.label.size()) };
                signature.label += chunk.Text;
                if (chunk.Kind == cl::CodeCompletionString::CK_Placeholder || chunk.Kind == cl::CodeCompletionString::CK_CurrentParameter)
                    signature.parameters.emplace_back(begin, static_cast<std::uint32_t>(signature.label.size()));
            }
            if (!resultType.empty()) signature.label = resultType + " " + signature.label;
            if (!resultType.empty())
                for (auto& [b, e] : signature.parameters) {
                    b += static_cast<std::uint32_t>(resultType.size() + 1);
                    e += static_cast<std::uint32_t>(resultType.size() + 1);
                }
            if (const char* brief = ccs->getBriefComment()) signature.documentation = brief;
            signatures.signatures.push_back(std::move(signature));
        }
    }

    cl::CodeCompletionAllocator& getAllocator() override { return info_.getAllocator(); }
    cl::CodeCompletionTUInfo& getCodeCompletionTUInfo() override { return info_; }

private:
    cl::CodeCompletionTUInfo info_;
};

// Completion at `at` in `text`, re-parsing through the unit's own parse.
void run_completion(cl::ASTUnit& ast, const std::string& path, const std::string& text, Position at, CompletionCollector& collector) {
    llvm::IntrusiveRefCntPtr<cl::DiagnosticsEngine> diags { &ast.getDiagnostics() };
    cl::LangOptions lang { ast.getLangOpts() };
    llvm::IntrusiveRefCntPtr<cl::FileManager> files { &ast.getFileManager() };
    llvm::IntrusiveRefCntPtr<cl::SourceManager> sources { new cl::SourceManager { *diags, *files } };
    llvm::SmallVector<cl::StoredDiagnostic, 8> stored;
    llvm::SmallVector<const llvm::MemoryBuffer*, 1> owned;
    std::vector<cl::ASTUnit::RemappedFile> remapped { { path, llvm::MemoryBuffer::getMemBufferCopy(text, path).release() } };
    ast.CodeComplete(path, at.line + 1, at.column + 1, remapped, true, false, true, collector, std::make_shared<cl::PCHContainerOperations>(),
                     diags, lang, sources, files, stored, owned, nullptr);
    for (const auto* buffer : owned) delete buffer;
}

// ============================================================================================
// 5. index
// ============================================================================================

struct IndexedOccurrence {
    std::string entity;
    Location location;
    std::uint32_t roles { 0 };
};

struct IndexedEntity {
    std::string name;
    std::string container;
    msa::Kind kind { msa::Kind::unknown };
};

class ProgramIndex {
public:
    void replace(const std::string& file, std::vector<IndexedOccurrence> occurrences, std::map<std::string, IndexedEntity> entities) {
        std::unique_lock lock { mutex_ };
        if (const auto old = shards_.find(file); old != shards_.end()) {
            for (const auto& o : old->second) {
                auto& list = byEntity_[o.entity];
                std::erase_if(list, [&](const IndexedOccurrence* p) { return p->location.path == file; });
            }
        }
        auto& shard = shards_[file];
        shard = std::move(occurrences);
        for (const auto& o : shard) byEntity_[o.entity].push_back(&o);
        for (auto& [id, info] : entities) entities_.insert_or_assign(id, std::move(info));
    }

    std::vector<Location> with_role(std::string_view entity, std::uint32_t role) const {
        std::shared_lock lock { mutex_ };
        std::vector<Location> out;
        const auto it = byEntity_.find(std::string { entity });
        if (it == byEntity_.end()) return out;
        for (const auto* o : it->second)
            if ((o->roles & role) && std::ranges::find(out, o->location) == out.end()) out.push_back(o->location);
        return out;
    }

    std::vector<msa::Found> find(std::string_view query, std::size_t limit) const {
        std::shared_lock lock { mutex_ };
        std::vector<msa::Found> out;
        std::string needle { query };
        std::ranges::transform(needle, needle.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        for (const auto& [id, info] : entities_) {
            if (out.size() >= limit) break;
            if (info.kind == msa::Kind::parameter || info.kind == msa::Kind::template_parameter || info.kind == msa::Kind::unknown) continue;
            std::string name { info.name };
            std::ranges::transform(name, name.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            if (!needle.empty() && name.find(needle) == std::string::npos) continue;
            const auto defs = byEntity_.find(id);
            if (defs == byEntity_.end() || defs->second.empty()) continue;
            const IndexedOccurrence* at { nullptr };
            for (const auto* o : defs->second)
                if (o->roles & (msa::role::definition | msa::role::declaration)) {
                    at = o;
                    if (o->roles & msa::role::definition) break;
                }
            if (!at) continue;
            out.push_back({ id, info.name, info.container, info.kind, at->location });
        }
        return out;
    }

    std::size_t files() const {
        std::shared_lock lock { mutex_ };
        return shards_.size();
    }

private:
    mutable std::shared_mutex mutex_;
    std::map<std::string, std::vector<IndexedOccurrence>> shards_;   // node-stable: pointers into vectors stay valid until replaced
    std::unordered_map<std::string, std::vector<const IndexedOccurrence*>> byEntity_;
    std::unordered_map<std::string, IndexedEntity> entities_;
};

// Everything the index keeps of one parsed unit: occurrences outside function bodies' locals.
void index_unit(cl::ASTUnit& ast, const std::string& path, ProgramIndex& index) {
    auto& sm = ast.getSourceManager();
    const auto& lo = ast.getLangOpts();
    const cl::FileID main { sm.getMainFileID() };
    std::vector<IndexedOccurrence> occurrences;
    std::map<std::string, IndexedEntity> entities;
    OccurrenceConsumer consumer { [&](const cl::Decl* d, cl::index::SymbolRoleSet roles, llvm::ArrayRef<cl::index::SymbolRelation> relations,
                                      cl::SourceLocation loc) {
        const cl::SourceLocation file { sm.getFileLoc(loc) };
        if (file.isInvalid() || sm.getFileID(file) != main) return;
        if (const auto* v = llvm::dyn_cast<cl::VarDecl>(d); v && v->isLocalVarDeclOrParm()) return;
        const cl::Decl* canonical { d->getCanonicalDecl() };
        std::string id { usr_of(canonical) };
        if (id.empty()) return;
        auto token = token_location(sm, lo, loc);
        if (!token) return;
        token->path = path;
        occurrences.push_back({ id, *token, roles_of(roles, relations) });
        if (!entities.contains(id)) {
            IndexedEntity info;
            if (const auto* nd = llvm::dyn_cast<cl::NamedDecl>(d)) info.name = nd->getNameAsString();
            for (const cl::DeclContext* dc { d->getDeclContext() }; dc; dc = dc->getParent())
                if (const auto* named = llvm::dyn_cast<cl::NamedDecl>(dc)) {
                    info.container = named->getQualifiedNameAsString();
                    break;
                }
            info.kind = kind_of(d);
            entities.emplace(std::move(id), std::move(info));
        }
    } };
    cl::index::indexASTUnit(ast, consumer, indexing_options(false));
    index.replace(path, std::move(occurrences), std::move(entities));
}

// ============================================================================================
// 6. workspace
// ============================================================================================

class WorkspaceImpl final : public msa::Workspace {
public:
    explicit WorkspaceImpl(Options options) : options_ { std::move(options) } {
        if (!options_.log) options_.log = [](std::string_view) {};
        if (!options_.changed) options_.changed = [] {};
        if (options_.workers == 0) options_.workers = std::max(1u, std::thread::hardware_concurrency() / 4);
        modules_ = std::make_unique<ModuleStore>(options_.cache_directory, options_.resource_directory, options_.workers, options_.log,
                                                 options_.changed);
        for (unsigned i { 0 }; i < std::max(1u, options_.workers / 2); ++i)
            indexers_.push_back(clang_thread([this] { index_work_(stop_.get_token()); }));
        foreground_ = std::make_unique<ClangPool>(std::max(2u, options_.workers));
    }

    ~WorkspaceImpl() override {
        {
            std::lock_guard lock { mutex_ };
            stopping_ = true;
            indexQueue_.clear();
        }
        stop_.request_stop();
        indexCv_.notify_all();
        for (auto& t : indexers_) t->join();
        indexers_.clear();
        foreground_.reset();
        modules_.reset();
    }

    void set_commands(std::vector<msa::Command> commands) override {
        std::map<std::string, msa::Command, std::less<>> byFile;
        for (auto& c : commands) {
            c.file = normalize_path(c.file, c.directory);
            byFile.insert_or_assign(c.file, std::move(c));
        }
        mcxx::graph::Graph graph;
        for (const auto& [file, command] : byFile) {
            if (const auto text = read_file(file)) graph.set(file, mcxx::graph::scan(*text));
        }
        {
            std::lock_guard lock { mutex_ };
            commands_ = byFile;
            graph_ = graph;
        }
        modules_->set_program(graph, byFile);
        modules_->prepare_all();
        if (options_.background_index) {
            std::lock_guard lock { mutex_ };
            indexQueue_.clear();
            // Interfaces first: their declarations are what the rest of the program refers to.
            for (const auto& [file, command] : byFile) {
                const auto* s = graph.scan_of(file);
                if (s && s->provides_interface()) indexQueue_.push_back(file);
            }
            for (const auto& [file, command] : byFile) {
                const auto* s = graph.scan_of(file);
                if (!s || !s->provides_interface()) indexQueue_.push_back(file);
            }
            indexTotal_ = indexQueue_.size();
        }
        indexCv_.notify_all();
        options_.changed();
    }

    msa::Status status() const override {
        msa::Status s { modules_->status() };
        std::lock_guard lock { mutex_ };
        s.units = commands_.size();
        s.indexed = indexDone_;
        if (!indexQueue_.empty() || indexRunning_ > 0) s.busy = true;
        return s;
    }

    std::shared_ptr<const msa::Unit> parse(const std::string& file, std::string text, std::int64_t version, msa::Cancel cancel) override {
        return foreground_->run([&] { return parse_(file, std::move(text), version, cancel); });
    }

    std::vector<msa::CompletionItem> complete(const std::string& file, const std::string& text, Position at, msa::Cancel cancel) override {
        return foreground_->run([&] { return complete_(file, text, at, cancel); });
    }

    msa::SignatureHelp signature_help(const std::string& file, const std::string& text, Position at, msa::Cancel cancel) override {
        return foreground_->run([&] { return signature_help_(file, text, at, cancel); });
    }

    std::shared_ptr<const msa::Unit> parse_(const std::string& file, std::string text, std::int64_t version, msa::Cancel cancel) {
        const std::string path { normalize_path(file) };
        auto request = prepare_(path, text, cancel);
        if (!request) return nullptr;
        request->parse.remap = true;
        std::vector<msa::Diagnostic> extra { std::move(request->failures) };
        auto ast = parse_ast(request->parse);
        if (cancel.stop_requested()) return nullptr;
        auto unit = std::make_shared<UnitImpl>(std::move(ast), path, std::move(request->parse.text), version, request->module, std::move(extra),
                                               foreground_.get());
        {
            std::lock_guard lock { mutex_ };
            latest_[path] = unit;
        }
        return unit;
    }

    std::vector<msa::CompletionItem> complete_(const std::string& file, const std::string& text, Position at, msa::Cancel cancel) {
        auto unit = latest_unit_(normalize_path(file), text, cancel);
        if (!unit) return {};
        cl::CodeCompleteOptions options;
        options.IncludeMacros = true;
        options.IncludeCodePatterns = false;
        options.IncludeGlobals = true;
        options.IncludeBriefComments = true;
        options.LoadExternal = true;
        CompletionCollector collector { options };
        unit->with_ast([&](cl::ASTUnit* ast) {
            if (ast) run_completion(*ast, unit->path(), text, at, collector);
            return 0;
        });
        // Only what the identifier being typed can become.
        std::size_t offset { 0 };
        {
            std::uint32_t line { 0 };
            while (offset < text.size() && line < at.line) {
                if (text[offset] == '\n') ++line;
                ++offset;
            }
            offset = std::min(text.size(), offset + at.column);
        }
        std::size_t begin { offset };
        while (begin > 0 && (std::isalnum(static_cast<unsigned char>(text[begin - 1])) || text[begin - 1] == '_')) --begin;
        std::string prefix { text.substr(begin, offset - begin) };
        std::ranges::transform(prefix, prefix.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        std::vector<msa::CompletionItem> items;
        for (auto& item : collector.items) {
            std::string label { item.filter_text };
            std::ranges::transform(label, label.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            if (!prefix.empty() && !label.starts_with(prefix)) continue;
            items.push_back(std::move(item));
        }
        std::ranges::stable_sort(items, [](const auto& a, const auto& b) { return std::tie(a.priority, a.label) < std::tie(b.priority, b.label); });
        if (items.size() > 200) items.resize(200);
        return items;
    }

    msa::SignatureHelp signature_help_(const std::string& file, const std::string& text, Position at, msa::Cancel cancel) {
        auto unit = latest_unit_(normalize_path(file), text, cancel);
        if (!unit) return {};
        cl::CodeCompleteOptions options;
        options.IncludeBriefComments = true;
        options.LoadExternal = true;
        CompletionCollector collector { options };
        unit->with_ast([&](cl::ASTUnit* ast) {
            if (ast) run_completion(*ast, unit->path(), text, at, collector);
            return 0;
        });
        return collector.signatures;
    }

    std::vector<Location> definitions(std::string_view entity) const override { return index_.with_role(entity, msa::role::definition); }
    std::vector<Location> declarations(std::string_view entity) const override { return index_.with_role(entity, msa::role::declaration); }
    std::vector<Location> references(std::string_view entity) const override {
        return index_.with_role(entity, msa::role::reference | msa::role::declaration | msa::role::definition);
    }
    std::vector<msa::Found> find(std::string_view query, std::size_t limit) const override { return index_.find(query, limit); }

    void file_changed(const std::string& file) override {
        const std::string path { normalize_path(file) };
        modules_->file_changed(path);
        bool known { false };
        {
            std::lock_guard lock { mutex_ };
            if (commands_.contains(path)) {
                known = true;
                if (const auto text = read_file(path)) graph_.set(path, mcxx::graph::scan(*text));
                if (std::ranges::find(indexQueue_, path) == indexQueue_.end()) indexQueue_.push_back(path);
            }
        }
        if (known) {
            std::map<std::string, msa::Command, std::less<>> commands;
            mcxx::graph::Graph graph;
            {
                std::lock_guard lock { mutex_ };
                commands = commands_;
                graph = graph_;
            }
            modules_->set_program(graph, commands);
            indexCv_.notify_all();
        }
    }

private:
    struct Prepared {
        ParseRequest parse;
        std::string module;
        std::vector<msa::Diagnostic> failures;
    };

    Options options_;
    std::unique_ptr<ModuleStore> modules_;
    ProgramIndex index_;
    mutable std::mutex mutex_;
    std::map<std::string, msa::Command, std::less<>> commands_;
    mcxx::graph::Graph graph_;
    std::map<std::string, std::shared_ptr<UnitImpl>> latest_;
    std::deque<std::string> indexQueue_;
    std::size_t indexTotal_ { 0 };
    std::size_t indexDone_ { 0 };
    std::size_t indexRunning_ { 0 };
    std::condition_variable_any indexCv_;
    bool stopping_ { false };
    std::stop_source stop_;
    std::vector<std::unique_ptr<llvm::thread>> indexers_;
    std::unique_ptr<ClangPool> foreground_;

    // The command, the module interfaces and the flags one parse of `path` needs.
    std::optional<Prepared> prepare_(const std::string& path, const std::string& text, msa::Cancel cancel) {
        std::optional<msa::Command> command;
        {
            std::lock_guard lock { mutex_ };
            if (const auto it = commands_.find(path); it != commands_.end()) command = it->second;
            else command = infer_command(commands_, path);
        }
        if (!command) return std::nullopt;
        const mcxx::graph::Scan scan { mcxx::graph::scan(text) };
        std::vector<std::string> roots { scan.imports };
        // An implementation unit imports its module's interface implicitly.
        if (scan.is_module_unit() && !scan.exported && !scan.is_partition()) roots.push_back(scan.module);
        const auto result = modules_->require(roots, cancel);
        Prepared prepared;
        prepared.module = scan.module;
        prepared.parse.command = *command;
        prepared.parse.path = path;
        prepared.parse.text = text;
        prepared.parse.module_unit = scan.is_module_unit() && (scan.exported || scan.is_partition());
        prepared.parse.modules = result.pcms;
        prepared.parse.resource_directory = options_.resource_directory;
        const auto status = modules_->status();
        for (const auto& module : result.failed) {
            std::string why;
            for (const auto& [m, reason] : status.failures)
                if (m == module) why = reason;
            if (std::ranges::find(roots, module) == roots.end()) continue;
            prepared.failures.push_back({ {}, msa::Severity::error, std::format("module '{}' could not be built: {}", module, why), "module-build-failed",
                                          "Modules", {} });
        }
        return prepared;
    }

    std::shared_ptr<UnitImpl> latest_unit_(const std::string& path, const std::string& text, msa::Cancel cancel) {
        {
            std::lock_guard lock { mutex_ };
            if (const auto it = latest_.find(path); it != latest_.end()) return it->second;
        }
        auto unit = parse_(path, text, 0, cancel);
        std::lock_guard lock { mutex_ };
        const auto it = latest_.find(path);
        return it == latest_.end() ? nullptr : it->second;
    }

    void index_work_(std::stop_token stop) {
        while (true) {
            std::string file;
            {
                std::unique_lock lock { mutex_ };
                indexCv_.wait(lock, stop, [&] { return !indexQueue_.empty() || stopping_; });
                if (stop.stop_requested() || stopping_) return;
                file = indexQueue_.front();
                indexQueue_.pop_front();
                ++indexRunning_;
            }
            static const bool trace { std::getenv("MCXX_TRACE_INDEX") != nullptr };
            if (trace) options_.log(std::format("indexing {}", file));
            if (const auto text = read_file(file)) {
                if (auto prepared = prepare_(file, *text, stop)) {
                    prepared->parse.remap = false;
                    if (auto ast = parse_ast(prepared->parse)) index_unit(*ast, file, index_);
                }
            }
            {
                std::lock_guard lock { mutex_ };
                --indexRunning_;
                ++indexDone_;
            }
            options_.changed();
        }
    }
};

} // namespace mcxx::clang_backend

namespace mcxx::clang {

std::string_view version() { return mcxx::clang_backend::CLANG_VERSION; }

msa::BackendInfo info() {
    return msa::BackendInfo { "mcxx.clang", std::format("0.1.0 (clang {})", mcxx::clang_backend::CLANG_VERSION),
                              std::string { mcxx::clang_backend::CLANG_VERSION } };
}

std::unique_ptr<msa::Workspace> make_workspace(msa::Workspace::Options options) {
    return std::make_unique<mcxx::clang_backend::WorkspaceImpl>(std::move(options));
}

} // namespace mcxx::clang
