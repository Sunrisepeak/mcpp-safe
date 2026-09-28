// mcxx.clang partition :index: the program index, built in the background
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

module mcxx.clang:index;

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
// 5. index
// ============================================================================================

struct IndexedOccurrence {
    std::string entity;
    Location location;
    std::uint32_t roles { 0 };
};

struct IndexedEntity {
    std::string name;
    std::string container;
    msa::Kind kind { msa::Kind::unknown };
};

class ProgramIndex {
public:
    void replace(const std::string& file, std::vector<IndexedOccurrence> occurrences, std::map<std::string, IndexedEntity> entities) {
        std::unique_lock lock { mutex_ };
        if (const auto old = shards_.find(file); old != shards_.end()) {
            for (const auto& o : old->second) {
                auto& list = byEntity_[o.entity];
                std::erase_if(list, [&](const IndexedOccurrence* p) { return p->location.path == file; });
            }
        }
        auto& shard = shards_[file];
        shard = std::move(occurrences);
        for (const auto& o : shard) byEntity_[o.entity].push_back(&o);
        for (auto& [id, info] : entities) entities_.insert_or_assign(id, std::move(info));
    }

    std::vector<Location> with_role(std::string_view entity, std::uint32_t role) const {
        std::shared_lock lock { mutex_ };
        std::vector<Location> out;
        const auto it = byEntity_.find(std::string { entity });
        if (it == byEntity_.end()) return out;
        for (const auto* o : it->second)
            if ((o->roles & role) && std::ranges::find(out, o->location) == out.end()) out.push_back(o->location);
        return out;
    }

    // Entities whose name contains the query's last component (any case); a query with a scope,
    // "ns::name" or "::name", keeps those whose container ends with that scope. Exact names first.
    std::vector<msa::Found> find(std::string_view query, std::size_t limit) const {
        auto lower = [](std::string_view text) {
            std::string out { text };
            std::ranges::transform(out, out.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            return out;
        };
        std::string scope;
        std::string needle { lower(query) };
        bool global { false };
        if (const auto split = needle.rfind("::"); split != std::string::npos) {
            scope = needle.substr(0, split);
            needle = needle.substr(split + 2);
            if (scope.starts_with("::")) scope.erase(0, 2);
            global = scope.empty();
        }
        std::shared_lock lock { mutex_ };
        std::vector<std::pair<int, msa::Found>> ranked;
        for (const auto& [id, info] : entities_) {
            if (info.kind == msa::Kind::parameter || info.kind == msa::Kind::template_parameter || info.kind == msa::Kind::unknown) continue;
            const std::string name { lower(info.name) };
            if (!needle.empty() && name.find(needle) == std::string::npos) continue;
            if (!scope.empty() || global) {
                const std::string container { lower(info.container) };
                if (global ? !container.empty() : !(container == scope || container.ends_with("::" + scope))) continue;
            }
            const auto defs = byEntity_.find(id);
            if (defs == byEntity_.end() || defs->second.empty()) continue;
            const IndexedOccurrence* at { nullptr };
            for (const auto* o : defs->second)
                if (o->roles & (msa::role::definition | msa::role::declaration)) {
                    at = o;
                    if (o->roles & msa::role::definition) break;
                }
            if (!at) continue;
            ranked.emplace_back(name == needle ? 0 : name.starts_with(needle) ? 1 : 2, msa::Found { id, info.name, info.container, info.kind, at->location });
        }
        std::ranges::sort(ranked, [](const auto& a, const auto& b) {
            if (a.first != b.first) return a.first < b.first;
            if (a.second.name.size() != b.second.name.size()) return a.second.name.size() < b.second.name.size();
            return a.second.name < b.second.name;
        });
        std::vector<msa::Found> out;
        for (auto& [rank, found] : ranked) {
            if (out.size() >= limit) break;
            out.push_back(std::move(found));
        }
        return out;
    }

    std::size_t files() const {
        std::shared_lock lock { mutex_ };
        return shards_.size();
    }

private:
    mutable std::shared_mutex mutex_;
    std::map<std::string, std::vector<IndexedOccurrence>> shards_;   // node-stable: pointers into vectors stay valid until replaced
    std::unordered_map<std::string, std::vector<const IndexedOccurrence*>> byEntity_;
    std::unordered_map<std::string, IndexedEntity> entities_;
};

// Everything the index keeps of one parsed unit: occurrences outside function bodies' locals.
void index_unit(cl::ASTUnit& ast, const std::string& path, ProgramIndex& index) {
    auto& sm = ast.getSourceManager();
    const auto& lo = ast.getLangOpts();
    const cl::FileID main { sm.getMainFileID() };
    std::vector<IndexedOccurrence> occurrences;
    std::map<std::string, IndexedEntity> entities;
    OccurrenceConsumer consumer { [&](const cl::Decl* d, cl::index::SymbolRoleSet roles, llvm::ArrayRef<cl::index::SymbolRelation> relations,
                                      cl::SourceLocation loc) {
        const cl::SourceLocation file { sm.getFileLoc(loc) };
        if (file.isInvalid() || sm.getFileID(file) != main) return;
        if (const auto* v = llvm::dyn_cast<cl::VarDecl>(d); v && v->isLocalVarDeclOrParm()) return;
        const cl::Decl* canonical { d->getCanonicalDecl() };
        std::string id { usr_of(canonical) };
        if (id.empty()) return;
        auto token = token_location(sm, lo, loc);
        if (!token) return;
        token->path = path;
        occurrences.push_back({ id, *token, roles_of(roles, relations) });
        if (!entities.contains(id)) {
            IndexedEntity info;
            if (const auto* nd = llvm::dyn_cast<cl::NamedDecl>(d)) info.name = nd->getNameAsString();
            for (const cl::DeclContext* dc { d->getDeclContext() }; dc; dc = dc->getParent())
                if (const auto* named = llvm::dyn_cast<cl::NamedDecl>(dc)) {
                    info.container = named->getQualifiedNameAsString();
                    break;
                }
            info.kind = kind_of(d);
            entities.emplace(std::move(id), std::move(info));
        }
    } };
    cl::index::indexASTUnit(ast, consumer, indexing_options(false));
    index.replace(path, std::move(occurrences), std::move(entities));
}

} // namespace mcxx::clang_backend

