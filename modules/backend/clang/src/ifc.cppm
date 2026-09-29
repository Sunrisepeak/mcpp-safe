// mcxx.backend.clang partition :ifc: MC2 -- beside every BMI a compile writes (a module interface unit,
// an implementation partition), X.ifc: the unit's T1 facts and its dialect in the IFC format
// (mcxx.ifc). An importer reads another module's interface from there, without its source (M1.2).
// Declarations; the definitions are in ifc.cpp (MC5 §8).
module;

#include <clang/AST/ASTContext.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendAction.h>
#include <llvm/ADT/StringRef.h>

#include <memory>
#include <string>
#include <vector>

module mcxx.backend.clang:ifc;

import mcxx.msa;
import mcxx.features;

namespace mcxx::clang_backend {

namespace cl = ::clang;

// MC++'s annotations of the main file's imports, `import m [[mcpp::allow("id")]];` (M1.2): blanked
// before Clang reads the file -- Clang refuses an attribute on an import -- and returned as waivers over
// their import declarations. Every position stays where it was.
std::vector<msa::fact::Suppression> strip_import_annotations(cl::CompilerInstance& ci);

// At the end of a compile of a module unit that writes a BMI, and only if it has no error: X.ifc
// beside X.pcm, unless it already says the same; and, once the compile has succeeded, a copy in the
// store under the BMI's content (the driver calls ifc::publish). A file that cannot be written is a
// warning: the build has what it asked for, and an importer that needs the file says it is missing.
void write_interface(cl::CompilerInstance& ci, cl::ASTContext& ctx, const std::string& path, const features::Plan& plan);

// A module build that also writes the interface beside `bmi`.
class InterfaceAction final : public cl::WrapperFrontendAction {
public:
    InterfaceAction(std::unique_ptr<cl::FrontendAction> wrapped, std::string path, std::string bmi)
        : cl::WrapperFrontendAction { std::move(wrapped) }, path_ { std::move(path) }, bmi_ { std::move(bmi) } {}

protected:
    std::unique_ptr<cl::ASTConsumer> CreateASTConsumer(cl::CompilerInstance& ci, llvm::StringRef in) override;

private:
    std::string path_;
    std::string bmi_;
};

} // namespace mcxx::clang_backend
