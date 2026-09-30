// mcxx.backend.clang implementation unit: the definitions of :ifc (MC2: a module unit's interface beside its BMI).
module;

#include <clang/AST/ASTContext.h>
#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/LangOptions.h>
#include <clang/Basic/Module.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Basic/TargetInfo.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendAction.h>
#include <clang/Frontend/FrontendOptions.h>
#include <clang/Frontend/MultiplexConsumer.h>
#include <llvm/Support/MemoryBuffer.h>
#include <clang/Lex/Preprocessor.h>

#include <chrono>
#include <filesystem>
#include <cstdint>
#include <format>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

module mcxx.backend.clang;

import mcxx.msa;
import mcxx.base;
import mcxx.plugin;
import mcxx.features;
import mcxx.ifc;
import mcxx.frontend;
import :support;
import :facts;
import :ifc;

namespace mcxx::clang_backend {

namespace {


// The BMI this compile writes, or "": --precompile's output, or -fmodule-output's. libmc++'s own
// parses (the editor, `mcxx check -p`, the probe) write none, though Clang derives a module output
// path for a .cppm given with -c; nor does a -fsyntax-only compile.
std::string bmi_output(const cl::CompilerInstance& ci) {
    if (on_clang_stack) return {};
    const auto& fo = ci.getFrontendOpts();
    if (fo.ProgramAction == cl::frontend::GenerateModuleInterface || fo.ProgramAction == cl::frontend::GenerateReducedModuleInterface)
        return fo.OutputFile == "-" ? std::string {} : fo.OutputFile;
    if (fo.ProgramAction == cl::frontend::ParseSyntaxOnly) return {};
    return fo.ModuleOutputPath;
}

std::uint32_t cplusplus_of(const cl::LangOptions& lang) {
    if (lang.CPlusPlus26) return 202400;
    if (lang.CPlusPlus23) return 202302;
    if (lang.CPlusPlus20) return 202002;
    if (lang.CPlusPlus17) return 201703;
    if (lang.CPlusPlus14) return 201402;
    return 201103;
}

// The dialect the unit was gated with: its package's profiles, every feature's level for code in
// the module, and the levels its package sets for namespaces.
ifc::Dialect dialect_of(const features::Plan& plan, std::string_view module, std::string_view path) {
    ifc::Dialect dialect;
    dialect.profiles = plan.config.profiles;
    // The unit's own file's levels: what its code was gated with (MC1 0.4.0's `files` included).
    const std::string file { plan.relative(path) };
    for (const auto& gate : plan.gates)
        dialect.features.push_back({ gate.entry->feature->id, std::string { plugin::to_string(plan.level(gate, module, {}, file)) } });
    for (const auto& [name, levels] : plan.config.namespaces)
        for (const auto& [feature, level] : levels) dialect.namespaces.push_back({ name, feature, std::string { plugin::to_string(level) } });
    return dialect;
}

// The unit's interface: its T1 declarations, what else an importer reaches through it (MC2 1.2.0), its
// dialect, what it re-exports (`export import`).
ifc::Interface interface_of(cl::CompilerInstance& ci, cl::ASTContext& ctx, const cl::Module& m, const std::string& path, const features::Plan& plan) {
    ifc::Interface unit;
    unit.module = m.getFullModuleName();
    unit.internal = m.Kind == cl::Module::ModulePartitionImplementation;
    unit.source = path;
    unit.target = ci.getTarget().getTriple().str();
    unit.cplusplus = cplusplus_of(ci.getLangOpts());
    unit.dialect = dialect_of(plan, unit.module, path);
    unit.declarations = ifc::interface_declarations(facts_of(ctx, &ci.getPreprocessor(), msa::fact::Kinds::declarations | msa::fact::Kinds::declaration_types));
    unit.reachable = reachable_of(ctx);
    for (const auto& e : m.Exports)
        if (const cl::Module* x = static_cast<cl::Module*>(e.first); x != nullptr && x->isNamedModule()) unit.reexports.push_back(x->getFullModuleName());
    for (const cl::Module* x : m.Imports)
        if (x != nullptr && x->isNamedModule() && !std::ranges::contains(unit.reexports, x->getFullModuleName())) unit.imports.push_back(x->getFullModuleName());
    return unit;
}

// Writes the unit's interface to `out`; false (with a warning) when it cannot.
bool save_interface(cl::CompilerInstance& ci, cl::ASTContext& ctx, const std::string& path, const features::Plan& plan, const std::string& out) {
    const cl::Module* m { ctx.getCurrentNamedModule() };
    if (m == nullptr || ci.getDiagnostics().hasErrorOccurred()) return false;
    base::trace::Span span { "ifc", "write", path, std::chrono::milliseconds { 200 } };
    const ifc::Interface unit { interface_of(ci, ctx, *m, path, plan) };
    if (const auto error = ifc::save(out, unit)) {
        auto& diags = ci.getDiagnostics();
        diags.Report(diags.getCustomDiagID(cl::DiagnosticsEngine::Warning, "%0 [mcxx-ifc]")) << *error;
        return false;
    }
    span.note(std::format("{} declarations", unit.declarations.size()));
    base::trace::count("ifc.written");
    return true;
}

// libmc++'s own BMI builds (the editor's, `mcxx check -p`'s): the interface beside the BMI they
// write, so an importer's parse finds it there.
class InterfaceWriter final : public cl::ASTConsumer {
public:
    InterfaceWriter(cl::CompilerInstance& ci, std::string path, std::string bmi) : ci_ { ci }, path_ { std::move(path) }, bmi_ { std::move(bmi) } {}
    void HandleTranslationUnit(cl::ASTContext& ctx) override {
        if (ci_.getDiagnostics().hasErrorOccurred()) return;
        (void)save_interface(ci_, ctx, path_, *features::plan_for(path_), ifc::path_for(bmi_));
    }

private:
    cl::CompilerInstance& ci_;
    std::string path_;
    std::string bmi_;
};

} // namespace

std::vector<msa::fact::Suppression> strip_import_annotations(cl::CompilerInstance& ci) {
    auto& sm = ci.getSourceManager();
    const cl::FileID main { sm.getMainFileID() };
    if (main.isInvalid()) return {};
    const auto entry = sm.getFileEntryRefForID(main);
    const auto buffer = sm.getBufferOrNone(main);
    if (!entry || !buffer) return {};
    const std::string_view text { buffer->getBuffer().data(), buffer->getBuffer().size() };
    if (!text.contains("mcpp::allow") || !text.contains("import")) return {};
    const auto annotations = frontend::import_annotations(text);
    if (annotations.empty()) return {};
    sm.overrideFileContents(*entry, llvm::MemoryBuffer::getMemBufferCopy(frontend::blank_import_annotations(text, annotations), buffer->getBufferIdentifier()));
    std::vector<msa::fact::Suppression> out;
    for (const auto& a : annotations) {
        msa::fact::Suppression s;
        s.range = a.range;
        s.ids = a.ids;
        s.declaration = "import " + a.module;
        s.reason = a.reason;
        out.push_back(std::move(s));
    }
    return out;
}

void write_interface(cl::CompilerInstance& ci, cl::ASTContext& ctx, const std::string& path, const features::Plan& plan) {
    const std::string bmi { bmi_output(ci) };
    if (bmi.empty()) return;
    const std::string out { ifc::path_for(bmi) };
    if (!save_interface(ci, ctx, path, plan, out)) return;
    ifc::note_written(absolute_path(bmi), absolute_path(out));
}

std::unique_ptr<cl::ASTConsumer> InterfaceAction::CreateASTConsumer(cl::CompilerInstance& ci, llvm::StringRef in) {
    (void)strip_import_annotations(ci);   // no gates here: only what Clang must not see
    std::vector<std::unique_ptr<cl::ASTConsumer>> both;
    auto wrapped = cl::WrapperFrontendAction::CreateASTConsumer(ci, in);
    if (!wrapped) return nullptr;
    both.push_back(std::move(wrapped));
    both.push_back(std::make_unique<InterfaceWriter>(ci, path_, bmi_));
    return std::make_unique<cl::MultiplexConsumer>(std::move(both));
}

} // namespace mcxx::clang_backend
