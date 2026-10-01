// mcxx.backend.clang implementation unit: the definitions of :gate (MC++ feature gates inside every Clang
// compilation of this program, and [[mcpp::allow]]).
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
#include <llvm/TargetParser/Host.h>
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

module mcxx.backend.clang;

import mcxx.msa;
import mcxx.graph;
import mcxx.base;
import mcxx.plugin;
import mcxx.features;
import mcxx.plugin.host;
import mcxx.frontend;
import :support;
import :unit;
import :facts;
import :ifc;
import :diagnostics;
import :gate;

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

// The attributes providers claim (MC4 §2: `[[acme::hot]]`, a region `[[acme::device]]`): accepted on
// a declaration, and kept as an annotation -- `mcpp::attr|name|arg<US>arg...` -- that :facts turns
// into a fact::Attribute. Clang makes one instance of each attribute plugin the first time it looks
// for one; the spellings are the catalog's then, and follow it when it changes (refresh()).
class ClaimedAttrInfo final : public cl::ParsedAttrInfo {
public:
    ClaimedAttrInfo() {
        NumArgs = 0;
        OptArgs = 15;
        instance() = this;
        refresh();
    }

    static ClaimedAttrInfo*& instance() {
        static ClaimedAttrInfo* self { nullptr };
        return self;
    }

    // The catalog's attributes as spellings, scoped and (Clang 23's parser asks by the unscoped name
    // alone, as for mcpp::allow) unscoped.
    void refresh() {
        const auto catalog = plugin::catalog();
        if (catalog->generation == generation_) return;
        generation_ = catalog->generation;
        names_.clear();
        for (const auto& a : catalog->attributes) {
            names_.push_back(a.attribute->name);
            names_.push_back(a.attribute->name.substr(a.attribute->name.rfind(':') + 1));
        }
        spellings_.clear();
        for (const auto& n : names_) spellings_.push_back({ cl::ParsedAttr::AS_CXX11, n.c_str() });
        Spellings = spellings_;
    }

    bool diagAppertainsToDecl(cl::Sema&, const cl::ParsedAttr&, const cl::Decl*) const override { return true; }

    AttrHandling handleDeclAttribute(cl::Sema& sema, cl::Decl* d, const cl::ParsedAttr& attr) const override {
        // The claimed name: scoped as written, or the one claimed attribute with this unscoped name.
        std::string name { attr.getAttrName() ? attr.getAttrName()->getName().str() : std::string {} };
        if (attr.getScopeName() != nullptr) name = attr.getScopeName()->getName().str() + "::" + name;
        else
            for (const auto& n : names_)
                if (n.ends_with("::" + name)) {
                    name = n;
                    break;
                }
        std::string text { "mcpp::attr|" + name + "|" };
        const auto& sm = sema.getSourceManager();
        for (unsigned i { 0 }; i < attr.getNumArgs(); ++i) {
            if (i != 0) text += '\x1f';
            const cl::Expr* e { attr.getArgAsExpr(i) };
            if (e == nullptr) continue;
            if (const auto* s = llvm::dyn_cast<cl::StringLiteral>(e->IgnoreParenCasts())) text += s->getString().str();
            else text += cl::Lexer::getSourceText(cl::CharSourceRange::getTokenRange(e->getSourceRange()), sm, sema.getLangOpts()).str();
        }
        d->addAttr(cl::AnnotateAttr::Create(sema.Context, text, nullptr, 0, attr.getRange()));
        return AttributeApplied;
    }

private:
    std::uint64_t generation_ { 0 };
    std::vector<std::string> names_;
    std::vector<Spelling> spellings_;
};

cl::ParsedAttrInfoRegistry::Add<ClaimedAttrInfo> claimed_registration { "mcxx-claimed", "the attributes MC++'s providers claim (MC4 §2)" };

// At the end of every compilation this program runs (mcxx's own, and libmc++'s parses for the
// editor): the active rules -- MC++'s built-in ones and the plugins linked in -- over the file's
// facts, at the levels its configuration gives. The configuration's plan says what to collect: a
// file whose package gates nothing costs a manifest lookup (cached), no walk.
class GateConsumer final : public cl::ASTConsumer {
public:
    GateConsumer(cl::CompilerInstance& ci, std::vector<plugin::Finding> filtered, std::vector<msa::fact::Suppression> import_waivers)
        : ci_ { ci }, filtered_ { std::move(filtered) }, import_waivers_ { std::move(import_waivers) } {}

