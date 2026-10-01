// mcxx.backend.clang partition :unit: parsing a file: diagnostics, occurrences, symbols, entities
// Declarations, and the units' class; the definitions are in unit.cpp (MC5 §8).
module;

#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/AST/PrettyPrinter.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Frontend/ASTUnit.h>
#include <clang/Index/IndexDataConsumer.h>
#include <clang/Index/IndexSymbol.h>
#include <clang/Index/IndexingOptions.h>
#include <llvm/ADT/ArrayRef.h>

#include <cstdint>
#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

module mcxx.backend.clang:unit;

import mcxx.msa;
import mcxx.graph;
import mcxx.base;
import mcxx.plugin;
import :support;

namespace mcxx::clang_backend {

namespace cl = ::clang;
namespace fs = std::filesystem;
using msa::Position;
using msa::Range;
using msa::Location;

// ============================================================================================
// 3. units
// ============================================================================================

// The file of a source manager's file id ("" for none).
std::string path_of(const cl::SourceManager& sm, cl::FileID fid);
// A location's 0-based line and column.
Position position_of(const cl::SourceManager& sm, cl::SourceLocation loc);
// The name token at `loc`, as a location in the file it is written in.
std::optional<Location> token_location(const cl::SourceManager& sm, const cl::LangOptions& lo, cl::SourceLocation loc);
// A range in the main file, whole tokens; nothing when it is not all there.
std::optional<Range> source_range(const cl::SourceManager& sm, const cl::LangOptions& lo, cl::SourceRange range, cl::FileID main);
// A declaration's USR ("" when it has none).
std::string usr_of(const cl::Decl* d);
msa::Kind kind_of(const cl::Decl* d);
std::uint32_t roles_of(cl::index::SymbolRoleSet roles, llvm::ArrayRef<cl::index::SymbolRelation> relations);
// Declarations as a signature shows them: terse, no bodies, no unwritten scopes.
cl::PrintingPolicy printing_policy(const cl::ASTContext& ctx);

// Occurrences of named entities, as Clang's indexer reports them.
class OccurrenceConsumer : public cl::index::IndexDataConsumer {
public:
    using Handler = std::function<void(const cl::Decl*, cl::index::SymbolRoleSet, llvm::ArrayRef<cl::index::SymbolRelation>, cl::SourceLocation)>;
    explicit OccurrenceConsumer(Handler handler) : handler_ { std::move(handler) } {}
    bool handleDeclOccurrence(const cl::Decl* d, cl::index::SymbolRoleSet roles, llvm::ArrayRef<cl::index::SymbolRelation> relations,
                              cl::SourceLocation loc, ASTNodeInfo) override {
        if (d) handler_(d, roles, relations, loc);
        return true;
    }

private:
    Handler handler_;
};

cl::index::IndexingOptions indexing_options(bool locals);

// A diagnostic's stable name, as clangd gives it: Clang's own name less its kind ("err_", "warn_",
// "ext_"), e.g. "expected_semi_after_module_or_import".
std::string diagnostic_code(unsigned id);

class UnitImpl final : public msa::Unit {
public:
    UnitImpl(std::unique_ptr<cl::ASTUnit> ast, std::string path, std::string text, std::int64_t version, std::string module,
             std::vector<msa::Diagnostic> extra, ClangPool* pool);

    const std::string& path() const override { return path_; }
    std::int64_t version() const override { return version_; }
    std::string_view text() const override { return text_; }
    std::string_view module_name() const override { return module_; }
    std::span<const msa::Diagnostic> diagnostics() const override { return diagnostics_; }
    std::span<const msa::Occurrence> occurrences() const override { return occurrences_; }

    std::vector<msa::Symbol> symbols() const override;
    std::optional<msa::Entity> entity_at(Position at) const override;
    std::optional<msa::Entity> entity(std::string_view id) const override;
    std::vector<Location> overriders(std::string_view id) const override;

    // MC3 v0 facts of the file's own code (defined in facts.cpp).
    const msa::fact::Facts& facts() const override;
    msa::fact::Facts facts(msa::fact::Kinds kinds) const override;
    // The file's own declarations counted by Clang's kind names, straight off the AST (A0.4.3's
    // reference: a clang::RecursiveASTVisitor with Clang's defaults, not MSA's collector).
    std::map<std::string, std::int64_t> census() const;

    // For completion: the parse this snapshot came from, used exclusively.
    template <class F>
    auto with_ast(F&& f) const {
        std::lock_guard lock { mutex_ };
        return f(ast_.get());
    }

private:
    std::vector<msa::Symbol> symbols_() const;
    std::optional<msa::Entity> entity_(std::string_view id) const;
    std::vector<Location> overriders_(std::string_view id) const;

    ClangPool* pool_;            // the Workspace's: AST access runs on a Clang stack
    mutable std::mutex mutex_;   // the AST loads declarations from interfaces lazily; one reader at a time
    std::unique_ptr<cl::ASTUnit> ast_;
    std::string path_;
    std::string text_;
    std::int64_t version_;
    std::string module_;
    std::vector<msa::Diagnostic> diagnostics_;
    std::vector<msa::Occurrence> occurrences_;
    std::unordered_map<std::string, const cl::Decl*> decls_;
    mutable std::optional<msa::fact::Facts> facts_;
};

struct ParseRequest {
    msa::Command command;
    std::string path;
    std::string text;
    bool remap { true };
    bool module_unit { false };
    std::map<std::string, std::string> modules;   // module -> interface file
    std::string resource_directory;
};

// `rejected`, when the parse could not start: why the driver rejected the command.
std::unique_ptr<cl::ASTUnit> parse_ast(const ParseRequest& request, std::string* rejected = nullptr);

} // namespace mcxx::clang_backend
