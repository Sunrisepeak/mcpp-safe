// mcxx.backend.clang partition :completion: code completion and signature help
// Declarations; the definitions are in completion.cpp (MC5 §8).
module;

#include <clang/Frontend/ASTUnit.h>
#include <clang/Sema/CodeCompleteConsumer.h>
#include <clang/Sema/Sema.h>

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

module mcxx.backend.clang:completion;

import mcxx.msa;
import mcxx.graph;
import mcxx.base;
import :support;
import :unit;

namespace mcxx::clang_backend {

namespace cl = ::clang;
namespace fs = std::filesystem;
using msa::Position;
using msa::Range;
using msa::Location;

// ============================================================================================
// 4. completion
// ============================================================================================

class CompletionCollector : public cl::CodeCompleteConsumer {
public:
    explicit CompletionCollector(const cl::CodeCompleteOptions& options)
        : cl::CodeCompleteConsumer(options), info_ { std::make_shared<cl::GlobalCodeCompletionAllocator>() } {}

    std::vector<msa::CompletionItem> items;
    msa::SignatureHelp signatures;

    void ProcessCodeCompleteResults(cl::Sema& sema, cl::CodeCompletionContext context, cl::CodeCompletionResult* results, unsigned count) override;
    void ProcessOverloadCandidates(cl::Sema& sema, unsigned current, OverloadCandidate* candidates, unsigned count, cl::SourceLocation,
                                   bool braced) override;

    cl::CodeCompletionAllocator& getAllocator() override { return info_.getAllocator(); }
    cl::CodeCompletionTUInfo& getCodeCompletionTUInfo() override { return info_; }

private:
    cl::CodeCompletionTUInfo info_;
};

// Completion at `at` in `text`, re-parsing through the unit's own parse.
void run_completion(cl::ASTUnit& ast, const std::string& path, const std::string& text, Position at, CompletionCollector& collector);

} // namespace mcxx::clang_backend
