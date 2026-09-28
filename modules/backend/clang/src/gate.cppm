// mcxx.backend.clang partition :gate: MC++ feature gates inside every Clang compilation of this program, and
// [[mcpp::allow]].
module;

#include <clang/AST/ASTContext.h>
#include <clang/AST/Attr.h>
#include <clang/AST/DynamicRecursiveASTVisitor.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/StmtCXX.h>
#include <clang/Lex/MacroInfo.h>
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
#include <clang/Frontend/FrontendPluginRegistry.h>
#include <clang/Sema/ParsedAttr.h>
#include <clang/Sema/Sema.h>
#include <clang/Basic/ParsedAttrInfo.h>
#include <clang/Basic/TargetInfo.h>
#include <llvm/TargetParser/Triple.h>
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

module mcxx.backend.clang:gate;

import mcxx.msa;
import mcxx.graph;
import mcxx.base;
import mcxx.plugin;
import mcxx.features;
import :support;
import :unit;
import :facts;

namespace mcxx::clang_backend {

namespace cl = ::clang;
namespace fs = std::filesystem;
using msa::Position;
using msa::Range;
using msa::Location;

namespace {

// [[mcpp::allow("feature")]] and [[mcpp::allow("feature", "reason")]] (several features: "a, b"):
// kept on the declaration as an annotation, which :facts turns into a Suppression.
struct AllowAttrInfo final : public cl::ParsedAttrInfo {
    AllowAttrInfo() {
        NumArgs = 1;
        OptArgs = 1;
        // Clang 23's parser asks plugins about a scoped attribute by its name alone ("allow", not
        // "mcpp::allow": clang::hasAttribute passes the unscoped name to hasSpelling), and skips
        // the arguments of one no plugin claims; the attribute then arrives with none. So the bare
        // name is a spelling too; the scoped one is what the documentation and the rules use.
        static constexpr Spelling spellings[] { { cl::ParsedAttr::AS_CXX11, "mcpp::allow" }, { cl::ParsedAttr::AS_CXX11, "allow" } };
        Spellings = spellings;
    }

    bool diagAppertainsToDecl(cl::Sema&, const cl::ParsedAttr&, const cl::Decl*) const override { return true; }

    AttrHandling handleDeclAttribute(cl::Sema& sema, cl::Decl* d, const cl::ParsedAttr& attr) const override {
        std::string parts[2];
        for (unsigned i { 0 }; i < attr.getNumArgs() && i < 2; ++i) {
            const auto* literal = llvm::dyn_cast_or_null<cl::StringLiteral>(attr.getArgAsExpr(i) ? attr.getArgAsExpr(i)->IgnoreParenCasts() : nullptr);
            if (literal == nullptr) {
                const unsigned id { sema.getDiagnostics().getCustomDiagID(cl::DiagnosticsEngine::Error,
                                                                          "[[mcpp::allow]] takes a feature id and an optional reason, as string literals") };
                sema.Diag(attr.getLoc(), id);
                return AttributeNotApplied;
            }
            parts[i] = literal->getString().str();
        }
        d->addAttr(cl::AnnotateAttr::Create(sema.Context, "mcpp::allow|" + parts[0] + "|" + parts[1], nullptr, 0, attr.getRange()));
        return AttributeApplied;
    }
};

cl::ParsedAttrInfoRegistry::Add<AllowAttrInfo> allow_registration { "mcpp-allow", "[[mcpp::allow(\"feature\")]]: waives an MC++ feature gate" };

// At the end of every compilation this program runs (mcxx's own, and libmc++'s parses for the
// editor): the active rules -- MC++'s built-in ones and the plugins linked in -- over the file's
// facts, at the levels its configuration gives. The configuration's plan says what to collect: a
// file whose package gates nothing costs a manifest lookup (cached), no walk.
class GateConsumer final : public cl::ASTConsumer {
public:
    GateConsumer(cl::CompilerInstance& ci, std::vector<plugin::Finding> filtered) : ci_ { ci }, filtered_ { std::move(filtered) } {}

