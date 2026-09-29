// The MC++ semantic API (MSA, spec MC3): what a compiler front end knows about a C++ modules
// program, in values that name no compiler's types.
//
// Everything above a backend -- the LSP service, rules, mcppls -- is written against this module
// only; `mcxx.backend.clang` implements it over Clang today, and MC++'s own front end will implement it
// later. A backend's type never appears here (architecture plan P1).
//
// Positions: 0-based lines, columns in UTF-8 bytes. The LSP layer converts to UTF-16.
export module mcxx.msa;

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

struct Parameter {
    std::string name;
    std::string type;
    std::string default_value;
};

// One program entity, as far as the unit that was asked knows it.
struct Entity {
    std::string id;              // stable across units of one program (Clang: the USR)
    std::string name;
    std::string qualified_name;  // "ns::S::f"
    Kind kind { Kind::unknown };
    std::string signature;       // the declaration without its body, one line
    std::string type;            // the entity's type, or the aliased type
    std::string return_type;     // callables
    std::vector<Parameter> parameters;
    std::string documentation;   // the comment attached to it, markers stripped
    std::string module;          // the named module it belongs to ("" for the global module)
    std::string container;       // the enclosing entity's qualified name
    std::string access;          // "public", "protected", "private" or ""
    std::string value;           // enumerator value, constant initializer
    std::optional<Location> declaration;   // canonical (first) declaration
    std::optional<Location> definition;    // when the unit sees it
    std::string type_entity;               // id of the entity the type names (typeDefinition)
    std::optional<Location> type_location; // where that type is declared
};

struct Occurrence {
    Range range;           // the name as written
    std::string entity;    // Entity::id
    std::string name;
    Kind kind { Kind::unknown };
    std::uint32_t roles { 0 };
};

struct Symbol {
    std::string name;
    std::string detail;
    Kind kind { Kind::unknown };
    Range range;       // the whole declaration
    Range selection;   // the name
    std::vector<Symbol> children;
};

struct CompletionItem {
    std::string label;
    std::string detail;        // type, or signature
    std::string insert_text;
    std::string filter_text;
    std::string documentation;
    Kind kind { Kind::unknown };
    std::uint32_t priority { 0 };   // lower first
    bool snippet { false };
};

struct Signature {
    std::string label;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> parameters;   // [begin, end) byte offsets into label
    std::string documentation;
};

struct SignatureHelp {
    std::vector<Signature> signatures;
    std::uint32_t active_signature { 0 };
    std::uint32_t active_parameter { 0 };
};

// A symbol found by name across the program (workspace/symbol).
struct Found {
    std::string entity;
    std::string name;
    std::string container;
    Kind kind { Kind::unknown };
    Location location;
};

// What a backend is, in terms its consumers can act on without knowing how it is built.
struct BackendInfo {
    std::string name;             // the backend's own name, for status and reports
    std::string version;          // its version
    // The standard library release whose headers the backend reads as its own: a semantic kit
    // (prebuilt standard library sources for machines without a toolchain) must be this release.
    // Empty: any.
    std::string kit_stdlib_version;
};

// A compilation unit as its build describes it: the compile command of one source file.
struct Command {
    std::string directory;
    std::string file;                     // absolute
    std::vector<std::string> arguments;   // arguments[0] is the driver
};

// A module whose interface could not be built.
struct ModuleFailure {
    std::string module;
    std::string reason;   // why, in the backend's words
    std::string cause;    // the module whose own build failed: `module` itself, or one it depends on
    bool command { false };   // the backend rejected the compile command itself, before reading the code
};

// A compile command the backend rejected outright (an option it does not accept): a problem of the
// build environment, not of the code.
struct RejectedCommand {
    std::string file;
    std::string reason;   // the driver's own words
};

// How much a backend's log line matters.
enum class LogLevel { debug, info, warning, error };

// Progress a workspace reports while it prepares modules and indexes.
struct Status {
    std::size_t units { 0 };            // compile commands known
    std::size_t modules { 0 };          // named modules (and partitions) the program provides
    std::size_t modules_ready { 0 };    // with an up-to-date interface built
    std::size_t modules_failed { 0 };
    std::size_t indexed { 0 };          // units in the program index
    bool busy { false };
    std::vector<ModuleFailure> failures;
    std::size_t commands_rejected { 0 };    // every rejection seen, by module builds and parses
    std::vector<RejectedCommand> rejected;  // the first few
    // What the backend counted since it started ("modules.build", "parse.parse", ...): for reports
    // and for finding out where time and work went.
    std::vector<std::pair<std::string, std::int64_t>> counters;
};

