// mcxx.backend.clang implementation unit: msa::Workspace over the partitions
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
#include <clang/Basic/LangStandard.h>
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
#include <llvm/TargetParser/Triple.h>

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

module mcxx.backend.clang;

import mcxx.msa;
import mcxx.graph;
import mcxx.base;
import :support;
import :store;
import :unit;
import :completion;
import :index;
import :facts;
import :gate;

namespace mcxx::clang_backend {

namespace cl = ::clang;
namespace fs = std::filesystem;
using msa::Position;
using msa::Range;
using msa::Location;

// ============================================================================================
// 6. workspace
// ============================================================================================

class WorkspaceImpl final : public msa::Workspace {
public:
    explicit WorkspaceImpl(Options options) : options_ { std::move(options) } {
        if (!options_.changed) options_.changed = [] {};
        if (options_.workers == 0) options_.workers = std::max(1u, std::thread::hardware_concurrency() / 4);
        // The backend's lines go to the host's sink; which are produced is MCXX_LOG's to say, and
        // failures (info and above) always are.
        {
            base::trace::Config config { base::trace::configuration() };
            if (config.level > base::trace::Level::info) config.level = base::trace::Level::info;
            base::trace::configure(std::move(config));
            base::trace::configure_from_environment();
        }
        if (options_.log) {
            base::trace::set_sink([log = options_.log](base::trace::Level level, std::string_view category, std::string_view message) {
                const msa::LogLevel mapped { level == base::trace::Level::debug    ? msa::LogLevel::debug
                                             : level == base::trace::Level::info    ? msa::LogLevel::info
                                             : level == base::trace::Level::warning ? msa::LogLevel::warning
                                                                                    : msa::LogLevel::error };
                log(mapped, category, message);
            });
        }
        modules_ = std::make_unique<ModuleStore>(options_.cache_directory, options_.resource_directory, options_.workers, options_.changed);
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
        base::trace::Span span { "workspace", "set_commands", std::format("{} commands", commands.size()) };
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
            // New commands: whatever was rejected may be accepted now; what is not says so again.
            rejectedCount_ = 0;
            rejected_.clear();
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
        s.counters = base::trace::counters();
        std::lock_guard lock { mutex_ };
        s.commands_rejected += rejectedCount_;
        for (const auto& r : rejected_)
            if (s.rejected.size() < 5) s.rejected.push_back(r);
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
        base::trace::Span span { "parse", "parse", std::format("{} v{}", path, version) };
        modules_->set_buffer(path, text);
        auto request = prepare_(path, text, cancel);
        if (!request) return nullptr;
        request->parse.remap = true;
        std::vector<msa::Diagnostic> extra { std::move(request->failures) };
        std::string rejected;
        auto ast = parse_ast(request->parse, &rejected);
        if (cancel.stop_requested()) return nullptr;
        if (!ast && !rejected.empty()) {
            extra.push_back({ {}, msa::Severity::error, "the compile command was rejected: " + rejected, "command-rejected", "Command", {} });
            note_rejected_(path, rejected);
        }
        auto unit = std::make_shared<UnitImpl>(std::move(ast), path, std::move(request->parse.text), version, request->module, std::move(extra),
                                               foreground_.get());
        {
            std::lock_guard lock { mutex_ };
            latest_[path] = unit;
        }
        return unit;
    }

