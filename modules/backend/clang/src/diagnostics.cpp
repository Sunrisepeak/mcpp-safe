// The view's consumer: Clang's diagnostics as mcxx.diagnostics Items (MC5 §9).
module;

#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/DiagnosticIDs.h>
#include <clang/Basic/DiagnosticOptions.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Lex/Lexer.h>
#include <llvm/ADT/SmallString.h>
#include <llvm/Support/raw_ostream.h>

module mcxx.backend.clang;

import std;
import mcxx.msa;
import mcxx.diagnostics;
import :support;
import :unit;
import :diagnostics;

namespace mcxx::clang_backend {

namespace {

namespace cl = ::clang;
namespace view = ::mcxx::diagnostics;

// The findings the gates told, by the place and text they were reported with.
std::map<std::pair<std::uint64_t, std::string>, msa::Diagnostic>& told() {
    thread_local std::map<std::pair<std::uint64_t, std::string>, msa::Diagnostic> findings;
    return findings;
}

std::uint64_t key_of(cl::SourceLocation at) { return at.getRawEncoding(); }

msa::Severity severity_of_level(cl::DiagnosticsEngine::Level level) {
    switch (level) {
    case cl::DiagnosticsEngine::Error:
    case cl::DiagnosticsEngine::Fatal: return msa::Severity::error;
    case cl::DiagnosticsEngine::Warning: return msa::Severity::warning;
    case cl::DiagnosticsEngine::Note: return msa::Severity::information;
    default: return msa::Severity::hint;
    }
}

class ViewConsumer final : public cl::DiagnosticConsumer {
public:
    ViewConsumer(view::View v, bool color) : view_ { v }, color_ { color } {}
    ~ViewConsumer() override { flush(); }

    void HandleDiagnostic(cl::DiagnosticsEngine::Level level, const cl::Diagnostic& info) override {
        cl::DiagnosticConsumer::HandleDiagnostic(level, info);   // the counts a compile's status comes from
        view::Item item { place(info) };
        llvm::SmallString<256> text;
        info.FormatDiagnostic(text);
        item.diagnostic.message = std::string { text.str() };
        item.diagnostic.severity = severity_of_level(level);
        if (info.getID() < cl::diag::DIAG_UPPER_LIMIT) item.diagnostic.code = diagnostic_code(info.getID());
        if (auto found = told().find({ key_of(info.getLocation()), item.diagnostic.message }); found != told().end()) {
            // A gate's finding: what it says apart, its range as the gate gave it.
            const msa::Range range { item.diagnostic.range };
            item.diagnostic = std::move(found->second);
            if (item.diagnostic.range == msa::Range {}) item.diagnostic.range = range;
            told().erase(found);
        }
        for (const auto& hint : info.getFixItHints()) {
            if (!info.hasSourceManager()) break;
            const auto& sm = info.getSourceManager();
            const auto begin = sm.getPresumedLoc(sm.getFileLoc(hint.RemoveRange.getBegin()));
            const auto end = sm.getPresumedLoc(sm.getFileLoc(hint.RemoveRange.getEnd()));
            if (begin.isInvalid() || end.isInvalid()) continue;
            item.fixits.push_back({ { { begin.getLine() - 1, begin.getColumn() - 1 }, { end.getLine() - 1, end.getColumn() - 1 } }, hint.CodeToInsert });
        }
        if (level == cl::DiagnosticsEngine::Note && pending_) {
            pending_->notes.push_back(std::move(item));
            return;
        }
        flush();
        pending_ = std::move(item);
    }

    void EndSourceFile() override { flush(); }

private:
    view::View view_;
    bool color_;
    std::optional<view::Item> pending_;

    void flush() {
        if (!pending_) return;
        llvm::errs() << view::render(*pending_, view_, color_);
        llvm::errs().flush();
        pending_.reset();
    }

    // Where a diagnostic is: its file, its range (a token's end measured), the lines it covers.
    static view::Item place(const cl::Diagnostic& info) {
        view::Item item;
        if (!info.hasSourceManager() || info.getLocation().isInvalid()) return item;
        const auto& sm = info.getSourceManager();
        const cl::SourceLocation at { sm.getFileLoc(info.getLocation()) };
        const auto begin = sm.getPresumedLoc(at);
        if (begin.isInvalid()) return item;
        item.path = begin.getFilename();
        msa::Range range { { begin.getLine() - 1, begin.getColumn() - 1 }, { begin.getLine() - 1, begin.getColumn() } };
        for (const auto& r : info.getRanges()) {
            cl::SourceLocation end { sm.getFileLoc(r.getEnd()) };
            if (r.isTokenRange()) end = cl::Lexer::getLocForEndOfToken(end, 0, sm, cl::LangOptions {});
            const auto e = sm.getPresumedLoc(end);
            const auto b = sm.getPresumedLoc(sm.getFileLoc(r.getBegin()));
            if (e.isInvalid() || b.isInvalid() || b.getFileID() != begin.getFileID()) continue;
            if (b.getLine() == begin.getLine() && b.getColumn() == begin.getColumn() && (e.getLine() > begin.getLine() || e.getColumn() > begin.getColumn()))
                range.end = { e.getLine() - 1, e.getColumn() - 1 };
            break;
        }
        item.diagnostic.range = range;
        bool invalid { false };
        const llvm::StringRef buffer { sm.getBufferData(sm.getFileID(at), &invalid) };
        if (!invalid) {
            std::size_t line { 0 }, from { 0 };
            const std::string_view text { buffer.data(), buffer.size() };
            while (line < range.begin.line && from != std::string_view::npos) {
                from = text.find('\n', from);
                if (from != std::string_view::npos) ++from;
                ++line;
            }
            for (std::uint32_t l { range.begin.line }; l <= range.end.line && from != std::string_view::npos && from <= text.size(); ++l) {
                const std::size_t to { text.find('\n', from) };
                item.lines.emplace_back(text.substr(from, to == std::string_view::npos ? std::string_view::npos : to - from));
                from = to == std::string_view::npos ? to : to + 1;
            }
        }
        return item;
    }
};

} // namespace

void remember_finding(cl::SourceLocation at, const std::string& message, const msa::Diagnostic& finding) { told()[{ key_of(at), message }] = finding; }

void install_view(cl::CompilerInstance& ci) {
    if (on_clang_stack) return;
    const view::View v { view::view_for(std::nullopt, view::stderr_is_terminal()) };
    if (v == view::View::clang) return;
    // One consumer per compile: a second action on the same instance keeps it.
    if (dynamic_cast<ViewConsumer*>(&ci.getDiagnosticClient()) != nullptr) return;
    const bool color { view::colored(v) };
    // The agent view is JSON lines alone: no "N errors generated." after them.
    if (v == view::View::agent) ci.getDiagnosticOpts().ShowCarets = false;
    ci.getDiagnostics().setClient(new ViewConsumer { v, color }, /*ShouldOwnClient=*/true);
}

} // namespace mcxx::clang_backend