// ---- Facts (MC3 v0) -----------------------------------------------------------------------------
//
// What the code of one file does, as values: the input of MC++'s feature gates and of every rule a
// plugin adds. T1 is what the file declares; T2 is what its code does (initializations, casts,
// allocations, pointer arithmetic, jumps, macros, uses of constructs, includes). A backend fills
// them for the file's own code only -- never for what it includes or imports -- and says how sure
// it is.
namespace fact {

// Where a fact sits: its enclosing namespace, for namespace-scoped gates ("" = global).
struct Place {
    Range range;
    std::string container;
};

struct Declaration : Place {
    Range name;
    std::string entity;               // id (Entity::id)
    std::string qualified_name;
    Kind kind { Kind::unknown };
    std::string type;                 // variables, members, parameters, aliases: the declared type (Kinds::declaration_types)
    // Every class template the declared type names, qualified without inline namespaces
    // ("std::vector", "nlohmann::basic_json"): what library control (lib:std.vector) looks at
    // (Kinds::declaration_types).
    std::vector<std::string> templates;
    bool exported { false };          // inside `export`
    bool c_array { false };           // of C array type
    bool pointer { false };           // the declared type holds a raw pointer (T*), anywhere in it
    bool is_union { false };          // a union
    bool c_variadic { false };        // a function with a C `...` parameter
    bool local { false };             // declared in a function's body (a local variable, a local class and its
                                      // members): not reachable from outside it (MC3 0.3.0)
};

enum class InitForm { default_init, copy, direct, direct_list, copy_list };

struct Initialization : Place {
    Range name;                       // the variable's or member's name
    std::string entity;
    std::string variable;             // its qualified name
    std::string type;                 // the initialized object's type
    std::string type_template;        // the class template the type specializes, or ""
    InitForm form { InitForm::default_init };
    std::string constructor;          // the constructor chosen, qualified with its parameters, or ""
    bool initializer_list_constructor { false };   // that constructor takes a std::initializer_list first
    std::uint32_t elements { 0 };     // a braced list's elements
    bool element_braced { false };    // a one-element list whose element is itself a braced list
    bool member_default { false };    // a default member initializer
    // No initializer, and default-initialization leaves the value indeterminate ([dcl.init]):
    // a local of scalar type, or of a class or array whose default-initialization does not
    // initialize it. Reading it before writing is undefined behavior.
    bool indeterminate { false };
};

enum class CastKind { static_cast_, dynamic_cast_, const_cast_, reinterpret_cast_, c_style, functional };
std::string_view to_string(CastKind kind) {
    switch (kind) {
    case CastKind::static_cast_: return "static_cast";
    case CastKind::dynamic_cast_: return "dynamic_cast";
    case CastKind::const_cast_: return "const_cast";
    case CastKind::reinterpret_cast_: return "reinterpret_cast";
    case CastKind::c_style: return "C-style cast";
    case CastKind::functional: return "functional cast";
    }
    return "cast";
}

struct Cast : Place {
    CastKind kind { CastKind::static_cast_ };
    std::string from;
    std::string to;
    bool reinterprets { false };      // what the cast does is a reinterpret_cast (also a C-style cast that does)
    bool to_scalar { false };         // to a scalar type: a functional cast `T(x)` to one is a C-style cast ([expr.type.conv])
};

struct Allocation : Place {
    bool is_delete { false };
    bool array { false };
    std::string type;
};

struct PointerArithmetic : Place {
    std::string op;                   // "+", "-", "+=", "-=", "++", "--", "[]"
    std::string type;                 // the pointer's type
};

struct Goto : Place {
    std::string label;
};

struct MacroDefinition : Place {
    std::string name;
};

// A language construct the code uses, by its name: "throw", "try", "typeid", "asm", "va_arg".
struct Use : Place {
    std::string construct;
    std::string detail;
};

// An #include in the file's own text.
struct Include : Place {
    std::string header;               // as written, with its brackets or quotes
    bool global_module_fragment { false };   // before the module declaration of a module unit (`module;` ... `module m;`)
};

// An attribute a plugin claims (MC4 §2: `[[acme::hot]]`, `[[acme::device]]`) on a declaration: its
// name, its arguments as written (string literals unquoted), and the declaration it is on, whose
// range is the attribute's reach -- a rule reads the facts inside it (plugin::subtree).
struct Attribute : Place {
    std::string name;                 // "acme::hot", as claimed
    std::vector<std::string> arguments;
    Range name_range;                 // where the attribute is written
    std::string entity;               // the declaration's id
    std::string declaration;          // and its qualified name
    Kind kind { Kind::unknown };      // and its kind
};

// [[mcpp::allow("id", ...)]] on a declaration: the gates it waives, over the declaration's range.
struct Suppression : Place {
    std::vector<std::string> ids;
    std::string entity;               // what it is attached to (its id)
    std::string declaration;          // and its qualified name, for people reading an audit
    std::string reason;               // an optional second form: [[mcpp::allow("id", "reason")]]
};

// An import of a named module (`import m;`, `export import m:p;`) in the file's own code, and what it
// brings in as the modules' MC2 interfaces say -- the .ifc beside each BMI, never their sources: the
// module and every module it re-exports, each with its dialect and its exported declarations. Their
// declarations' ranges are in their own files. A dialect boundary is decided from these (M1.2).
struct Import : Place {
    std::string module;               // as named: "legacy", "app:part"
    Range name;                       // the module's name as written
    bool exported { false };          // `export import`
    struct Interface {
        std::string module;
        bool found { false };         // its interface was read; otherwise nothing below is known
        std::vector<std::string> profiles;                        // its dialect: the profiles
        std::vector<std::pair<std::string, std::string>> levels;  // and each feature's level for code in it
        std::vector<Declaration> exported;                        // its exported T1 declarations
        // The level its own dialect gives `feature` ("" when it does not say: unknown to its catalog).
        std::string_view level(std::string_view feature) const {
            for (const auto& [id, l] : levels)
                if (id == feature) return l;
            return {};
        }
    };
    std::vector<Interface> interfaces;   // the module first, then what it re-exports
};

// The kinds of facts, as a set: what a feature is decided from (plugin::Feature::needs), and so
// what a host asks a backend to collect. A file whose gates need none of them is not walked.
enum class Kinds : std::uint32_t {
    none = 0,
    declarations = 1u << 0,
    initializations = 1u << 1,
    casts = 1u << 2,
    allocations = 1u << 3,
    pointer_arithmetic = 1u << 4,
    gotos = 1u << 5,
    macros = 1u << 6,
    uses = 1u << 7,
    includes = 1u << 8,
    suppressions = 1u << 9,
    // Every declaration's type text and the templates it names: costs more than the declarations
    // themselves, so asked for on its own. Without it, `type` is filled only for a declaration a
    // flag marks (c_array, pointer) and `templates` is empty.
    declaration_types = 1u << 10,
    attributes = 1u << 11,
    imports = 1u << 12,
    all = (1u << 13) - 1,
};
constexpr Kinds operator|(Kinds a, Kinds b) { return static_cast<Kinds>(std::to_underlying(a) | std::to_underlying(b)); }
constexpr Kinds& operator|=(Kinds& a, Kinds b) { return a = a | b; }
constexpr bool contains(Kinds set, Kinds kind) { return (std::to_underlying(set) & std::to_underlying(kind)) != 0; }

// Each kind by its MC3 name ("pointer-arithmetic"), in order.
inline constexpr std::pair<Kinds, std::string_view> KIND_NAMES[] {
    { Kinds::declarations, "declarations" },   { Kinds::initializations, "initializations" },
    { Kinds::casts, "casts" },                 { Kinds::allocations, "allocations" },
    { Kinds::pointer_arithmetic, "pointer-arithmetic" }, { Kinds::gotos, "gotos" },
    { Kinds::macros, "macros" },               { Kinds::uses, "uses" },
    { Kinds::includes, "includes" },           { Kinds::suppressions, "suppressions" },
    { Kinds::declaration_types, "declaration-types" }, { Kinds::attributes, "attributes" },
    { Kinds::imports, "imports" },
};
std::vector<std::string_view> names(Kinds set) {
    std::vector<std::string_view> out;
    for (const auto& [kind, name] : KIND_NAMES)
        if (contains(set, kind)) out.push_back(name);
    return out;
}
std::optional<Kinds> parse_kind(std::string_view name) {
    for (const auto& [kind, n] : KIND_NAMES)
        if (n == name) return kind;
    return std::nullopt;
}

struct Facts {
    Certainty certainty { Certainty::certain };
    Kinds collected { Kinds::all };   // what was looked for: a kind outside it is empty because nobody asked
    std::vector<Declaration> declarations;
    std::vector<Initialization> initializations;
    std::vector<Cast> casts;
    std::vector<Allocation> allocations;
    std::vector<PointerArithmetic> pointer_arithmetic;
    std::vector<Goto> gotos;
    std::vector<MacroDefinition> macros;
    std::vector<Use> uses;
    std::vector<Include> includes;
    std::vector<Suppression> suppressions;
    std::vector<Attribute> attributes;
    std::vector<Import> imports;
};

} // namespace fact

