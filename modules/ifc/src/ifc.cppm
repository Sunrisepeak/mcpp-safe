// MC2 v1 (specs/mc2-ifc.md): what MC++ knows of a module unit, in the IFC format -- Microsoft's, format
// 0.43, the IFC SDK's structures -- written beside the unit's BMI as X.ifc. It carries the unit's T1
// facts, the declarations that are not local (MC3 §4.2), and its dialect: the profiles and the feature
// levels its code was gated with. An importer learns what another module exposes from here, without
// its source (M1.2).
//
// The declarations are IFC declarations in IFC scopes: names, sorts, places. What MC3 says of each
// that IFC 0.43 has no field for -- the entity, the qualified name, the type as text, the templates,
// the flags, the ranges -- is an attribute [[mcxx::decl(...)]] on it, associated by the decl-attrs
// trait. The dialect is an attribute declaration in the global scope:
// [[mcxx::mc2("1.0.0"), mcxx::profile("safe"), mcxx::feature("goto", "deny"), ...]];
//
// Reading does not use the SDK's reader: that one ends the process on a malformed file (ifc_assert).
// Every offset here is checked, and a file that is not what MC2 v1 writes is an error, not a guess.
module;

#include <ifc/abstract-sgraph.hxx>
#include <ifc/file.hxx>

export module mcxx.ifc;

import std;
import mcxx.msa;

export namespace mcxx::ifc {

inline constexpr std::string_view MC2_VERSION { "1.1.0" };   // 1.1.0 adds re-exports; 1.0.0 files are read too
inline constexpr std::uint8_t IFC_MAJOR { 0 };
inline constexpr std::uint8_t IFC_MINOR { 43 };

// What a module's code was gated with: its package's profiles, every feature's level for code in the
// module (a namespace's own levels apart), and the levels the package sets for namespaces.
struct Dialect {
    struct Feature {
        std::string id;
        std::string level;   // "allow" | "warn" | "deny"
        bool operator==(const Feature&) const = default;
    };
    struct NamespaceLevel {
        std::string name;    // "app::detail": that namespace and those inside it
        std::string feature;
        std::string level;
        bool operator==(const NamespaceLevel&) const = default;
    };
    std::vector<std::string> profiles;
    std::vector<Feature> features;
    std::vector<NamespaceLevel> namespaces;
    bool operator==(const Dialect&) const = default;
};

// One module unit's interface: an interface unit ("m", "m:part") or an implementation partition
// ("m:impl", internal).
struct Interface {
    std::string module;
    bool internal { false };             // an implementation partition: importable only by its module
    std::string source;                  // the unit's source file
    std::string target;                  // the target triple ("" = unknown)
    std::uint32_t cplusplus { 202302 };  // __cplusplus of the compile
    Dialect dialect;
    std::vector<msa::fact::Declaration> declarations;   // T1: what is not local, in the facts' order
    std::vector<std::string> reexports;                 // `export import`: the modules an importer also sees ("m:part")
};

// The declarations of a unit's facts that an interface carries: those that are not local.
std::vector<msa::fact::Declaration> interface_declarations(const msa::fact::Facts& facts);

std::vector<std::byte> write(const Interface& unit);
std::expected<Interface, std::string> read(std::span<const std::byte> bytes);

// X.pcm -> X.ifc (any extension; none: .ifc added).
std::string path_for(std::string_view bmi);
// Writes `unit` to `path` unless the file already holds exactly these bytes (a BMI's neighbour keeps
// its time when nothing changed). An error, or nothing.
std::optional<std::string> save(const std::string& path, const Interface& unit);
std::expected<Interface, std::string> load(const std::string& path);

// ---- Finding an interface from a BMI --------------------------------------------------------------
//
// Beside it, or -- for a BMI a build copied from elsewhere (mcpp's build caches keep a BMI, not what is
// beside it) -- in the store, where the compile that wrote an interface keeps a copy under the
// SHA-256 of its BMI's bytes: $MCXX_IFC_STORE, else $XDG_CACHE_HOME/mcxx/ifc, else ~/.cache/mcxx/ifc.

std::string store_directory();
// A host's own store (mcppls keeps one in its cache), in place of the default; "" : the default again.
void use_store(std::string directory);
// Called by the compile that wrote `ifc` beside `bmi`; publish() -- once the compile has succeeded
// and the BMI is in place -- keeps a copy of each in the store.
void note_written(std::string bmi, std::string ifc);
void publish();
// The interface of the module whose BMI is `bmi`: read once per BMI (its size and time), shared. Null
// when there is none, or it cannot be read (`error` says which).
std::shared_ptr<const Interface> interface_for(const std::string& bmi, std::string* error = nullptr);

// Item by item: where two lists of declarations differ, field by field ("declaration 3 (a::f): type
// `int *` != `int*`"), at most `limit` of them.
std::vector<std::string> differences(std::span<const msa::fact::Declaration> expected, std::span<const msa::fact::Declaration> actual,
                                     std::size_t limit = 20);

} // namespace mcxx::ifc

