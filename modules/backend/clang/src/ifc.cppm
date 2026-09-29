// mcxx.backend.clang partition :ifc: MC2 -- beside every BMI a compile writes (a module interface unit,
// an implementation partition), X.ifc: the unit's T1 facts and its dialect in the IFC format
// (mcxx.ifc). An importer reads another module's interface from there, without its source (M1.2).
module;

#include <clang/AST/ASTContext.h>
#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/LangOptions.h>
#include <clang/Basic/Module.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Basic/TargetInfo.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendOptions.h>
#include <clang/Lex/Preprocessor.h>

#include <chrono>
#include <cstdint>
#include <format>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

module mcxx.backend.clang:ifc;

import mcxx.msa;
import mcxx.base;
import mcxx.plugin;
import mcxx.features;
import mcxx.ifc;
import :support;
import :facts;

namespace mcxx::clang_backend {

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
ifc::Dialect dialect_of(const features::Plan& plan, std::string_view module) {
    ifc::Dialect dialect;
    dialect.profiles = plan.config.profiles;
    for (const auto& gate : plan.gates)
        dialect.features.push_back({ gate.entry->feature->id, std::string { plugin::to_string(plan.level(gate, module, {})) } });
    for (const auto& [name, levels] : plan.config.namespaces)
        for (const auto& [feature, level] : levels) dialect.namespaces.push_back({ name, feature, std::string { plugin::to_string(level) } });
    return dialect;
}

// At the end of a compile of a module unit that writes a BMI, and only if it has no error: X.ifc
// beside X.pcm, unless it already says the same. A file that cannot be written is a warning: the
// build has what it asked for, and an importer that needs the file says it is missing.
void write_interface(cl::CompilerInstance& ci, cl::ASTContext& ctx, const std::string& path, const features::Plan& plan) {
    const cl::Module* m { ctx.getCurrentNamedModule() };
    if (m == nullptr || ci.getDiagnostics().hasErrorOccurred()) return;
    const std::string bmi { bmi_output(ci) };
    if (bmi.empty()) return;
    base::trace::Span span { "ifc", "write", path, std::chrono::milliseconds { 200 } };
    ifc::Interface unit;
    unit.module = m->getFullModuleName();
    unit.internal = m->Kind == cl::Module::ModulePartitionImplementation;
    unit.source = path;
    unit.target = ci.getTarget().getTriple().str();
    unit.cplusplus = cplusplus_of(ci.getLangOpts());
    unit.dialect = dialect_of(plan, unit.module);
    unit.declarations = ifc::interface_declarations(facts_of(ctx, &ci.getPreprocessor(), msa::fact::Kinds::declarations | msa::fact::Kinds::declaration_types));
    const std::string out { ifc::path_for(bmi) };
    if (const auto error = ifc::save(out, unit)) {
        auto& diags = ci.getDiagnostics();
        diags.Report(diags.getCustomDiagID(cl::DiagnosticsEngine::Warning, "%0 [mcxx-ifc]")) << *error;
        return;
    }
    span.note(std::format("{} declarations", unit.declarations.size()));
    base::trace::count("ifc.written");
}

} // namespace mcxx::clang_backend