    void HandleTranslationUnit(cl::ASTContext& ctx) override {
        if (gates_suppressed || ci_.getDiagnostics().hasFatalErrorOccurred()) return;
        const auto& sm = ctx.getSourceManager();
        const cl::FileID main { sm.getMainFileID() };
        std::string path { path_of(sm, main) };
        if (path.empty()) return;
        // Absolute, as audit records and configuration lookup name files (MC1 §8), whatever the
        // command line spelled.
        path = absolute_path(path);
        std::shared_ptr<const features::Plan> plan { features::plan_for(path) };
        // The package's plugins (MC4 §3): out-of-process ones are started and join the catalog (the
        // plan is then made again); static ones must be composed into this compiler.
        std::vector<std::string> plugin_problems;
        if (!plan->config.plugins.empty()) {
            plugin_problems = plugin::host::load(plan->config);
            for (const auto& entry : plan->config.plugins)
                if (entry.is_static() && std::ranges::find(plugin::host::composed(), entry.name) == plugin::host::composed().end())
                    plugin_problems.push_back(std::format("static plugin {} is not composed into this compiler: run `mcxx compose` in {} (MC4-3-4)",
                                                          entry.name, fs::path { plan->config.manifest }.parent_path().generic_string()));
            plan = features::plan_for(path);
        }
        report_plugin_problems(ctx, plugin_problems);
        check(ctx, path, *plan);
        // A module unit's BMI gets its interface beside it (MC2), once the unit has passed its gates.
        write_interface(ci_, ctx, path, *plan);
    }

private:
    cl::CompilerInstance& ci_;
    std::vector<plugin::Finding> filtered_;   // the source filters' findings (an extension's uses)
    std::vector<msa::fact::Suppression> import_waivers_;   // `import m [[mcpp::allow("id")]];` (M1.2)

    // The active rules over the file's facts, at the levels its configuration gives.
    void check(cl::ASTContext& ctx, const std::string& path, const features::Plan& planned) {
        const features::Plan* plan { &planned };
        const auto& sm = ctx.getSourceManager();
        const cl::FileID main { sm.getMainFileID() };
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
        facts.suppressions.insert(facts.suppressions.end(), import_waivers_.begin(), import_waivers_.end());
        // The manifest's allowances for imports (MC1 §5): a waiver over each import of the module.
        for (const auto& im : facts.imports)
            if (const auto it = plan->config.imports.find(im.module); it != plan->config.imports.end()) {
                msa::fact::Suppression s;
                s.range = im.range;
                s.ids = it->second.ids;
                s.declaration = std::format("import {} ({})", im.module, fs::path { plan->config.manifest }.filename().generic_string());
                s.reason = it->second.reason;
                facts.suppressions.push_back(std::move(s));
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
            // MC++'s own codes travel in the message, as a feature's id does (MC1-9-3).
            const std::string text { d.code.starts_with("mcxx-") && !d.message.contains("[" + d.code + "]") ? std::format("{} [{}]", d.message, d.code) : d.message };
            remember_finding(begin, text, d);   // what the view lays out apart (MC5 §9)
            auto report = diags.Report(begin, id);
            report << text;
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

    // A plugin the package declares that is not there: what it gates is unknown, so a compilation
    // fails (MC4-5-5); the editor's parse says so as a warning, since it cannot compose.
    void report_plugin_problems(cl::ASTContext& ctx, const std::vector<std::string>& problems) {
        if (problems.empty()) return;
        auto& diags = ci_.getDiagnostics();
        const auto& sm = ctx.getSourceManager();
        const cl::SourceLocation top { sm.getLocForStartOfFile(sm.getMainFileID()) };
        const unsigned id { diags.getCustomDiagID(on_clang_stack ? cl::DiagnosticsEngine::Warning : cl::DiagnosticsEngine::Error, "%0 [mcxx-plugin]") };
        for (const auto& p : problems) diags.Report(top, id) << p;
    }
};

// The target, as a source filter reads it (plugin::Target).
// A target in a configuration's words, from its triple (pointer width and byte order by the
// triple's architecture).
plugin::Target target_of_triple(const llvm::Triple& t) {
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
    target.pointer_width = t.isArch64Bit() ? 64 : t.isArch32Bit() ? 32 : 16;
    target.endian = t.isLittleEndian() ? "little" : "big";
    return target;
}

plugin::Target target_of(const cl::CompilerInstance& ci) {
    plugin::Target target { target_of_triple(ci.getTarget().getTriple()) };
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
        install_view(ci);   // human or agent diagnostics, where asked for (MC5 §9)
        // The package's out-of-process plugins before the file is parsed: their attributes are claimed too.
        if (!gates_suppressed) {
            const auto& sm = ci.getSourceManager();
            if (const std::string path { path_of(sm, sm.getMainFileID()) }; !path.empty())
                if (const auto plan = features::plan_for(path); !plan->config.plugins.empty()) (void)plugin::host::load(plan->config);
        }
        if (auto* claimed = ClaimedAttrInfo::instance()) claimed->refresh();
        // Compiling a precompiled interface (a .pcm) to an object: its source was parsed, and gated,
        // when it was precompiled. Gating the AST read back would report every finding twice (MC5-3-3).
        for (const auto& input : ci.getFrontendOpts().Inputs)
            if (input.getKind().getFormat() == cl::InputKind::Precompiled) return std::make_unique<cl::ASTConsumer>();
        auto filtered = filter_main_file(ci);
        return std::make_unique<GateConsumer>(ci, std::move(filtered), strip_import_annotations(ci));
    }
    bool ParseArgs(const cl::CompilerInstance&, const std::vector<std::string>&) override { return true; }
    ActionType getActionType() override { return AddAfterMainAction; }
};

cl::FrontendPluginRegistry::Add<GateAction> gate_registration { "mcxx-gates", "MC++ feature gates: the built-in ISO controls and the plugins linked in" };

// The target MC++'s front end has predefined macros for, from a command's --target.
std::string frontend_target(std::string_view target) {
    if (target.empty()) return "x86_64-unknown-linux-gnu";
    if (target.find("windows") != std::string_view::npos || target.find("msvc") != std::string_view::npos || target.find("mingw") != std::string_view::npos)
        return "x86_64-pc-windows-msvc";
    if (target.find("apple") != std::string_view::npos || target.find("darwin") != std::string_view::npos || target.find("macos") != std::string_view::npos)
        return "aarch64-apple-darwin";
    return "x86_64-unknown-linux-gnu";
}

} // namespace