namespace mcxx::ifc {

namespace {

namespace sdk = ::ifc;
namespace sym = ::ifc::symbolic;
using msa::Kind;
using msa::Position;
using msa::Range;
using msa::fact::Declaration;

// ---- Names MC3 gives kinds and flags -------------------------------------------------------------

// msa::Kind by its MC3 name: to_string's words joined by '-' ("namespace-alias").
std::string kind_name(Kind kind) {
    std::string name { msa::to_string(kind) };
    std::ranges::replace(name, ' ', '-');
    return name;
}

std::optional<Kind> kind_from(std::string_view name) {
    for (int k { 0 }; k <= static_cast<int>(Kind::label); ++k)
        if (kind_name(static_cast<Kind>(k)) == name) return static_cast<Kind>(k);
    return std::nullopt;
}

bool is_scope(Kind k) { return k == Kind::namespace_ || k == Kind::class_ || k == Kind::struct_ || k == Kind::union_; }
bool is_function(Kind k) {
    return k == Kind::function || k == Kind::method || k == Kind::conversion || k == Kind::constructor || k == Kind::destructor;
}

// The IFC sort a declaration of each kind is written as (specs/mc2-ifc.md, MC2-3-1). What IFC 0.43
// has no sort for here is a barren declaration carrying the attribute.
sdk::DeclSort sort_for(Kind k) {
    switch (k) {
    case Kind::namespace_:
    case Kind::class_:
    case Kind::struct_:
    case Kind::union_: return sdk::DeclSort::Scope;
    case Kind::enum_: return sdk::DeclSort::Enumeration;
    case Kind::enumerator: return sdk::DeclSort::Enumerator;
    case Kind::type_alias: return sdk::DeclSort::Alias;
    case Kind::function: return sdk::DeclSort::Function;
    case Kind::method:
    case Kind::conversion: return sdk::DeclSort::Method;
    case Kind::constructor: return sdk::DeclSort::Constructor;
    case Kind::destructor: return sdk::DeclSort::Destructor;
    case Kind::field: return sdk::DeclSort::Field;
    case Kind::variable: return sdk::DeclSort::Variable;
    case Kind::parameter: return sdk::DeclSort::Parameter;
    default: return sdk::DeclSort::Barren;
    }
}

constexpr std::pair<bool Declaration::*, std::string_view> FLAGS[] {
    { &Declaration::exported, "exported" }, { &Declaration::c_array, "c-array" },       { &Declaration::pointer, "pointer" },
    { &Declaration::is_union, "union" },    { &Declaration::c_variadic, "c-variadic" }, { &Declaration::local, "local" },
};

std::string flags_text(const Declaration& d) {
    std::string out;
    for (const auto& [member, name] : FLAGS)
        if (d.*member) {
            if (!out.empty()) out += ',';
            out += name;
        }
    return out;
}

std::string range_text(const Range& r) { return std::format("{}:{}-{}:{}", r.begin.line, r.begin.column, r.end.line, r.end.column); }

std::optional<Range> parse_range(std::string_view text) {
    std::uint32_t v[4] {};
    const char* p { text.data() };
    const char* const end { text.data() + text.size() };
    constexpr char separators[] { ':', '-', ':' };
    for (int i { 0 }; i < 4; ++i) {
        const auto [next, ec] = std::from_chars(p, end, v[i]);
        if (ec != std::errc {}) return std::nullopt;
        p = next;
        if (i < 3) {
            if (p == end || *p != separators[i]) return std::nullopt;
            ++p;
        }
    }
    if (p != end) return std::nullopt;
    return Range { { v[0], v[1] }, { v[2], v[3] } };
}

// The last component of a qualified name, for the IFC identity: after the last `::` outside angle
// brackets and parentheses. An operator's symbol is part of its name (`operator<<`, `operator""_kb`,
// `operator new[]`); a conversion's type is the rest of it.
std::string_view simple_name(std::string_view qualified) {
    std::size_t start { 0 };
    int depth { 0 };
    for (std::size_t i { 0 }; i < qualified.size(); ++i) {
        const char c { qualified[i] };
        if (depth == 0 && i == start && qualified.substr(i).starts_with("operator")) {
            std::size_t j { i + 8 };
            if (j < qualified.size() && qualified[j] == ' ') {
                const std::string_view rest { qualified.substr(j + 1) };
                if (!rest.starts_with("new") && !rest.starts_with("delete")) return qualified.substr(i);   // a conversion
                j += rest.starts_with("new") ? 4 : 7;
                if (qualified.substr(j).starts_with("[]")) j += 2;
            } else if (qualified.substr(j).starts_with("\"\"")) {
                j += 2;
                while (j < qualified.size() && (std::isalnum(static_cast<unsigned char>(qualified[j])) || qualified[j] == '_')) ++j;
            } else {
                while (j < qualified.size() && std::string_view { "+-*/%^&|~!=<>,[]()" }.contains(qualified[j])) ++j;
            }
            i = j - 1;
            continue;
        }
        if (c == '<' || c == '(') ++depth;
        else if ((c == '>' || c == ')') && depth > 0) --depth;
        else if (depth == 0 && c == ':' && i + 1 < qualified.size() && qualified[i + 1] == ':') {
            start = i + 2;
            ++i;
        }
    }
    return qualified.substr(start);
}

// A string as a C++ string literal's spelling, and back: the text of an attribute argument's word.
std::string quote(std::string_view s) {
    std::string out { '"' };
    for (const unsigned char c : s) {
        switch (c) {
        case '"': out += "\\\""; break;
        case '\\': out += "\\\\"; break;
        case '\n': out += "\\n"; break;
        case '\t': out += "\\t"; break;
        case '\r': out += "\\r"; break;
        default:
            if (c < 0x20 || c == 0x7f) out += std::format("\\{:03o}", c);
            else out += static_cast<char>(c);
        }
    }
    out += '"';
    return out;
}

std::optional<std::string> unquote(std::string_view s) {
    if (s.size() < 2 || s.front() != '"' || s.back() != '"') return std::nullopt;
    s = s.substr(1, s.size() - 2);
    std::string out;
    for (std::size_t i { 0 }; i < s.size(); ++i) {
        if (s[i] != '\\') {
            out += s[i];
            continue;
        }
        if (++i == s.size()) return std::nullopt;
        switch (s[i]) {
        case '"': out += '"'; break;
        case '\\': out += '\\'; break;
        case 'n': out += '\n'; break;
        case 't': out += '\t'; break;
        case 'r': out += '\r'; break;
        default: {
            if (i + 3 > s.size()) return std::nullopt;
            unsigned value { 0 };
            for (std::size_t k { i }; k < i + 3; ++k) {
                if (s[k] < '0' || s[k] > '7') return std::nullopt;
                value = value * 8 + static_cast<unsigned>(s[k] - '0');
            }
            out += static_cast<char>(value);
            i += 2;
        }
        }
    }
    return out;
}

sdk::Architecture architecture_of(std::string_view triple) {
    const std::string_view arch { triple.substr(0, triple.find('-')) };
    if (arch == "x86_64" || arch == "amd64") return sdk::Architecture::X64;
    if (arch == "aarch64" || arch == "arm64") return sdk::Architecture::ARM64;
    if (arch == "i386" || arch == "i486" || arch == "i586" || arch == "i686" || arch == "x86") return sdk::Architecture::X86;
    if (arch.starts_with("arm") || arch.starts_with("thumb")) return sdk::Architecture::ARM32;
    return sdk::Architecture::Unknown;
}

// ---- Writing -------------------------------------------------------------------------------------

template<typename T>
std::string_view bytes_of(const T& value) {
    static_assert(std::is_trivially_copyable_v<T>);
    return { reinterpret_cast<const char*>(&value), sizeof(T) };
}

// Every byte of an entry, padding too: the SDK's structures have padding, which initialization leaves
// as it was, and the same interface must give the same bytes (MC2-2-4). Their members' defaults are 0.
template<typename T>
void clear(T& value) {
    static_assert(std::is_trivially_copyable_v<T>);
    std::memset(static_cast<void*>(&value), 0, sizeof(T));
}

class Builder {
public:
    Builder() { strings_.push_back('\0'); }   // offset 0: no text

    sdk::TextOffset text(std::string_view s) {
        if (s.empty()) return sdk::TextOffset {};
        if (const auto it = offsets_.find(s); it != offsets_.end()) return sdk::TextOffset { it->second };
        const auto offset = static_cast<std::uint32_t>(strings_.size());
        strings_.append(s);
        strings_.push_back('\0');
        offsets_.emplace(std::string { s }, offset);
        return sdk::TextOffset { offset };
    }

    // Appends an entry to a partition; its index there.
    template<typename T>
    std::uint32_t add(std::string_view partition, const T& value) {
        auto& p = partition_(partition, sizeof(T));
        p.bytes.append(bytes_of(value));
        return p.count++;
    }

    // Reserves `n` entries (zeroed), for sequences laid out before they are filled.
    template<typename T>
    std::uint32_t reserve(std::string_view partition, std::uint32_t n) {
        auto& p = partition_(partition, sizeof(T));
        const std::uint32_t first { p.count };
        p.bytes.append(static_cast<std::size_t>(n) * sizeof(T), '\0');
        p.count += n;
        return first;
    }

    template<typename T>
    void set(std::string_view partition, std::uint32_t index, const T& value) {
        auto& p = partition_(partition, sizeof(T));
        std::memcpy(p.bytes.data() + static_cast<std::size_t>(index) * sizeof(T), &value, sizeof(T));
    }

    std::uint32_t count(std::string_view partition) const {
        const auto it = partitions_.find(partition);
        return it == partitions_.end() ? 0 : it->second.count;
    }