    std::vector<msa::CompletionItem> complete_(const std::string& file, const std::string& text, Position at, msa::Cancel cancel) {
        base::trace::Span span { "complete", "complete", std::format("{}:{}:{}", file, at.line + 1, at.column + 1), std::chrono::milliseconds { 500 } };
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
        base::trace::Span span { "complete", "signature", std::format("{}:{}:{}", file, at.line + 1, at.column + 1), std::chrono::milliseconds { 500 } };
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

    void close(const std::string& file) override {
        const std::string path { normalize_path(file) };
        modules_->set_buffer(path, std::nullopt);
        std::lock_guard lock { mutex_ };
        latest_.erase(path);
    }

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
    std::size_t rejectedCount_ { 0 };
    std::vector<msa::RejectedCommand> rejected_;   // the first few
    std::deque<std::string> indexQueue_;
    std::size_t indexTotal_ { 0 };
    std::size_t indexDone_ { 0 };
    std::size_t indexRunning_ { 0 };
    std::condition_variable_any indexCv_;
    bool stopping_ { false };
    std::stop_source stop_;
    std::vector<std::unique_ptr<llvm::thread>> indexers_;
    std::unique_ptr<ClangPool> foreground_;

    void note_rejected_(const std::string& path, const std::string& reason) {
        {
            std::lock_guard lock { mutex_ };
            ++rejectedCount_;
            if (rejected_.size() < 5 && std::ranges::none_of(rejected_, [&](const msa::RejectedCommand& r) { return r.file == path; }))
                rejected_.push_back({ path, reason });
        }
        if (options_.changed) options_.changed();
    }

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
        auto position_of = [&](std::size_t offset) {
            Position p;
            for (std::size_t i { 0 }; i < offset && i < text.size(); ++i) {
                if (text[i] == '\n') {
                    ++p.line;
                    p.column = 0;
                } else {
                    ++p.column;
                }
            }
            return p;
        };
        for (const auto& module : result.failed) {
            const auto root = std::ranges::find(roots, module);
            if (root == roots.end()) continue;
            const msa::ModuleFailure* failure { nullptr };
            const msa::ModuleFailure* cause { nullptr };
            for (const auto& f : status.failures) {
                if (f.module == module) failure = &f;
            }
            if (failure != nullptr) {
                for (const auto& f : status.failures) {
                    if (f.module == failure->cause) cause = &f;
                }
            }
            // One diagnostic on the import (or on the declaration, for an implementation unit's own
            // module), naming the module whose build failed.
            std::pair<std::size_t, std::size_t> span { scan.module_span };
            if (const auto at = static_cast<std::size_t>(root - roots.begin()); at < scan.import_spans.size()) span = scan.import_spans[at];
            std::string message;
            if (failure == nullptr) message = std::format("module {} could not be built", module);
            else if (cause == nullptr || failure->cause == module) message = std::format("module {} did not compile: {}", module, failure->reason);
            else message = std::format("module {} cannot be built because {} did not compile: {}", module, cause->module, cause->reason);
            prepared.failures.push_back({ { position_of(span.first), position_of(span.second) }, msa::Severity::error, std::move(message), "module-failed",
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
            base::trace::Span span { "index", "unit", file };
            const struct Quiet {
                Quiet() { gates_suppressed = true; }
                ~Quiet() { gates_suppressed = false; }
            } quiet;   // the index does not report; the gates run where a file is parsed for its diagnostics
            if (const auto text = read_file(file)) {
                if (auto prepared = prepare_(file, *text, stop)) {
                    prepared->parse.remap = false;
                    std::string rejected;
                    if (auto ast = parse_ast(prepared->parse, &rejected)) index_unit(*ast, file, index_);
                    else if (!rejected.empty()) note_rejected_(file, rejected);
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

namespace mcxx::backend::clang {

std::string_view version() { return mcxx::clang_backend::CLANG_VERSION; }

msa::BackendInfo info() {
    return msa::BackendInfo { "mcxx.backend.clang", std::format("0.1.0 (clang {})", mcxx::clang_backend::CLANG_VERSION),
                              std::string { mcxx::clang_backend::CLANG_VERSION } };
}

std::unique_ptr<msa::Workspace> make_workspace(msa::Workspace::Options options) {
    return std::make_unique<mcxx::clang_backend::WorkspaceImpl>(std::move(options));
}


std::vector<std::string> derived_arguments(const msa::Command& command) {
    msa::Command absolute { command };
    if (!absolute.file.empty() && std::filesystem::path { absolute.file }.is_relative())
        absolute.file = clang_backend::normalize_path(absolute.file, absolute.directory);
    return clang_backend::normalize(absolute);
}

std::map<std::string, std::int64_t> census(const msa::Unit& unit) {
    const auto* impl = dynamic_cast<const clang_backend::UnitImpl*>(&unit);
    return impl != nullptr ? impl->census() : std::map<std::string, std::int64_t> {};
}

std::vector<RawToken> raw_tokens(std::string_view text) {
    ::clang::LangOptions language;
    std::vector<std::string> includes;
    ::clang::LangOptions::setLangDefaults(language, ::clang::Language::CXX, llvm::Triple { "x86_64-unknown-linux-gnu" }, includes,
                                          ::clang::LangStandard::lang_cxx23);
    // A buffer Clang may read one past: the lexer wants a NUL at the end.
    const std::string buffer { text };
    const auto start = ::clang::SourceLocation::getFromRawEncoding(1);
    ::clang::Lexer lexer { start, language, buffer.data(), buffer.data(), buffer.data() + buffer.size() };
    // As -dump-raw-tokens lexes: whitespace kept, so what is not a token (an unterminated comment)
    // comes back as one, as there; whitespace itself is dropped below.
    lexer.SetKeepWhitespaceMode(true);
    std::vector<std::uint32_t> lines { 0 };   // the offset each line starts at
    for (std::size_t i { 0 }; i < buffer.size(); ++i)
        if (buffer[i] == '\n' || (buffer[i] == '\r' && (i + 1 == buffer.size() || buffer[i + 1] != '\n'))) lines.push_back(static_cast<std::uint32_t>(i + 1));
    std::vector<RawToken> out;
    ::clang::Token token;
    for (lexer.LexFromRawLexer(token); token.isNot(::clang::tok::eof); lexer.LexFromRawLexer(token)) {
        const auto begin = token.getLocation().getRawEncoding() - start.getRawEncoding();
        if (token.is(::clang::tok::unknown)) {
            // Whitespace (splices and NUL bytes among it) is an unknown token in this mode.
            const std::string_view spelled { buffer.data() + begin, token.getLength() };
            const auto space = [](char c) { return c == ' ' || c == '\t' || c == '\v' || c == '\f' || c == '\n' || c == '\r' || c == '\0'; };
            bool blank { false };
            for (std::size_t i { 0 }; i < spelled.size(); ++i) {
                if (spelled[i] == '\\') {   // a splice: a backslash, blanks, a line break
                    std::size_t k { i + 1 };
                    while (k < spelled.size() && (spelled[k] == ' ' || spelled[k] == '\t' || spelled[k] == '\v' || spelled[k] == '\f')) ++k;
                    if (k == spelled.size() || (spelled[k] != '\n' && spelled[k] != '\r')) {
                        blank = false;
                        break;
                    }
                    i = k;
                    continue;
                }
                blank = space(spelled[i]);
                if (!blank) break;
            }
            if (blank) continue;
        }
        const auto line = static_cast<std::uint32_t>(std::ranges::upper_bound(lines, begin) - lines.begin());
        out.push_back({ .kind = ::clang::tok::getTokenName(token.getKind()),
                        .begin = begin,
                        .end = begin + token.getLength(),
                        .line = line,
                        .column = begin - lines[line - 1] + 1 });
    }
    return out;
}

std::optional<msa::Command> inferred_command(std::span<const msa::Command> commands, std::string_view file) {
    std::map<std::string, msa::Command, std::less<>> byFile;
    for (const auto& c : commands) byFile.emplace(c.file, c);
    return clang_backend::infer_command(byFile, std::string { file });
}

} // namespace mcxx::backend::clang

