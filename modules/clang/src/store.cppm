// mcxx.clang partition :store: module interfaces (BMIs) built in dependency order, cached by content
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

module mcxx.clang:store;

import mcxx.msa;
import mcxx.graph;
import mcxx.base;
import :support;

namespace mcxx::clang_backend {

namespace cl = ::clang;
namespace fs = std::filesystem;
using msa::Position;
using msa::Range;
using msa::Location;

class ModuleStore {
public:
    struct Result {
        std::map<std::string, std::string> pcms;   // module -> interface file, dependencies first
        std::vector<std::string> failed;           // modules that could not be built (or depend on one)
        std::vector<std::string> missing;          // imported, provided by no unit
    };

    ModuleStore(std::string cacheDirectory, std::string resourceDirectory, unsigned workers, std::function<void()> changed)
        : cacheDirectory_ { std::move(cacheDirectory) }, resourceDirectory_ { std::move(resourceDirectory) },
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

    // The editor's text of an open file, read in place of the disk's; nullopt: the disk's again. An
    // interface whose text changed is stale, and so is every interface that imports it.
    void set_buffer(const std::string& path, std::optional<std::string> text) {
        std::lock_guard lock { mutex_ };
        const auto it = buffers_.find(path);
        if (text) {
            if (it != buffers_.end() && it->second == *text) return;
            buffers_.insert_or_assign(path, std::move(*text));
        } else {
            if (it == buffers_.end()) return;
            buffers_.erase(it);
        }
        std::vector<std::string> changed;
        for (const auto& module : graph_.modules())
            if (graph_.provider(module) == path) changed.push_back(module);
        if (changed.empty()) return;
        for (auto& [module, e] : entries_) {
            bool hit { std::ranges::find(changed, module) != changed.end() };
            if (!hit) {
                for (const auto& dependency : graph_.closure(graph_.requires_of(module)))
                    if (std::ranges::find(changed, dependency) != changed.end()) hit = true;
            }
            if (!hit) continue;
            if (e.state == State::ready || e.state == State::failed) e.state = State::stale;
            else if (e.state == State::building) e.again = true;
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
                s.failures.push_back({ module, e.error, e.cause.empty() ? module : e.cause, e.command });
                if (e.command) {
                    ++s.commands_rejected;
                    if (s.rejected.size() < 5) s.rejected.push_back({ e.source, e.error });
                }
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
        std::string cause;                 // failed: the module whose own build failed
        bool command { false };            // failed: the compile command was rejected
        bool again { false };              // building: its input changed meanwhile, build it again
        std::string source;
        std::vector<std::string> inputs;   // files the last build read
    };

    std::string cacheDirectory_;
    std::string resourceDirectory_;
    std::function<void()> changed_;
    mutable std::mutex mutex_;
    std::condition_variable_any cv_;
    mcxx::graph::Graph graph_;
    std::map<std::string, msa::Command, std::less<>> commands_;
    std::map<std::string, Entry, std::less<>> entries_;
    std::map<std::string, std::string, std::less<>> buffers_;   // path -> the editor's text
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
        base::trace::Span span { "modules", "build", module };
        // An interface built for the editor: its own diagnostics (and the gates') are the file's, not
        // a reason to fail every importer.
        const struct Quiet {
            Quiet() { gates_suppressed = true; }
            ~Quiet() { gates_suppressed = false; }
        } quiet;
        std::string file;
        msa::Command command;
        std::vector<std::pair<std::string, std::string>> dependencies;   // module, pcm (transitive)
        std::string dependencyKeys;
        bool dependencyFailed { false };
        std::string failedDependency;
        std::optional<std::string> buffer;
        {
            std::lock_guard lock { mutex_ };
            file = graph_.provider(module);
            if (const auto it = buffers_.find(file); it != buffers_.end()) buffer = it->second;
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
        bool rejected { false };
        auto finish = [&](bool ok, std::string pcm, std::string key, std::string error, std::vector<std::string> inputs) {
            std::lock_guard lock { mutex_ };
            auto& e = entries_[module];
            e.state = ok ? State::ready : State::failed;
            e.pcm = std::move(pcm);
            e.key = std::move(key);
            e.error = std::move(error);
            e.cause.clear();
            e.command = rejected;
            if (!ok) {
                const auto d = dependencyFailed ? entries_.find(failedDependency) : entries_.end();
                e.cause = d == entries_.end() ? module : (d->second.cause.empty() ? failedDependency : d->second.cause);
            }
            e.source = file;
            e.inputs = std::move(inputs);
            if (e.again) {
                e.again = false;
                e.state = State::queued;
                waiting_.push_back(module);
            }
        };
        if (dependencyFailed) {
            finish(false, {}, {}, "depends on " + failedDependency + ", which failed", {});
            base::trace::count("modules.failed");
            base::trace::info("modules", "Failed to build module {}; due to its dependency {} failing", module, failedDependency);
            return;
        }
        if (command.arguments.empty()) {
            finish(false, {}, {}, "no compile command for " + file, {});
            return;
        }
        const auto source = buffer ? buffer : read_file(file);
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
                base::trace::count("modules.reused");
                base::trace::debug("modules", "Reusing persistent module {} from {}", module, pcm);
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
            rejected = true;
            const std::string reason { consumer.errors.empty() ? std::string { "the driver gave no reason" } : consumer.errors.front() };
            finish(false, {}, key, reason, {});
            base::trace::count("modules.failed");
            base::trace::warning("modules", "Failed to build module {}; the compile command of {} was rejected: {}", module, file, reason);
            return;
        }
        invocation->getFrontendOpts().OutputFile = tmp;
        // A library does not write to standard error: no "N errors generated." after a failed build.
        invocation->getDiagnosticOpts().ShowCarets = false;
        if (buffer) invocation->getPreprocessorOpts().addRemappedFile(file, llvm::MemoryBuffer::getMemBufferCopy(*buffer, file).release());
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
            base::trace::count("modules.failed");
            base::trace::info("modules", "Failed to build module {}; due to Failed to compile {}: {}", module, file, reason);
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
        base::trace::count("modules.built");
        base::trace::debug("modules", "Built module {} to {} in {:.2f} s", module, pcm, seconds);
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

} // namespace mcxx::clang_backend