    // The file: signature, header, string table, the partitions that have entries (by name), the table
    // of contents; the header's hash over everything after the hash (the SDK's).
    std::vector<std::byte> finish(sdk::Header header) {
        std::string out;
        out.append(reinterpret_cast<const char*>(sdk::InterfaceSignature), sizeof sdk::InterfaceSignature);
        const std::size_t header_at { out.size() };
        out.append(sizeof(sdk::Header), '\0');
        align(out);
        header.string_table_bytes = sdk::ByteOffset { static_cast<std::uint32_t>(out.size()) };
        header.string_table_size = sdk::Cardinality { static_cast<std::uint32_t>(strings_.size()) };
        out.append(strings_);
        std::vector<sdk::PartitionSummaryData> toc;
        for (const auto& [name, p] : partitions_) {
            if (p.count == 0) continue;
            align(out);
            sdk::PartitionSummaryData summary {};
            clear(summary);
            summary.name = text_offset_of(name);
            summary.offset = sdk::ByteOffset { static_cast<std::uint32_t>(out.size()) };
            summary.cardinality = sdk::Cardinality { p.count };
            summary.entry_size = sdk::EntitySize { p.entry_size };
            toc.push_back(summary);
            out.append(p.bytes);
        }
        align(out);
        header.toc = sdk::ByteOffset { static_cast<std::uint32_t>(out.size()) };
        header.partition_count = sdk::Cardinality { static_cast<std::uint32_t>(toc.size()) };
        for (const auto& summary : toc) out.append(bytes_of(summary));
        std::memcpy(out.data() + header_at, &header, sizeof header);
        std::vector<std::byte> file(out.size());
        std::memcpy(file.data(), out.data(), out.size());
        constexpr std::size_t hashed { sizeof sdk::InterfaceSignature + sizeof(sdk::SHA256Hash) };
        const sdk::SHA256Hash hash { sdk::hash_bytes(file.data() + hashed, file.data() + file.size()) };
        std::memcpy(file.data() + sizeof sdk::InterfaceSignature, hash.value.data(), sizeof hash.value);
        return file;
    }

private:
    struct Partition {
        std::string bytes;
        std::uint32_t count { 0 };
        std::uint32_t entry_size { 0 };
    };
    struct Hash {
        using is_transparent = void;
        std::size_t operator()(std::string_view s) const { return std::hash<std::string_view> {}(s); }
    };
    std::string strings_;
    std::unordered_map<std::string, std::uint32_t, Hash, std::equal_to<>> offsets_;
    std::map<std::string, Partition, std::less<>> partitions_;
    // Partition names go in the string table before it is written: finish() looks them up.
    std::map<std::string, std::uint32_t, std::less<>> partition_names_;

    Partition& partition_(std::string_view name, std::size_t entry_size) {
        auto it = partitions_.find(name);
        if (it == partitions_.end()) {
            it = partitions_.emplace(std::string { name }, Partition {}).first;
            it->second.entry_size = static_cast<std::uint32_t>(entry_size);
            partition_names_.emplace(std::string { name }, sdk::to_underlying(text(name)));
        }
        return it->second;
    }

    sdk::TextOffset text_offset_of(std::string_view name) const { return sdk::TextOffset { partition_names_.find(name)->second }; }

    static void align(std::string& out) {
        while (out.size() % 4 != 0) out.push_back('\0');
    }
};

// The partitions MC2 v1 writes (their IFC names).
constexpr std::string_view P_SCOPE_DECL { "decl.scope" }, P_FUNCTION { "decl.function" }, P_METHOD { "decl.method" },
    P_VARIABLE { "decl.variable" }, P_FIELD { "decl.field" }, P_ENUM { "decl.enum" }, P_ENUMERATOR { "decl.enumerator" },
    P_ALIAS { "decl.alias" }, P_PARAMETER { "decl.parameter" }, P_CONSTRUCTOR { "decl.constructor" },
    P_DESTRUCTOR { "decl.destructor" }, P_BARREN { "decl.barren" }, P_SCOPES { "scope.desc" }, P_MEMBERS { "scope.member" },
    P_FUNDAMENTAL { "type.fundamental" }, P_CHART { "chart.unilevel" }, P_FILES { "name.source-file" }, P_LINES { "src.line" },
    P_DECL_ATTRS { ".msvc.trait.decl-attrs" }, P_ATTR_BASIC { "attr.basic" }, P_ATTR_SCOPED { "attr.scoped" },
    P_ATTR_CALLED { "attr.called" }, P_ATTR_TUPLE { "attr.tuple" }, P_ATTR_HEAP { "heap.attr" }, P_DIR_ATTRIBUTE { "dir.attribute" };

std::string_view partition_of(sdk::DeclSort sort) {
    switch (sort) {
    case sdk::DeclSort::Scope: return P_SCOPE_DECL;
    case sdk::DeclSort::Function: return P_FUNCTION;
    case sdk::DeclSort::Method: return P_METHOD;
    case sdk::DeclSort::Variable: return P_VARIABLE;
    case sdk::DeclSort::Field: return P_FIELD;
    case sdk::DeclSort::Enumeration: return P_ENUM;
    case sdk::DeclSort::Enumerator: return P_ENUMERATOR;
    case sdk::DeclSort::Alias: return P_ALIAS;
    case sdk::DeclSort::Parameter: return P_PARAMETER;
    case sdk::DeclSort::Constructor: return P_CONSTRUCTOR;
    case sdk::DeclSort::Destructor: return P_DESTRUCTOR;
    default: return P_BARREN;
    }
}

class Writer {
public:
    explicit Writer(const Interface& unit) : unit_ { unit } {}