// A parsed file: the snapshot one version of its text produced. Immutable once returned, so any
// number of threads may read it.
class Unit {
public:
    virtual ~Unit() = default;
    virtual const std::string& path() const = 0;
    virtual std::int64_t version() const = 0;
    virtual std::string_view text() const = 0;
    virtual std::string_view module_name() const = 0;   // "m", "m:p" or "" when the file is no module unit
    virtual std::span<const Diagnostic> diagnostics() const = 0;
    // Every occurrence of a named entity in this file, ordered by position.
    virtual std::span<const Occurrence> occurrences() const = 0;
    virtual std::vector<Symbol> symbols() const = 0;
    // The entity whose name covers `at`, if any.
    virtual std::optional<Entity> entity_at(Position at) const = 0;
    // An entity this unit's AST can resolve, by id.
    virtual std::optional<Entity> entity(std::string_view id) const = 0;
    // Methods overriding the given one, as far as this unit sees them.
    virtual std::vector<Location> overriders(std::string_view id) const = 0;
    // What the file's own code declares and does (MC3 v0): computed once, on first use.
    virtual const fact::Facts& facts() const = 0;
};

// Cooperative cancellation: a long operation polls it and returns early when set.
using Cancel = std::stop_token;

// The program: its compile commands, its modules, and what is known across units.
class Workspace {
public:
    struct Options {
        std::string cache_directory;      // module interfaces and the index live here
        std::string resource_directory;   // the backend's builtin headers (Clang: lib/clang/<v>)
        unsigned workers { 0 };           // 0: a quarter of the hardware threads, at least 1
        bool background_index { true };
        // Where the backend's own log lines go (never standard output): each with its level and
        // category ("modules", "parse", "index", "complete", "workspace"). Which ones are produced is
        // MCXX_LOG's to say (mcxx.base.trace); failures always are.
        std::function<void(LogLevel level, std::string_view category, std::string_view message)> log;
        // Called, from any thread, whenever status() changed.
        std::function<void()> changed;
    };

