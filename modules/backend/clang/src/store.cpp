// mcxx.backend.clang implementation unit: the definitions of :store (module interfaces, built in dependency order).
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
#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <filesystem>
#include <format>
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

module mcxx.backend.clang;

import mcxx.msa;
import mcxx.graph;
import mcxx.base;
import :support;
import :ifc;
import :store;

namespace mcxx::clang_backend {

namespace cl = ::clang;
namespace fs = std::filesystem;

namespace {

// The options a BMI and every unit importing it must agree on that a build description may leave out
// of one command and give the rest (Clang: "ExceptionHandling differs in precompiled file", "was
// compiled for the target 'arm64-apple-macosx15.0.0' but the current translation unit is being
// compiled for target 'arm64-apple-macosx14.0.0'"). Each kind: whether an argument names it.
constexpr std::array<bool (*)(std::string_view), 3> AGREED {
    [](std::string_view a) { return a == "-fdwarf-exceptions" || a == "-fseh-exceptions" || a == "-fsjlj-exceptions" || a == "-fwasm-exceptions"; },
    [](std::string_view a) { return a.starts_with("--target") || a == "-target"; },
    [](std::string_view a) { return a.starts_with("-mmacosx-version-min=") || a.starts_with("-mmacos-version-min="); },
};

} // namespace

void ModuleStore::set_program(const mcxx::graph::Graph& graph, const std::map<std::string, msa::Command, std::less<>>& commands) {
    std::lock_guard lock { mutex_ };
    graph_ = graph;
    commands_ = commands;
    // Of each kind in AGREED, the one argument most of the program's commands give, when one does: a
    // module whose own command names none of that kind is built with it. mcpp's std for openkal's
    // Windows runtime had no `-fdwarf-exceptions` and every unit of the program had it; on macOS std
    // had no deployment target and its importers 14.0: every import of std failed.
    agreed_.clear();
    for (const auto& is : AGREED) {
        std::map<std::string, std::size_t> given;
        for (const auto& [file, command] : commands_)
            for (const auto& a : command.arguments)
                if (is(a) && a != "-target" && a != "--target") ++given[a];   // a two-word form's value is the next one
        for (const auto& [flag, count] : given)
            if (2 * count > commands_.size()) agreed_.push_back(flag);
    }
    for (auto& [module, entry] : entries_) {
        if (entry.state == State::ready || entry.state == State::failed) entry.state = State::stale;
        // One being built now reads the program as it was: built again once it is done, never
        // taken as ready (a std built for the old commands' -std would reach the new importers).
        else if (entry.state == State::building) entry.again = true;
    }
}

void ModuleStore::set_buffer(const std::string& path, std::optional<std::string> text) {
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
    invalidate_(changed);
}

void ModuleStore::file_changed(const std::string& path) {
    std::lock_guard lock { mutex_ };
    std::vector<std::string> changed;
    for (const auto& [module, entry] : entries_)
        if (entry.source == path || std::ranges::find(entry.inputs, path) != entry.inputs.end()) changed.push_back(module);
    invalidate_(changed);
}

void ModuleStore::invalidate_(const std::vector<std::string>& changed) {
    if (changed.empty()) return;
    for (auto& [module, e] : entries_) {
        bool hit { std::ranges::find(changed, module) != changed.end() };
        if (!hit) {
            for (const auto& dependency : graph_.closure(graph_.requires_of(module)))
                if (std::ranges::find(changed, dependency) != changed.end()) hit = true;
        }
        if (!hit) continue;
        if (e.state == State::ready || e.state == State::failed) e.state = State::stale;
        else if (e.state == State::building) e.again = true;   // it may have read the file before it changed
    }
}

ModuleStore::Result ModuleStore::require(const std::vector<std::string>& roots, std::stop_token cancel) {
    Result result;
    std::unique_lock lock { mutex_ };
    const std::vector<std::string> order { graph_.closure(roots, &result.missing) };
    for (const auto& module : order) schedule_(module);
    dispatch_();
    const auto since { std::chrono::steady_clock::now() };
    // What it waits for may change under it: a file changed on disk (file_changed: set_program, and
    // nothing scheduled) leaves a module it had ready stale, and a module the program no longer
    // provides (renamed, its unit gone) is dropped -- either way never ready or failed again, and the
    // wait never ended (win32-x64, verify-changes: a module renamed while cxx_verify waited, 120 s).
    // So a stale or unknown one is scheduled again, and one no longer provided is missing, not awaited.
    const auto gone = [&](const std::string& m) { return graph_.provider(m).empty(); };
    const auto settled = [&](const std::string& m) {
        const auto& e = entries_[m];
        return gone(m) || e.state == State::ready || e.state == State::failed;
    };
    const auto idle = [&](const std::string& m) {
        const auto& e = entries_[m];
        return !gone(m) && (e.state == State::unknown || e.state == State::stale);
    };
    // Woken by the cancel token too, not only by the next module done: a request given up on (an
    // older version of the text) let go of its thread while the store is still busy.
    while (true) {
        if (stopping_ || cancel.stop_requested() || std::ranges::all_of(order, settled)) break;
        if (std::ranges::any_of(order, idle)) {
            for (const auto& module : order)
                if (idle(module)) schedule_(module);
            dispatch_();
            continue;
        }
        if (cv_.wait_for(lock, cancel, std::chrono::seconds { 10 },
                         [&] { return stopping_ || std::ranges::all_of(order, settled) || std::ranges::any_of(order, idle); }))
            continue;
        if (cancel.stop_requested()) break;
        // A wait that does not end: what it waits for, and in which state each is.
        if (!base::trace::enabled("modules", base::trace::Level::debug)) continue;
        static constexpr std::array names { "unknown", "stale", "queued", "building", "ready", "failed" };
        std::string pending;
        for (const auto& module : order) {
            const auto& e = entries_[module];
            if (!settled(module))
                pending += std::format(" {} ({}{})", module, names[static_cast<std::size_t>(e.state)], e.again ? ", again" : "");
        }
        const auto seconds { std::chrono::duration<double>(std::chrono::steady_clock::now() - since).count() };
        base::trace::debug("modules", "Waiting {:.0f} s for{}; {} waiting, {} ready to build", seconds, pending, waiting_.size(), ready_.size());
    }
    for (const auto& module : order) {
        const auto& e = entries_[module];
        if (gone(module)) result.missing.push_back(module);
        else if (e.state == State::ready) result.pcms.emplace(module, e.pcm);
        else result.failed.push_back(module);
    }
    return result;
}

void ModuleStore::prepare_all() {
    std::unique_lock lock { mutex_ };
    const std::vector<std::string> all { graph_.modules() };
    for (const auto& module : graph_.closure(all)) schedule_(module);
    dispatch_();
}

msa::Status ModuleStore::status() const {
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

std::string ModuleStore::pcm_of(const std::string& module) const {
    std::lock_guard lock { mutex_ };
    const auto it = entries_.find(module);
    return it != entries_.end() && it->second.state == State::ready ? it->second.pcm : std::string {};
}

void ModuleStore::schedule_(const std::string& module) {
    auto& e = entries_[module];
    if (e.state == State::unknown || e.state == State::stale) {
        e.state = State::queued;
        waiting_.push_back(module);
    }
}

void ModuleStore::dispatch_() {
    bool moved { false };
    // By position, not iterator: schedule_ appends a dependency to waiting_ while it is walked, and an
    // append that grows the vector leaves an iterator dangling (a program change, whose importers
    // waiting here gain dependencies the store has not seen: the queue lost modules, which then
    // stayed queued for ever, and macOS crashed).
    for (std::size_t i { 0 }; i < waiting_.size();) {
        const std::string module { waiting_[i] };
        bool done { true };
        for (const auto& dependency : graph_.requires_of(module)) {
            if (graph_.provider(dependency).empty()) continue;
            const auto& d = entries_[dependency];
            if (d.state != State::ready && d.state != State::failed) {
                if (d.state == State::unknown || d.state == State::stale) schedule_(dependency);
                done = false;
            }
        }
        if (done) {
            ready_.push_back(module);
            waiting_.erase(waiting_.begin() + static_cast<std::ptrdiff_t>(i));
            moved = true;
        } else {
            ++i;
        }
    }
    if (moved) cv_.notify_all();
}

void ModuleStore::work_(std::stop_token stop) {
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

void ModuleStore::build_(const std::string& module) {
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
    std::vector<std::string> agreed;
    {
        std::lock_guard lock { mutex_ };
        file = graph_.provider(module);
        // Queued for a program that had it, and the program now has not (the inferred plan's units
        // the build tool does not describe, xlings' apps/gui): nothing to build, and no failure.
        if (file.empty()) {
            auto& e = entries_[module];
            e.again = false;
            e.state = State::unknown;
            return;
        }
        if (const auto it = buffers_.find(file); it != buffers_.end()) buffer = it->second;
        agreed = agreed_;
        const auto it = commands_.find(file);
        if (it != commands_.end()) command = it->second;
        const std::vector<std::string> roots { graph_.requires_of(module) };
        bool dependencyPending { false };
        for (const auto& dependency : graph_.closure(roots)) {
            const auto& d = entries_[dependency];
            if (d.state == State::ready) {
                dependencies.emplace_back(dependency, d.pcm);
                dependencyKeys += dependency + "=" + d.key + ";";
            } else if (d.state == State::failed) {
                dependencyFailed = true;
                failedDependency = dependency;
            } else {
                dependencyPending = true;
                schedule_(dependency);
            }
        }
        // Dispatched when its dependencies were done, and one is not any more: the program changed
        // under it (set_program, an edit) before a worker took it. It waits for that one to be built
        // again rather than fail on it, which nothing would undo (real-xlings in CI: what the
        // inferred plan's failures had dispatched failed on the build tool's program, and stayed so).
        if (dependencyPending) {
            auto& e = entries_[module];
            e.again = false;
            e.state = State::queued;
            waiting_.push_back(module);
            return;
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
    for (const auto& flag : agreed) {
        const auto kind = std::ranges::find_if(AGREED, [&](const auto& is) { return is(flag); });
        if (kind != AGREED.end() && std::ranges::none_of(args, [&](const std::string& a) { return (*kind)(a); })) args.push_back(flag);
    }
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
    // Built under a name of its own and renamed: two processes sharing the cache may build the same
    // interface at once, and a name from the thread's id alone was the same in both.
    const std::string tmp { pcm + ".building" + unique_suffix() };
    args.insert(args.end(), { "-x", "c++-module", "--precompile", file, "-o", tmp });
    if (base::trace::enabled("modules", base::trace::Level::debug)) {
        std::string joined;
        for (const auto& a : args) joined += " " + a;
        base::trace::debug("modules", "Building module {} from {} with {}'s command in {}:{}", module, file, command.file, command.directory, joined);
    }

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
    // The interface beside the BMI (MC2), for an importer's parse to read: written to the BMI's
    // final name, which the BMI takes once it is built.
    InterfaceAction action { std::make_unique<cl::GenerateReducedModuleInterfaceAction>(), file, pcm };
    const auto started = std::chrono::steady_clock::now();
    const bool ok { instance.ExecuteAction(action) && !instance.getDiagnostics().hasErrorOccurred() };
    const auto seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count();
    // What the build read, failed or not: a failure in a header is mended by changing the header.
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
    if (!ok) {
        std::error_code ec;
        fs::remove(tmp, ec);
        std::string reason { consumer.errors.empty() ? std::string { "errors" } : consumer.errors.front() };
        finish(false, {}, key, reason, std::move(inputs));
        base::trace::count("modules.failed");
        std::string noted;
        for (const auto& n : consumer.notes) noted += "; note: " + n;
        base::trace::info("modules", "Failed to build module {}; due to Failed to compile {}: {}{}", module, file, reason, noted);
        return;
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

std::optional<std::vector<std::string>> ModuleStore::valid_inputs_(const std::string& manifest) {
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

} // namespace mcxx::clang_backend