    std::vector<std::byte> run() {
        const auto& decls = unit_.declarations;
        const std::size_t n { decls.size() };
        // Line 0 is no place (a null SourceLocation); the unit's file is the only one.
        b_.add(P_LINES, sym::FileAndLine {});
        sym::SourceFileName file {};
        clear(file);
        file.name = b_.text(unit_.source);
        b_.add(P_FILES, file);

        // Where each declaration goes: its scope (the innermost namespace or class around it; -1 the
        // global scope), its function (a parameter), its enumeration (an enumerator). The facts come
        // in the order they were found, each declaration before what it holds.
        std::vector<std::ptrdiff_t> parent(n, -1), owner(n, -1);
        std::vector<sdk::DeclSort> sort(n);
        {
            std::vector<std::size_t> open;
            for (std::size_t i { 0 }; i < n; ++i) {
                const Range& r { decls[i].range };
                while (!open.empty() && !(decls[open.back()].range.begin <= r.begin && r.end <= decls[open.back()].range.end)) open.pop_back();
                const Kind k { decls[i].kind };
                sort[i] = sort_for(k);
                for (auto it = open.rbegin(); it != open.rend(); ++it) {
                    const Kind around { decls[*it].kind };
                    if (k == Kind::parameter ? is_function(around) : k == Kind::enumerator ? around == Kind::enum_ : is_scope(around)) {
                        (k == Kind::parameter || k == Kind::enumerator ? owner : parent)[i] = static_cast<std::ptrdiff_t>(*it);
                        break;
                    }
                    if (k != Kind::parameter && k != Kind::enumerator && is_function(around)) break;
                }
                if (k == Kind::enumerator && owner[i] < 0) sort[i] = sdk::DeclSort::Barren;   // no enumeration to hold it
                if (is_scope(k) || is_function(k) || k == Kind::enum_) open.push_back(i);
            }
        }

        // Indices. Parameters and enumerators are sequences by value: each function's, each
        // enumeration's, together, in order; a parameter no function holds after them all.
        std::vector<std::uint32_t> index(n);
        std::map<sdk::DeclSort, std::uint32_t> next;
        next[sdk::DeclSort::Barren] = 1;   // barren 0 is the dialect
        std::vector<std::vector<std::size_t>> held(n);
        std::vector<std::size_t> orphans;
        for (std::size_t i { 0 }; i < n; ++i) {
            if (sort[i] == sdk::DeclSort::Parameter || sort[i] == sdk::DeclSort::Enumerator) {
                if (owner[i] >= 0) held[static_cast<std::size_t>(owner[i])].push_back(i);
                else orphans.push_back(i);
                continue;
            }
            index[i] = next[sort[i]]++;
        }
        for (std::size_t f { 0 }; f < n; ++f)
            for (const std::size_t i : held[f]) index[i] = next[sort[i]]++;
        for (const std::size_t i : orphans) index[i] = next[sort[i]]++;
        for (const auto& [s, count] : next) b_.reserve_decls(s, count);

        // Scopes: 1 is the global scope; each namespace and class its own, in order.
        std::vector<std::uint32_t> scope_of(n, 0);
        std::uint32_t scopes { 1 };
        for (std::size_t i { 0 }; i < n; ++i)
            if (sort[i] == sdk::DeclSort::Scope) scope_of[i] = ++scopes;
        std::vector<std::vector<sdk::DeclIndex>> members(scopes + 1);
        members[1].push_back(sdk::DeclIndex { sdk::DeclSort::Barren, 0 });
        for (std::size_t i { 0 }; i < n; ++i) {
            if (sort[i] == sdk::DeclSort::Parameter || sort[i] == sdk::DeclSort::Enumerator) continue;
            members[parent[i] < 0 ? 1 : scope_of[static_cast<std::size_t>(parent[i])]].push_back(sdk::DeclIndex { sort[i], index[i] });
        }
        for (std::uint32_t s { 1 }; s <= scopes; ++s) {
            sym::Scope scope {};
            clear(scope);
            scope.start = sdk::Index { b_.count(P_MEMBERS) };
            scope.cardinality = sdk::Cardinality { static_cast<std::uint32_t>(members[s].size()) };
            for (const auto& m : members[s]) b_.add(P_MEMBERS, sym::Declaration { m });
            b_.add(P_SCOPES, scope);
        }

        // The dialect: barren 0, an attribute declaration.
        {
            std::vector<sdk::AttrIndex> items;
            items.push_back(called("mc2", { std::string { MC2_VERSION } }));
            if (!unit_.target.empty()) items.push_back(called("target", { unit_.target }));
            for (const auto& p : unit_.dialect.profiles) items.push_back(called("profile", { p }));
            for (const auto& f : unit_.dialect.features) items.push_back(called("feature", { f.id, f.level }));
            for (const auto& ns : unit_.dialect.namespaces) items.push_back(called("namespace_feature", { ns.name, ns.feature, ns.level }));
            for (const auto& m : unit_.reexports) items.push_back(called("reexport", { m }));
            sym::AttributeDir dir {};
            clear(dir);
            dir.attr = tuple(items);
            const std::uint32_t d { b_.add(P_DIR_ATTRIBUTE, dir) };
            sym::BarrenDecl barren {};
            clear(barren);
            barren.directive = sdk::DirIndex { sdk::DirSort::Attribute, d };
            b_.set(P_BARREN, 0, barren);
        }

        // The declarations, and each one's [[mcxx::decl(...)]].
        std::vector<sym::trait::DeclAttributes> attributes;
        for (std::size_t i { 0 }; i < n; ++i) {
            const Declaration& d { decls[i] };
            const sdk::DeclIndex self { sort[i], index[i] };
            const sdk::DeclIndex home { parent[i] < 0 ? sdk::DeclIndex {} : sdk::DeclIndex { sort[static_cast<std::size_t>(parent[i])], index[static_cast<std::size_t>(parent[i])] } };
            const std::string_view name { simple_name(d.qualified_name) };
            const sym::SourceLocation locus { place(d.name.begin) };
            const sdk::BasicSpecifiers spec { d.exported ? sdk::BasicSpecifiers::Cxx : sdk::BasicSpecifiers::NonExported };
            switch (sort[i]) {
            case sdk::DeclSort::Scope: {
                sym::ScopeDecl s {};
                clear(s);
                s.identity = { identifier(name), locus };
                s.type = fundamental(d.kind == Kind::namespace_ ? sym::TypeBasis::Namespace
                                     : d.kind == Kind::union_   ? sym::TypeBasis::Union
                                     : d.kind == Kind::struct_  ? sym::TypeBasis::Struct
                                                                : sym::TypeBasis::Class);
                s.initializer = sdk::ScopeIndex { scope_of[i] };
                s.home_scope = home;
                s.basic_spec = spec;
                if (name.empty()) s.scope_spec = sdk::ScopeTraits::Unnamed;
                b_.set(P_SCOPE_DECL, index[i], s);
                break;
            }
            case sdk::DeclSort::Enumeration: {
                sym::EnumerationDecl e {};
                clear(e);
                e.identity = { b_.text(name), locus };
                e.type = fundamental(sym::TypeBasis::Enum);
                if (!held[i].empty()) {
                    e.initializer.start = sdk::Index { index[held[i].front()] };
                    e.initializer.cardinality = sdk::Cardinality { static_cast<std::uint32_t>(held[i].size()) };
                }
                e.home_scope = home;
                e.basic_spec = spec;
                b_.set(P_ENUM, index[i], e);
                break;
            }
            case sdk::DeclSort::Enumerator: {
                sym::EnumeratorDecl e {};
                clear(e);
                e.identity = { b_.text(name), locus };
                e.basic_spec = spec;
                b_.set(P_ENUMERATOR, index[i], e);
                break;
            }
            case sdk::DeclSort::Alias: {
                sym::AliasDecl a {};
                clear(a);
                a.identity = { b_.text(name), locus };
                a.type = fundamental(sym::TypeBasis::Typename);
                a.home_scope = home;
                a.basic_spec = spec;
                b_.set(P_ALIAS, index[i], a);
                break;
            }
            case sdk::DeclSort::Function: {
                sym::FunctionDecl f {};
                clear(f);
                f.identity = { identifier(name), locus };
                f.home_scope = home;
                f.chart = chart(held[i], index);
                f.basic_spec = spec;
                b_.set(P_FUNCTION, index[i], f);
                break;
            }
            case sdk::DeclSort::Method: {
                sym::NonStaticMemberFunctionDecl f {};
                clear(f);
                f.identity = { identifier(name), locus };
                f.home_scope = home;
                f.chart = chart(held[i], index);
                f.basic_spec = spec;
                b_.set(P_METHOD, index[i], f);
                break;
            }
            case sdk::DeclSort::Constructor: {
                sym::ConstructorDecl f {};
                clear(f);
                f.identity = { b_.text(name), locus };
                f.home_scope = home;
                f.chart = chart(held[i], index);
                f.basic_spec = spec;
                b_.set(P_CONSTRUCTOR, index[i], f);
                break;
            }
            case sdk::DeclSort::Destructor: {
                sym::DestructorDecl f {};
                clear(f);
                f.identity = { b_.text(name), locus };
                f.home_scope = home;
                f.basic_spec = spec;
                b_.set(P_DESTRUCTOR, index[i], f);
                break;
            }
            case sdk::DeclSort::Field: {
                sym::FieldDecl f {};
                clear(f);
                f.identity = { b_.text(name), locus };
                f.home_scope = home;
                f.basic_spec = spec;
                b_.set(P_FIELD, index[i], f);
                break;
            }
            case sdk::DeclSort::Variable: {
                sym::VariableDecl v {};
                clear(v);
                v.identity = { identifier(name), locus };
                v.home_scope = home;
                v.basic_spec = spec;
                b_.set(P_VARIABLE, index[i], v);
                break;
            }
            case sdk::DeclSort::Parameter: {
                sym::ParameterDecl p {};
                clear(p);
                p.identity = { b_.text(name), locus };
                p.level = 1;
                p.position = owner[i] < 0 ? 0 : position_in(held[static_cast<std::size_t>(owner[i])], i);
                p.sort = sdk::ParameterSort::Object;
                b_.set(P_PARAMETER, index[i], p);
                break;
            }
            default: {
                sym::BarrenDecl barren {};
                clear(barren);
                barren.basic_spec = spec;
                b_.set(P_BARREN, index[i], barren);
            }
            }
            std::vector<std::string> args { std::to_string(i),     kind_name(d.kind), d.entity, d.qualified_name, d.container, d.type,
                                            flags_text(d),         range_text(d.range), range_text(d.name) };
            for (const auto& t : d.templates) args.push_back(t);
            sym::trait::DeclAttributes association {};
            clear(association);
            association.entity = self;
            association.trait = called("decl", args);
            attributes.push_back(association);
        }
        std::ranges::sort(attributes, [](const auto& a, const auto& x) { return ::index_like::rep(a.entity) < ::index_like::rep(x.entity); });
        for (const auto& a : attributes) b_.add(P_DECL_ATTRS, a);

        sdk::Header header {};
        clear(header);
        header.version = sdk::FormatVersion { sdk::Version { IFC_MAJOR }, sdk::Version { IFC_MINOR } };
        header.arch = architecture_of(unit_.target);
        header.cplusplus = sdk::CPlusPlus { unit_.cplusplus };
        header.unit = sdk::UnitIndex { b_.text(unit_.module), unit_.module.contains(':') ? sdk::UnitSort::Partition : sdk::UnitSort::Primary };
        header.src_path = b_.text(unit_.source);
        header.global_scope = sdk::ScopeIndex { 1 };
        header.internal_partition = unit_.internal;
        return b_.finish(header);
    }

private:
    const Interface& unit_;
    struct Decls : Builder {
        void reserve_decls(sdk::DeclSort sort, std::uint32_t count) {
            switch (sort) {
            case sdk::DeclSort::Scope: reserve<sym::ScopeDecl>(P_SCOPE_DECL, count); break;
            case sdk::DeclSort::Function: reserve<sym::FunctionDecl>(P_FUNCTION, count); break;
            case sdk::DeclSort::Method: reserve<sym::NonStaticMemberFunctionDecl>(P_METHOD, count); break;
            case sdk::DeclSort::Variable: reserve<sym::VariableDecl>(P_VARIABLE, count); break;
            case sdk::DeclSort::Field: reserve<sym::FieldDecl>(P_FIELD, count); break;
            case sdk::DeclSort::Enumeration: reserve<sym::EnumerationDecl>(P_ENUM, count); break;
            case sdk::DeclSort::Enumerator: reserve<sym::EnumeratorDecl>(P_ENUMERATOR, count); break;
            case sdk::DeclSort::Alias: reserve<sym::AliasDecl>(P_ALIAS, count); break;
            case sdk::DeclSort::Parameter: reserve<sym::ParameterDecl>(P_PARAMETER, count); break;
            case sdk::DeclSort::Constructor: reserve<sym::ConstructorDecl>(P_CONSTRUCTOR, count); break;
            case sdk::DeclSort::Destructor: reserve<sym::DestructorDecl>(P_DESTRUCTOR, count); break;
            default: reserve<sym::BarrenDecl>(P_BARREN, count);
            }
        }
    } b_;
    std::map<std::pair<std::uint32_t, std::uint32_t>, std::uint32_t> lines_;
    std::map<sym::TypeBasis, std::uint32_t> fundamentals_;