    void HandleTranslationUnit(cl::ASTContext& ctx) override {
        if (gates_suppressed || ci_.getDiagnostics().hasFatalErrorOccurred()) return;
        const auto& sm = ctx.getSourceManager();
        const cl::FileID main { sm.getMainFileID() };
        const std::string path { path_of(sm, main) };
        if (path.empty()) return;
        const std::shared_ptr<const features::Plan> plan { features::plan_for(path) };
        if (plan->idle()) {
            base::trace::count("gates.idle");
            return;
        }
        // A feature that needs a name the file does not declare or import (json-brace-init needs
        // nlohmann) cannot occur here: it is not asked, and its facts are not collected.
        const features::Selection selection { features::select(*plan, [&](std::string_view name) {
            return !ctx.getTranslationUnitDecl()->lookup(cl::DeclarationName { &ctx.Idents.get(llvm::StringRef { name.data(), name.size() }) }).empty();
        }) };
        if (!selection.gated && filtered_.empty() && plan->problems.empty()) {
            base::trace::count("gates.idle");
            return;
        }
        base::trace::Span span { "gates", "check", path, std::chrono::milliseconds { 500 } };
        std::string module;
        if (const cl::Module* m = ctx.getCurrentNamedModule()) module = m->getFullModuleName();
        msa::fact::Facts facts;
        if (selection.needs != msa::fact::Kinds::none) {
            base::trace::Span collect { "gates", "facts", path };
            facts = facts_of(ctx, &ci_.getPreprocessor(), selection.needs);
        }
        base::trace::Span rules { "gates", "rules", path };
        const features::Result result { features::evaluate({ path, module, facts }, *plan, selection, filtered_) };
        rules.note(std::format("{} rules", plan->catalog->rules.size()));
        auto& diags = ci_.getDiagnostics();
        for (const auto& d : result.diagnostics) {
            const unsigned id { d.severity == msa::Severity::error ? diags.getCustomDiagID(cl::DiagnosticsEngine::Error, "%0")
                                                                   : diags.getCustomDiagID(cl::DiagnosticsEngine::Warning, "%0") };
            const bool placed { d.range.begin != Position {} || d.range.end != Position {} };
            const cl::SourceLocation begin { placed ? sm.translateLineCol(main, d.range.begin.line + 1, d.range.begin.column + 1) : cl::SourceLocation {} };
            auto report = diags.Report(begin, id);
            report << d.message;
            if (placed) report << cl::CharSourceRange::getCharRange(begin, sm.translateLineCol(main, d.range.end.line + 1, d.range.end.column + 1));
        }
        for (const auto& [unknown, where] : result.unknown) {
            const unsigned id { diags.getCustomDiagID(cl::DiagnosticsEngine::Warning, "%0") };
            diags.Report(sm.translateLineCol(main, where.begin.line + 1, where.begin.column + 1), id)
                << std::format("[[mcpp::allow]] names `{}`, which no MC++ rule linked into this program declares", unknown);
        }
        span.note(std::format("{} findings, {} waived", result.diagnostics.size(), result.waived.size()));
        if (const char* audit = std::getenv("MCXX_AUDIT"); audit != nullptr && *audit != '\0') features::append_audit(audit, result.waived);
        base::trace::count("gates.waived", static_cast<std::int64_t>(result.waived.size()));
    }

private:
    cl::CompilerInstance& ci_;
    std::vector<plugin::Finding> filtered_;   // the source filters' findings (an extension's uses)
};

// The target, as a source filter reads it (plugin::Target).
plugin::Target target_of(const cl::CompilerInstance& ci) {
    const llvm::Triple& t { ci.getTarget().getTriple() };
    plugin::Target target;
    target.triple = t.str();
    target.os = t.isAndroid()        ? "android"
                : t.isOSLinux()      ? "linux"
                : t.isOSWindows()    ? "windows"
                : t.isMacOSX()       ? "macos"
                : t.isiOS()          ? "ios"
                : t.isOSFreeBSD()    ? "freebsd"
                : t.isOSWASI()       ? "wasi"
                : t.getOS() == llvm::Triple::UnknownOS ? "none"
                                     : t.getOSName().lower();
    target.family = t.isOSWindows() ? "windows"
                    : (t.isOSLinux() || t.isOSDarwin() || t.isOSFreeBSD() || t.isOSNetBSD() || t.isOSOpenBSD() || t.isAndroid()) ? "unix"
                                                                                                                                 : "";
    target.arch = llvm::Triple::getArchTypeName(t.getArch()).str();
    target.env = t.getEnvironmentName().str();
    target.pointer_width = static_cast<unsigned>(ci.getTarget().getPointerWidth(cl::LangAS::Default));
    target.endian = ci.getTarget().isBigEndian() ? "big" : "little";
    bool ndebug { false };
    for (const auto& [macro, undefined] : ci.getPreprocessorOpts().Macros) {
        const std::string_view name { std::string_view { macro }.substr(0, macro.find('=')) };
        if (name == "NDEBUG") ndebug = !undefined;
        if (!undefined && name.starts_with("MCPP_FEATURE_")) target.features.emplace_back(name.substr(std::string_view { "MCPP_FEATURE_" }.size()));
    }
    target.debug_assertions = !ndebug;
    return target;
}

// Before the main file is read: every active source filter over its text ([[mcpp::cfg]] and the
// like). The replacement has the text's length and line breaks, so every position stays. Returns
// the filters' findings, for the gates.
std::vector<plugin::Finding> filter_main_file(cl::CompilerInstance& ci) {
    if (plugin::catalog()->filters.empty()) return {};
    auto& sm = ci.getSourceManager();
    const cl::FileID main { sm.getMainFileID() };
    if (main.isInvalid()) return {};
    const auto entry = sm.getFileEntryRefForID(main);
    const auto buffer = sm.getBufferOrNone(main);
    if (!entry || !buffer) return {};
    const std::string path { path_of(sm, main) };
    const plugin::Target target { target_of(ci) };
    plugin::Filtered filtered { plugin::apply_source_filters({ path, target }, buffer->getBuffer()) };
    auto& diags = ci.getDiagnostics();
    for (const auto& p : filtered.problems) {
        const unsigned id { diags.getCustomDiagID(cl::DiagnosticsEngine::Error, "%0") };
        diags.Report(sm.translateLineCol(main, p.range.begin.line + 1, p.range.begin.column + 1), id) << p.message;
    }
    if (filtered.text) {
        base::trace::count("filters.changed");
        sm.overrideFileContents(*entry, llvm::MemoryBuffer::getMemBufferCopy(*filtered.text, buffer->getBufferIdentifier()));
    }
    return std::move(filtered.findings);
}

class GateAction final : public cl::PluginASTAction {
protected:
    // Called once the main file is known and before it is read: the moment a source filter needs.
    std::unique_ptr<cl::ASTConsumer> CreateASTConsumer(cl::CompilerInstance& ci, llvm::StringRef) override {
        return std::make_unique<GateConsumer>(ci, filter_main_file(ci));
    }
    bool ParseArgs(const cl::CompilerInstance&, const std::vector<std::string>&) override { return true; }
    ActionType getActionType() override { return AddAfterMainAction; }
};

cl::FrontendPluginRegistry::Add<GateAction> gate_registration { "mcxx-gates", "MC++ feature gates: the built-in ISO controls and the plugins linked in" };

} // namespace

} // namespace mcxx::clang_backend
