// mcxx.ifc:format (internal) -- what writing and reading MC2 share: MC3's names for kinds and flags,
// the IFC sort of each kind, attribute text, the IFC builder and the partitions MC2 uses.
module;

#include <ifc/abstract-sgraph.hxx>
#include <ifc/file.hxx>

module mcxx.ifc:format;

import std;
import mcxx.msa;

namespace mcxx::ifc {

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

} // namespace mcxx::ifc