    sdk::NameIndex identifier(std::string_view name) { return sdk::NameIndex { sdk::NameSort::Identifier, sdk::to_underlying(b_.text(name)) }; }

    sym::SourceLocation place(Position p) {
        const auto [it, added] = lines_.emplace(std::pair { 0u, p.line }, static_cast<std::uint32_t>(lines_.size() + 1));
        if (added) {
            sym::FileAndLine line {};
            clear(line);
            line.file = sdk::NameIndex { sdk::NameSort::SourceFile, 0 };
            line.line = static_cast<sdk::LineNumber>(p.line + 1);
            b_.add(P_LINES, line);
        }
        sym::SourceLocation locus {};
        clear(locus);
        locus.line = sdk::LineIndex { it->second };
        locus.column = static_cast<sdk::ColumnNumber>(p.column + 1);
        return locus;
    }

    sdk::TypeIndex fundamental(sym::TypeBasis basis) {
        auto it = fundamentals_.find(basis);
        if (it == fundamentals_.end()) {
            sym::FundamentalType t {};
            clear(t);
            t.basis = basis;
            it = fundamentals_.emplace(basis, b_.add(P_FUNDAMENTAL, t)).first;
        }
        return sdk::TypeIndex { sdk::TypeSort::Fundamental, it->second };
    }

    sdk::ChartIndex chart(const std::vector<std::size_t>& parameters, const std::vector<std::uint32_t>& index) {
        if (parameters.empty()) return sdk::ChartIndex {};
        sym::UnilevelChart c {};
        clear(c);
        c.start = sdk::Index { index[parameters.front()] };
        c.cardinality = sdk::Cardinality { static_cast<std::uint32_t>(parameters.size()) };
        return sdk::ChartIndex { sdk::ChartSort::Unilevel, b_.add(P_CHART, c) };
    }

    static std::uint32_t position_in(const std::vector<std::size_t>& list, std::size_t i) {
        return static_cast<std::uint32_t>(std::ranges::find(list, i) - list.begin()) + 1;
    }

    sym::Word word(std::string_view spelling, sdk::WordSort sort) {
        sym::Word w {};
        clear(w);
        w.text = b_.text(spelling);
        if (sort == sdk::WordSort::Literal) w.src_literal = sdk::source::Literal::String;
        else w.src_identifier = sdk::source::Identifier::Plain;
        w.algebra_sort = sort;
        return w;
    }

    sdk::AttrIndex tuple(const std::vector<sdk::AttrIndex>& items) {
        sym::TupleAttr t {};
        clear(t);
        t.start = sdk::Index { b_.count(P_ATTR_HEAP) };
        t.cardinality = sdk::Cardinality { static_cast<std::uint32_t>(items.size()) };
        for (const auto& item : items) b_.add(P_ATTR_HEAP, item);
        return sdk::AttrIndex { sdk::AttrSort::Tuple, b_.add(P_ATTR_TUPLE, t) };
    }

    // mcxx::name("arg", ...): a call whose function is a scoped name and whose arguments are a tuple
    // of string literals.
    sdk::AttrIndex called(std::string_view name, const std::vector<std::string>& args) {
        sym::ScopedAttr scoped {};
        clear(scoped);
        scoped.scope = word("mcxx", sdk::WordSort::Identifier);
        scoped.member = word(name, sdk::WordSort::Identifier);
        const sdk::AttrIndex function { sdk::AttrSort::Scoped, b_.add(P_ATTR_SCOPED, scoped) };
        std::vector<sdk::AttrIndex> items;
        for (const auto& a : args) {
            sym::BasicAttr basic {};
            clear(basic);
            basic.word = word(quote(a), sdk::WordSort::Literal);
            items.push_back(sdk::AttrIndex { sdk::AttrSort::Basic, b_.add(P_ATTR_BASIC, basic) });
        }
        sym::CalledAttr call {};
        clear(call);
        call.function = function;
        call.arguments = tuple(items);
        return sdk::AttrIndex { sdk::AttrSort::Called, b_.add(P_ATTR_CALLED, call) };
    }
};

// ---- Reading -------------------------------------------------------------------------------------

class Reader {
public:
    explicit Reader(std::span<const std::byte> bytes) : bytes_ { bytes } {}

