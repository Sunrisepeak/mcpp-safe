// mcxx.backend.clang partition :index: the program index, built in the background
// Declarations; the definitions are in index.cpp (MC5 §8).
module;

#include <clang/Frontend/ASTUnit.h>

#include <cstdint>
#include <filesystem>
#include <map>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

module mcxx.backend.clang:index;

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
    void replace(const std::string& file, std::vector<IndexedOccurrence> occurrences, std::map<std::string, IndexedEntity> entities);

    std::vector<Location> with_role(std::string_view entity, std::uint32_t role) const;

    // Entities whose name contains the query's last component (any case); a query with a scope,
    // "ns::name" or "::name", keeps those whose container ends with that scope. Exact names first.
    std::vector<msa::Found> find(std::string_view query, std::size_t limit) const;

    std::size_t files() const;

private:
    mutable std::shared_mutex mutex_;
    std::map<std::string, std::vector<IndexedOccurrence>> shards_;   // node-stable: pointers into vectors stay valid until replaced
    std::unordered_map<std::string, std::vector<const IndexedOccurrence*>> byEntity_;
    std::unordered_map<std::string, IndexedEntity> entities_;
};

// Everything the index keeps of one parsed unit: occurrences outside function bodies' locals.
void index_unit(cl::ASTUnit& ast, const std::string& path, ProgramIndex& index);

} // namespace mcxx::clang_backend
