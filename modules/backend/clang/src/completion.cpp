// mcxx.backend.clang implementation unit: the definitions of :completion (code completion, signature help).
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

module mcxx.backend.clang;

import mcxx.msa;
import mcxx.graph;
import mcxx.base;
import :support;
import :unit;
import :completion;

namespace mcxx::clang_backend {

namespace cl = ::clang;
using msa::Position;

void CompletionCollector::ProcessCodeCompleteResults(cl::Sema& sema, cl::CodeCompletionContext context, cl::CodeCompletionResult* results, unsigned count) {
    for (unsigned i { 0 }; i < count; ++i) {
        cl::CodeCompletionResult& r { results[i] };
        if (r.Availability == CXAvailability_NotAvailable || r.Hidden) continue;
        const cl::CodeCompletionString* ccs { r.CreateCodeCompletionString(sema, context, getAllocator(), getCodeCompletionTUInfo(), true) };
        if (!ccs) continue;
        msa::CompletionItem item;
        std::string arguments;
        for (const auto& chunk : *ccs) {
            switch (chunk.Kind) {
            case cl::CodeCompletionString::CK_TypedText: item.label += chunk.Text; item.insert_text += chunk.Text; break;
            case cl::CodeCompletionString::CK_ResultType: item.detail = chunk.Text; break;
            case cl::CodeCompletionString::CK_Optional:
            case cl::CodeCompletionString::CK_Informative: break;
            case cl::CodeCompletionString::CK_Placeholder:
            case cl::CodeCompletionString::CK_CurrentParameter:
            case cl::CodeCompletionString::CK_Text:
            case cl::CodeCompletionString::CK_LeftParen:
            case cl::CodeCompletionString::CK_RightParen:
            case cl::CodeCompletionString::CK_LeftAngle:
            case cl::CodeCompletionString::CK_RightAngle:
            case cl::CodeCompletionString::CK_Comma:
            case cl::CodeCompletionString::CK_Colon:
            case cl::CodeCompletionString::CK_Equal:
            case cl::CodeCompletionString::CK_HorizontalSpace:
                if (chunk.Text) arguments += chunk.Text;
                break;
            default: break;
            }
        }
        if (item.label.empty()) continue;
        item.filter_text = item.label;
        if (!arguments.empty()) item.detail = item.detail.empty() ? arguments : item.detail + " " + item.label + arguments;
        if (const char* brief = ccs->getBriefComment()) item.documentation = brief;
        item.priority = ccs->getPriority();
        switch (r.Kind) {
        case cl::CodeCompletionResult::RK_Declaration: item.kind = kind_of(r.Declaration); break;
        case cl::CodeCompletionResult::RK_Macro: item.kind = msa::Kind::macro; break;
        default: item.kind = msa::Kind::unknown; break;
        }
        items.push_back(std::move(item));
    }
}

void CompletionCollector::ProcessOverloadCandidates(cl::Sema& sema, unsigned current, OverloadCandidate* candidates, unsigned count, cl::SourceLocation,
                                                    bool braced) {
    signatures.active_parameter = current;
    for (unsigned i { 0 }; i < count; ++i) {
        const cl::CodeCompletionString* ccs {
            candidates[i].CreateSignatureString(current, sema, getAllocator(), getCodeCompletionTUInfo(), true, braced)
        };
        if (!ccs) continue;
        msa::Signature signature;
        std::string resultType;
        for (const auto& chunk : *ccs) {
            if (!chunk.Text) continue;
            if (chunk.Kind == cl::CodeCompletionString::CK_ResultType) {
                resultType = chunk.Text;
                continue;
            }
            if (chunk.Kind == cl::CodeCompletionString::CK_Optional || chunk.Kind == cl::CodeCompletionString::CK_Informative) continue;
            const std::uint32_t begin { static_cast<std::uint32_t>(signature.label.size()) };
            signature.label += chunk.Text;
            if (chunk.Kind == cl::CodeCompletionString::CK_Placeholder || chunk.Kind == cl::CodeCompletionString::CK_CurrentParameter)
                signature.parameters.emplace_back(begin, static_cast<std::uint32_t>(signature.label.size()));
        }
        if (!resultType.empty()) signature.label = resultType + " " + signature.label;
        if (!resultType.empty())
            for (auto& [b, e] : signature.parameters) {
                b += static_cast<std::uint32_t>(resultType.size() + 1);
                e += static_cast<std::uint32_t>(resultType.size() + 1);
            }
        if (const char* brief = ccs->getBriefComment()) signature.documentation = brief;
        signatures.signatures.push_back(std::move(signature));
    }
}

// Completion at `at` in `text`, re-parsing through the unit's own parse.
void run_completion(cl::ASTUnit& ast, const std::string& path, const std::string& text, Position at, CompletionCollector& collector) {
    llvm::IntrusiveRefCntPtr<cl::DiagnosticsEngine> diags { &ast.getDiagnostics() };
    cl::LangOptions lang { ast.getLangOpts() };
    llvm::IntrusiveRefCntPtr<cl::FileManager> files { &ast.getFileManager() };
    llvm::IntrusiveRefCntPtr<cl::SourceManager> sources { new cl::SourceManager { *diags, *files } };
    llvm::SmallVector<cl::StoredDiagnostic, 8> stored;
    llvm::SmallVector<const llvm::MemoryBuffer*, 1> owned;
    std::vector<cl::ASTUnit::RemappedFile> remapped { { path, llvm::MemoryBuffer::getMemBufferCopy(text, path).release() } };
    ast.CodeComplete(path, at.line + 1, at.column + 1, remapped, true, false, true, collector, std::make_shared<cl::PCHContainerOperations>(),
                     diags, lang, sources, files, stored, owned, nullptr);
    for (const auto* buffer : owned) delete buffer;
}

} // namespace mcxx::clang_backend