    std::expected<Interface, std::string> run() {
        if (bytes_.size() < sizeof sdk::InterfaceSignature + sizeof(sdk::Header)) return fail("too short for an IFC header");
        if (std::memcmp(bytes_.data(), sdk::InterfaceSignature, sizeof sdk::InterfaceSignature) != 0) return fail("not an IFC file (signature)");
        std::memcpy(&header_, bytes_.data() + sizeof sdk::InterfaceSignature, sizeof header_);
        if (header_.version.major != sdk::Version { IFC_MAJOR } || header_.version.minor != sdk::Version { IFC_MINOR })
            return fail(std::format("IFC format {}.{}, MC2 v1 reads {}.{}", static_cast<int>(header_.version.major),
                                    static_cast<int>(header_.version.minor), IFC_MAJOR, IFC_MINOR));
        constexpr std::size_t hashed { sizeof sdk::InterfaceSignature + sizeof(sdk::SHA256Hash) };
        const sdk::SHA256Hash hash { sdk::hash_bytes(bytes_.data() + hashed, bytes_.data() + bytes_.size()) };
        if (std::memcmp(bytes_.data() + sizeof sdk::InterfaceSignature, hash.value.data(), sizeof hash.value) != 0)
            return fail("content hash does not match");
        const std::size_t strings_at { sdk::to_underlying(header_.string_table_bytes) };
        const std::size_t strings_size { sdk::to_underlying(header_.string_table_size) };
        if (strings_at + strings_size > bytes_.size() || strings_size == 0 || static_cast<char>(bytes_[strings_at + strings_size - 1]) != '\0')
            return fail("string table out of bounds");
        strings_ = { reinterpret_cast<const char*>(bytes_.data() + strings_at), strings_size };
        const std::size_t toc_at { sdk::to_underlying(header_.toc) };
        const std::size_t parts { sdk::to_underlying(header_.partition_count) };
        if (toc_at + parts * sizeof(sdk::PartitionSummaryData) > bytes_.size()) return fail("table of contents out of bounds");
        for (std::size_t i { 0 }; i < parts; ++i) {
            sdk::PartitionSummaryData s {};
            clear(s);
            std::memcpy(&s, bytes_.data() + toc_at + i * sizeof s, sizeof s);
            const auto name = text(s.name);
            if (!name) return fail("a partition's name is out of bounds");
            const std::size_t size { static_cast<std::size_t>(sdk::to_underlying(s.cardinality)) * sdk::to_underlying(s.entry_size) };
            if (sdk::to_underlying(s.offset) + size > bytes_.size()) return fail(std::format("partition {} out of bounds", *name));
            toc_.emplace(std::string { *name }, s);
        }

        Interface unit;
        const auto module = text(sdk::TextOffset { sdk::to_underlying(header_.unit.index()) });
        if (!module || (header_.unit.sort() != sdk::UnitSort::Primary && header_.unit.sort() != sdk::UnitSort::Partition))
            return fail("not a module interface unit");
        unit.module = *module;
        unit.internal = header_.internal_partition;
        unit.source = text(header_.src_path).value_or("");
        unit.cplusplus = sdk::to_underlying(header_.cplusplus);

        // The dialect: the global scope's attribute declarations.
        const auto global = entry<sym::Scope>(P_SCOPES, 0);
        if (!global) return fail("no global scope");
        bool versioned { false };
        for (std::uint32_t m { 0 }; m < sdk::to_underlying(global->cardinality); ++m) {
            const auto member = entry<sym::Declaration>(P_MEMBERS, sdk::to_underlying(global->start) + m);
            if (!member) return fail("a scope member out of bounds");
            if (member->index.sort() != sdk::DeclSort::Barren) continue;
            const auto barren = entry<sym::BarrenDecl>(P_BARREN, sdk::to_underlying(member->index.index()));
            if (!barren) return fail("a barren declaration out of bounds");
            if (barren->directive.sort() != sdk::DirSort::Attribute) continue;
            const auto dir = entry<sym::AttributeDir>(P_DIR_ATTRIBUTE, sdk::to_underlying(barren->directive.index()));
            if (!dir) return fail("an attribute declaration out of bounds");
            const auto items = elements(dir->attr);
            if (!items) return fail(items.error());
            for (const auto item : *items) {
                auto c = call(item);
                if (!c) return fail(c.error());
                auto& [name, args] = *c;
                const auto arity = [&](std::size_t k) { return args.size() == k; };
                if (name == "mc2" && arity(1)) {
                    if (!args[0].starts_with("1.")) return fail(std::format("MC2 {}; this reader reads 1.x", args[0]));
                    versioned = true;
                } else if (name == "target" && arity(1)) unit.target = args[0];
                else if (name == "profile" && arity(1)) unit.dialect.profiles.push_back(args[0]);
                else if (name == "feature" && arity(2)) unit.dialect.features.push_back({ args[0], args[1] });
                else if (name == "namespace_feature" && arity(3)) unit.dialect.namespaces.push_back({ args[0], args[1], args[2] });
                else if (name == "reexport" && arity(1)) unit.reexports.push_back(args[0]);
                else return fail(std::format("unknown dialect item mcxx::{} with {} arguments", name, args.size()));
            }
        }
        if (!versioned) return fail("no mcxx::mc2 version: not written by MC2");

        // The declarations: every one with an [[mcxx::decl]], back in the facts' order.
        std::vector<std::pair<std::size_t, Declaration>> found;
        const auto it = toc_.find(P_DECL_ATTRS);
        const std::uint32_t count { it == toc_.end() ? 0 : sdk::to_underlying(it->second.cardinality) };
        for (std::uint32_t i { 0 }; i < count; ++i) {
            const auto association = entry<sym::trait::DeclAttributes>(P_DECL_ATTRS, i);
            if (!association) return fail("a declaration's attribute out of bounds");
            auto c = call(association->trait);
            if (!c) return fail(c.error());
            auto& [name, args] = *c;
            if (name != "decl" || args.size() < 9) return fail(std::format("declaration attribute mcxx::{} with {} arguments", name, args.size()));
            std::size_t ordinal { 0 };
            if (std::from_chars(args[0].data(), args[0].data() + args[0].size(), ordinal).ec != std::errc {}) return fail("a declaration's ordinal: " + args[0]);
            Declaration d;
            const auto kind = kind_from(args[1]);
            if (!kind) return fail("unknown kind " + args[1]);
            d.kind = *kind;
            const sdk::DeclSort sort { association->entity.sort() };
            if (sort != sort_for(d.kind) && !(d.kind == Kind::enumerator && sort == sdk::DeclSort::Barren))
                return fail(std::format("declaration {} ({}) is a {} written as sort {}", ordinal, args[3], args[1], static_cast<int>(sort)));
            if (!declared(association->entity)) return fail(std::format("declaration {} ({}) out of bounds", ordinal, args[3]));
            d.entity = std::move(args[2]);
            d.qualified_name = std::move(args[3]);
            d.container = std::move(args[4]);
            d.type = std::move(args[5]);
            for (const auto flag : std::views::split(std::string_view { args[6] }, ',')) {
                const std::string_view f { flag.begin(), flag.end() };
                if (f.empty()) continue;
                const auto known = std::ranges::find(FLAGS, f, &std::pair<bool Declaration::*, std::string_view>::second);
                if (known == std::end(FLAGS)) return fail(std::format("declaration {}: unknown flag {}", ordinal, f));
                d.*(known->first) = true;
            }
            const auto range = parse_range(args[7]);
            const auto name_range = parse_range(args[8]);
            if (!range || !name_range) return fail(std::format("declaration {}: a range is not line:column-line:column", ordinal));
            d.range = *range;
            d.name = *name_range;
            for (std::size_t t { 9 }; t < args.size(); ++t) d.templates.push_back(std::move(args[t]));
            found.emplace_back(ordinal, std::move(d));
        }
        std::ranges::sort(found, {}, &std::pair<std::size_t, Declaration>::first);
        for (std::size_t i { 0 }; i < found.size(); ++i) {
            if (found[i].first != i) return fail(std::format("declaration ordinals are not 0..{}", found.size() - 1));
            unit.declarations.push_back(std::move(found[i].second));
        }
        return unit;
    }

private:
    std::span<const std::byte> bytes_;
    sdk::Header header_ {};
    std::string_view strings_;
    std::map<std::string, sdk::PartitionSummaryData, std::less<>> toc_;