    virtual ~Workspace() = default;

    // The program is described again: new or changed commands, units gone.
    virtual void set_commands(std::vector<Command> commands) = 0;
    virtual Status status() const = 0;

    // Parse one file at one version of its text, building the module interfaces it imports first.
    // The text is the file's from then on, for every unit that reads it (an interface edited in the
    // editor is what its importers see), until close(). Blocks; returns null only when cancelled or
    // when no command at all can be found for the file.
    virtual std::shared_ptr<const Unit> parse(const std::string& path, std::string text, std::int64_t version,
                                              Cancel cancel = {}) = 0;
    virtual std::vector<CompletionItem> complete(const std::string& path, const std::string& text, Position at,
                                                 Cancel cancel = {}) = 0;
    virtual SignatureHelp signature_help(const std::string& path, const std::string& text, Position at,
                                         Cancel cancel = {}) = 0;

    // The program index (every unit, built in the background).
    virtual std::vector<Location> definitions(std::string_view entity) const = 0;
    virtual std::vector<Location> declarations(std::string_view entity) const = 0;
    virtual std::vector<Location> references(std::string_view entity) const = 0;
    virtual std::vector<Found> find(std::string_view query, std::size_t limit) const = 0;

    // What a syntax-level reading of the text says at once, without a parse (the Clang backend: MC++'s
    // own front end): the gate findings of the features it decides, and those features -- what an
    // editor shows until the parse's diagnostics come (A1.8.3). Nothing when the backend has no such
    // reading, or nothing is gated.
    struct Quick {
        std::vector<Diagnostic> diagnostics;
        std::vector<std::string> features;   // decided here: a parse's findings of these, on an older text, are superseded
    };
    virtual Quick quick(const std::string& path, std::string_view text) {
        (void)path;
        (void)text;
        return {};
    }

    // A file the editor changed on disk (not an open buffer): its unit and dependents are stale.
    virtual void file_changed(const std::string& path) = 0;
    // The editor closed the file: its text is the disk's again.
    virtual void close(const std::string& path) = 0;
};

} // namespace mcxx::msa