msa::Workspace::Quick quick_gates(const std::string& path, std::string_view text, const msa::Command* command) {
    msa::Workspace::Quick out;
    const auto plan = features::plan_for(path);
    if (!plan || !plan->gated) return out;
    frontend::PreprocessOptions options;
    options.file = path;
    std::string target;
    if (command != nullptr) {
        const auto& a = command->arguments;
        for (std::size_t i { 1 }; i < a.size(); ++i) {
            const std::string_view x { a[i] };
            if ((x == "-D" || x == "-U") && i + 1 < a.size()) (x == "-D" ? options.defines : options.undefines).push_back(a[++i]);
            else if (x.starts_with("-D") && x.size() > 2) options.defines.emplace_back(x.substr(2));
            else if (x.starts_with("-U") && x.size() > 2) options.undefines.emplace_back(x.substr(2));
            else if (x.starts_with("--target=")) target = x.substr(9);
            else if (x == "-target" && i + 1 < a.size()) target = a[++i];
        }
    }
    options.target = frontend_target(target);
    const auto syntax = frontend::parse(text, options);
    const auto facts = frontend::facts(syntax);
    // The features the reading decides: those decided from the kinds it fills.
    std::set<std::string, std::less<>> decided;
    for (const auto& entry : plan->catalog->features) {
        // What an import brings in needs the imported BMIs (MC3 §4.13): not known before the parse, whose
        // findings replace these. The rest of such a feature is decided here all the same.
        const auto needs = std::to_underlying(entry.feature->needs) & ~std::to_underlying(msa::fact::Kinds::imports);
        if (needs != 0 && (needs & ~std::to_underlying(facts.collected)) == 0) decided.insert(entry.feature->id);
    }
    const std::string module { syntax.pp.module.name + (syntax.pp.module.partition.empty() ? "" : ":" + syntax.pp.module.partition) };
    const plugin::Context context { path, module, facts };
    for (auto& d : features::evaluate(context, *plan).diagnostics)
        if (decided.contains(d.code)) out.diagnostics.push_back(std::move(d));
    out.features.assign(decided.begin(), decided.end());
    return out;
}

std::vector<std::string> language_arguments_of(const std::string& path, const std::vector<std::string>& args) {
    if (plugin::catalog()->languages.empty()) return {};
    std::string triple;
    for (std::size_t i { 0 }; i < args.size(); ++i) {
        if (args[i].starts_with("--target=")) triple = args[i].substr(9);
        else if ((args[i] == "-target" || args[i] == "--target") && i + 1 < args.size()) triple = args[i + 1];
    }
    if (triple.empty()) triple = llvm::sys::getDefaultTargetTriple();
    return features::language_arguments(path, target_of_triple(llvm::Triple { triple }), args);
}

} // namespace mcxx::clang_backend