    static std::unexpected<std::string> fail(std::string message) { return std::unexpected { std::move(message) }; }

    std::optional<std::string_view> text(sdk::TextOffset offset) const {
        const std::size_t at { sdk::to_underlying(offset) };
        if (at == 0) return std::string_view {};
        if (at >= strings_.size()) return std::nullopt;
        return std::string_view { strings_.data() + at };   // the table ends with a NUL (checked)
    }

    template<typename T>
    std::optional<T> entry(std::string_view partition, std::uint32_t index) const {
        const auto it = toc_.find(partition);
        if (it == toc_.end() || sdk::to_underlying(it->second.entry_size) != sizeof(T) || index >= sdk::to_underlying(it->second.cardinality))
            return std::nullopt;
        T value;
        std::memcpy(&value, bytes_.data() + sdk::to_underlying(it->second.offset) + static_cast<std::size_t>(index) * sizeof(T), sizeof(T));
        return value;
    }

    bool declared(sdk::DeclIndex index) const {
        const auto it = toc_.find(partition_of(index.sort()));
        return it != toc_.end() && sdk::to_underlying(index.index()) < sdk::to_underlying(it->second.cardinality);
    }

    std::expected<std::vector<sdk::AttrIndex>, std::string> elements(sdk::AttrIndex index) const {
        if (index.sort() != sdk::AttrSort::Tuple) return fail("an attribute list is not a tuple");
        const auto t = entry<sym::TupleAttr>(P_ATTR_TUPLE, sdk::to_underlying(index.index()));
        if (!t) return fail("a tuple attribute out of bounds");
        std::vector<sdk::AttrIndex> out;
        for (std::uint32_t k { 0 }; k < sdk::to_underlying(t->cardinality); ++k) {
            const auto item = entry<sdk::AttrIndex>(P_ATTR_HEAP, sdk::to_underlying(t->start) + k);
            if (!item) return fail("an attribute heap entry out of bounds");
            out.push_back(*item);
        }
        return out;
    }

