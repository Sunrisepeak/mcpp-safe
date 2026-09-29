// mcxx.msa:basics -- positions, ranges, locations; certainty; diagnostics; the kinds of entities and
// the roles an occurrence plays.
export module mcxx.msa:basics;

import std;

export namespace mcxx::msa {

struct Position {
    std::uint32_t line { 0 };
    std::uint32_t column { 0 };   // UTF-8 bytes
    auto operator<=>(const Position&) const = default;
};

struct Range {
    Position begin;
    Position end;
    bool operator==(const Range&) const = default;
    bool contains(Position p) const { return begin <= p && p <= end; }
};

struct Location {
    std::string path;   // absolute, '/'-separated
    Range range;
    bool operator==(const Location&) const = default;
};

// How sure a backend is of an answer (plan P4). `unknown` means "ask someone else", never "no".
enum class Certainty { certain, unknown };

enum class Severity { error = 1, warning = 2, information = 3, hint = 4 };

struct Note {
    Location location;
    std::string message;
};

struct Diagnostic {
    Range range;
    Severity severity { Severity::error };
    std::string message;
    std::string code;       // the backend's stable name for it, e.g. "err_undeclared_var_use"
    std::string category;   // e.g. "Semantic Issue"
    std::vector<Note> notes;
};

enum class Kind {
    unknown,
    module,
    namespace_,
    namespace_alias,
    class_,
    struct_,
    union_,
    enum_,
    enumerator,
    type_alias,
    concept_,
    function,
    method,
    constructor,
    destructor,
    conversion,
    field,
    variable,
    parameter,
    template_parameter,
    macro,
    label,
};

std::string_view to_string(Kind kind) {
    switch (kind) {
    case Kind::unknown: return "unknown";
    case Kind::module: return "module";
    case Kind::namespace_: return "namespace";
    case Kind::namespace_alias: return "namespace alias";
    case Kind::class_: return "class";
    case Kind::struct_: return "struct";
    case Kind::union_: return "union";
    case Kind::enum_: return "enum";
    case Kind::enumerator: return "enumerator";
    case Kind::type_alias: return "type alias";
    case Kind::concept_: return "concept";
    case Kind::function: return "function";
    case Kind::method: return "method";
    case Kind::constructor: return "constructor";
    case Kind::destructor: return "destructor";
    case Kind::conversion: return "conversion";
    case Kind::field: return "field";
    case Kind::variable: return "variable";
    case Kind::parameter: return "parameter";
    case Kind::template_parameter: return "template parameter";
    case Kind::macro: return "macro";
    case Kind::label: return "label";
    }
    return "unknown";
}

bool is_type(Kind kind) {
    return kind == Kind::class_ || kind == Kind::struct_ || kind == Kind::union_ || kind == Kind::enum_ || kind == Kind::type_alias;
}

bool is_callable(Kind kind) {
    return kind == Kind::function || kind == Kind::method || kind == Kind::constructor || kind == Kind::destructor ||
           kind == Kind::conversion;
}

// What an occurrence of an entity does at that place.
namespace role {
inline constexpr std::uint32_t declaration { 1u << 0 };
inline constexpr std::uint32_t definition { 1u << 1 };
inline constexpr std::uint32_t reference { 1u << 2 };
inline constexpr std::uint32_t read { 1u << 3 };
inline constexpr std::uint32_t write { 1u << 4 };
inline constexpr std::uint32_t call { 1u << 5 };
inline constexpr std::uint32_t implicit { 1u << 6 };
inline constexpr std::uint32_t overrides { 1u << 7 };
} // namespace role

} // namespace mcxx::msa
