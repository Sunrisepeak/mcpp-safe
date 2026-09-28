// Parse a C++ module interface in memory with Clang's libraries and report what it declares.
#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/Frontend/ASTUnit.h>
#include <clang/Tooling/Tooling.h>
#include <cstdio>

int main() {
    const char* code = "export module m;\nexport namespace m { int answer() { return 42; } struct S { int x; }; }\n";
    std::unique_ptr<clang::ASTUnit> unit = clang::tooling::buildASTFromCodeWithArgs(code, {"-std=c++23", "-xc++-module"}, "m.cppm");
    if (!unit) { std::puts("FAIL: no AST"); return 1; }
    unsigned decls = 0;
    for (const clang::Decl* d : unit->getASTContext().getTranslationUnitDecl()->decls()) { (void)d; ++decls; }
    const clang::Module* module = unit->getASTContext().getCurrentNamedModule();
    std::printf("module=%s top-level-decls=%u errors=%u\n", module ? module->Name.c_str() : "(none)", decls,
                unit->getDiagnostics().getClient()->getNumErrors());
    return module && module->Name == "m" ? 0 : 1;
}