    // mcxx::name("arg", ...) -> name, the arguments unquoted.
    std::expected<std::pair<std::string, std::vector<std::string>>, std::string> call(sdk::AttrIndex index) const {
        if (index.sort() != sdk::AttrSort::Called) return fail("an MC2 attribute is not a call");
        const auto c = entry<sym::CalledAttr>(P_ATTR_CALLED, sdk::to_underlying(index.index()));
        if (!c || c->function.sort() != sdk::AttrSort::Scoped) return fail("an MC2 attribute's name is not scoped");
        const auto scoped = entry<sym::ScopedAttr>(P_ATTR_SCOPED, sdk::to_underlying(c->function.index()));
        if (!scoped) return fail("a scoped attribute out of bounds");
        const auto scope = text(scoped->scope.text);
        const auto member = text(scoped->member.text);
        if (!scope || !member || *scope != "mcxx") return fail("an attribute outside mcxx::");
        const auto items = elements(c->arguments);
        if (!items) return std::unexpected { items.error() };
        std::vector<std::string> args;
        for (const auto item : *items) {
            if (item.sort() != sdk::AttrSort::Basic) return fail("an MC2 attribute's argument is not a word");
            const auto basic = entry<sym::BasicAttr>(P_ATTR_BASIC, sdk::to_underlying(item.index()));
            if (!basic || basic->word.algebra_sort != sdk::WordSort::Literal) return fail("an MC2 attribute's argument is not a literal");
            const auto spelling = text(basic->word.text);
            if (!spelling) return fail("an argument's text out of bounds");
            auto value = unquote(*spelling);
            if (!value) return fail(std::format("an argument is not a string literal: {}", *spelling));
            args.push_back(std::move(*value));
        }
        return std::pair { std::string { *member }, std::move(args) };
    }
};

} // namespace

std::vector<msa::fact::Declaration> interface_declarations(const msa::fact::Facts& facts) {
    std::vector<msa::fact::Declaration> out;
    for (const auto& d : facts.declarations)
        if (!d.local) out.push_back(d);
    return out;
}

std::vector<std::byte> write(const Interface& unit) { return Writer { unit }.run(); }

std::expected<Interface, std::string> read(std::span<const std::byte> bytes) { return Reader { bytes }.run(); }

std::string path_for(std::string_view bmi) {
    const std::size_t slash { bmi.find_last_of('/') };
    const std::size_t dot { bmi.find_last_of('.') };
    if (dot == std::string_view::npos || (slash != std::string_view::npos && dot < slash)) return std::string { bmi } + ".ifc";
    return std::string { bmi.substr(0, dot) } + ".ifc";
}

std::optional<std::string> save(const std::string& path, const Interface& unit) {
    const std::vector<std::byte> bytes { write(unit) };
    std::error_code ec;
    if (std::filesystem::file_size(path, ec) == bytes.size() && !ec) {
        std::ifstream in { path, std::ios::binary };
        std::vector<std::byte> old(bytes.size());
        if (in.read(reinterpret_cast<char*>(old.data()), static_cast<std::streamsize>(old.size())) && old == bytes) return std::nullopt;
    }
    const std::string tmp { path + ".tmp" + std::to_string(std::hash<std::thread::id> {}(std::this_thread::get_id())) };
    {
        std::ofstream out { tmp, std::ios::binary | std::ios::trunc };
        out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
        if (!out) return std::format("cannot write {}", tmp);
    }
    std::filesystem::rename(tmp, path, ec);
    if (ec) {
        std::filesystem::remove(tmp, ec);
        return std::format("cannot place {}: {}", path, ec.message());
    }
    return std::nullopt;
}

std::expected<Interface, std::string> load(const std::string& path) {
    std::ifstream in { path, std::ios::binary };
    if (!in) return std::unexpected { std::format("cannot read {}", path) };
    std::vector<char> text { std::istreambuf_iterator<char> { in }, {} };
    std::vector<std::byte> bytes(text.size());
    std::memcpy(bytes.data(), text.data(), text.size());
    auto unit = read(bytes);
    if (!unit) return std::unexpected { std::format("{}: {}", path, unit.error()) };
    return unit;
}

std::vector<std::string> differences(std::span<const msa::fact::Declaration> expected, std::span<const msa::fact::Declaration> actual,
                                     std::size_t limit) {
    std::vector<std::string> out;
    const auto note = [&](std::string line) {
        if (out.size() < limit) out.push_back(std::move(line));
    };
    if (expected.size() != actual.size()) note(std::format("{} declarations, {} read back", expected.size(), actual.size()));
    for (std::size_t i { 0 }; i < std::min(expected.size(), actual.size()); ++i) {
        const auto& e = expected[i];
        const auto& a = actual[i];
        const auto field = [&](std::string_view name, const auto& x, const auto& y, auto show) {
            if (!(x == y)) note(std::format("declaration {} ({}): {} `{}` != `{}`", i, e.qualified_name, name, show(x), show(y)));
        };
        const auto str = [](const std::string& s) { return s; };
        const auto rng = [](const Range& r) { return range_text(r); };
        const auto kind = [](Kind k) { return kind_name(k); };
        const auto flag = [](bool b) { return std::string { b ? "true" : "false" }; };
        const auto list = [](const std::vector<std::string>& v) {
            std::string s;
            for (const auto& x : v) s += (s.empty() ? "" : ",") + x;
            return s;
        };
        field("range", e.range, a.range, rng);
        field("name", e.name, a.name, rng);
        field("container", e.container, a.container, str);
        field("entity", e.entity, a.entity, str);
        field("qualified name", e.qualified_name, a.qualified_name, str);
        field("kind", e.kind, a.kind, kind);
        field("type", e.type, a.type, str);
        field("templates", e.templates, a.templates, list);
        for (const auto& [member, name] : FLAGS) field(name, e.*member, a.*member, flag);
    }
    return out;
}

namespace {

std::string hex(const sdk::SHA256Hash& hash) {
    std::string out;
    const auto* bytes = reinterpret_cast<const unsigned char*>(hash.value.data());
    for (std::size_t i { 0 }; i < sizeof hash.value; ++i) out += std::format("{:02x}", bytes[i]);
    return out;
}

std::optional<std::string> sha256_of_file(const std::string& path) {
    std::ifstream in { path, std::ios::binary };
    if (!in) return std::nullopt;
    std::vector<char> data { std::istreambuf_iterator<char> { in }, {} };
    const auto* first = reinterpret_cast<const std::byte*>(data.data());
    return hex(sdk::hash_bytes(first, first + data.size()));
}

std::string sha256_of_text(std::string_view text) {
    const auto* first = reinterpret_cast<const std::byte*>(text.data());
    return hex(sdk::hash_bytes(first, first + text.size()));
}

struct Stamp {
    std::uintmax_t size { 0 };
    std::int64_t time { 0 };
    bool operator==(const Stamp&) const = default;
};

std::optional<Stamp> stamp_of(const std::string& path) {
    std::error_code ec;
    const auto size = std::filesystem::file_size(path, ec);
    if (ec) return std::nullopt;
    const auto time = std::filesystem::last_write_time(path, ec);
    if (ec) return std::nullopt;
    return Stamp { size, static_cast<std::int64_t>(time.time_since_epoch().count()) };
}

bool copy_atomically(const std::string& from, const std::string& to) {
    std::error_code ec;
    std::filesystem::create_directories(std::filesystem::path { to }.parent_path(), ec);
    const std::string tmp { to + ".tmp" + std::to_string(std::hash<std::thread::id> {}(std::this_thread::get_id())) };
    std::filesystem::copy_file(from, tmp, std::filesystem::copy_options::overwrite_existing, ec);
    if (ec) return false;
    std::filesystem::rename(tmp, to, ec);
    if (ec) std::filesystem::remove(tmp, ec);
    return !ec;
}

// A BMI's SHA-256, remembered by its path, size and time in the store (by-path/), so a BMI is read
// whole once, not at every importer's compile.
std::optional<std::string> bmi_digest(const std::string& bmi, const Stamp& stamp) {
    const std::string memo { store_directory() + "/by-path/" + sha256_of_text(bmi) };
    if (std::ifstream in { memo }; in) {
        Stamp kept;
        std::string digest;
        if (in >> kept.size >> kept.time >> digest && kept == stamp && digest.size() == 64) return digest;
    }
    auto digest = sha256_of_file(bmi);
    if (!digest) return std::nullopt;
    std::error_code ec;
    std::filesystem::create_directories(std::filesystem::path { memo }.parent_path(), ec);
    const std::string tmp { memo + ".tmp" + std::to_string(std::hash<std::thread::id> {}(std::this_thread::get_id())) };
    if (std::ofstream out { tmp }; out) out << stamp.size << ' ' << stamp.time << ' ' << *digest << '\n';
    std::filesystem::rename(tmp, memo, ec);
    if (ec) std::filesystem::remove(tmp, ec);
    return digest;
}

std::mutex& written_lock() {
    static std::mutex m;
    return m;
}
std::vector<std::pair<std::string, std::string>>& written() {
    static std::vector<std::pair<std::string, std::string>> w;
    return w;
}

} // namespace

std::mutex& store_lock() {
    static std::mutex m;
    return m;
}
std::string& chosen_store() {
    static std::string s;
    return s;
}

void use_store(std::string directory) {
    std::lock_guard lock { store_lock() };
    chosen_store() = std::move(directory);
}

std::string store_directory() {
    {
        std::lock_guard lock { store_lock() };
        if (!chosen_store().empty()) return chosen_store();
    }
    if (const char* s = std::getenv("MCXX_IFC_STORE"); s != nullptr && *s != '\0') return s;
    if (const char* x = std::getenv("XDG_CACHE_HOME"); x != nullptr && *x != '\0') return std::string { x } + "/mcxx/ifc";
    if (const char* h = std::getenv("HOME"); h != nullptr && *h != '\0') return std::string { h } + "/.cache/mcxx/ifc";
    return (std::filesystem::temp_directory_path() / "mcxx-ifc").string();
}

void note_written(std::string bmi, std::string ifc) {
    std::lock_guard lock { written_lock() };
    written().emplace_back(std::move(bmi), std::move(ifc));
}

void publish() {
    std::vector<std::pair<std::string, std::string>> done;
    {
        std::lock_guard lock { written_lock() };
        done.swap(written());
    }
    for (const auto& [bmi, ifc] : done) {
        const auto stamp = stamp_of(bmi);
        if (!stamp) continue;   // the compile did not leave its BMI (an error after the interface was written)
        if (const auto digest = bmi_digest(bmi, *stamp)) {
            const std::string kept { store_directory() + "/" + *digest + ".ifc" };
            if (!std::filesystem::exists(kept)) (void)copy_atomically(ifc, kept);
        }
    }
}

std::shared_ptr<const Interface> interface_for(const std::string& bmi, std::string* error) {
    struct Entry {
        Stamp stamp;
        std::shared_ptr<const Interface> unit;
        std::string error;
    };
    static std::mutex lock;
    static std::unordered_map<std::string, Entry> cache;
    const auto stamp = stamp_of(bmi);
    if (!stamp) {
        if (error) *error = std::format("no BMI at {}", bmi);
        return nullptr;
    }
    {
        std::lock_guard guard { lock };
        if (const auto it = cache.find(bmi); it != cache.end() && it->second.stamp == *stamp) {
            if (error) *error = it->second.error;
            return it->second.unit;
        }
    }
    Entry entry { *stamp, nullptr, {} };
    std::string path { path_for(bmi) };
    if (!std::filesystem::exists(path)) {
        path.clear();
        if (const auto digest = bmi_digest(bmi, *stamp)) {
            const std::string kept { store_directory() + "/" + *digest + ".ifc" };
            if (std::filesystem::exists(kept)) path = kept;
        }
    }
    if (path.empty()) entry.error = std::format("no {} beside {}, and none kept for it in {}", std::filesystem::path { path_for(bmi) }.filename().string(), bmi, store_directory());
    else if (auto unit = load(path)) entry.unit = std::make_shared<const Interface>(std::move(*unit));
    else entry.error = unit.error();
    if (error) *error = entry.error;
    std::lock_guard guard { lock };
    return (cache[bmi] = std::move(entry)).unit;
}

} // namespace mcxx::ifc
