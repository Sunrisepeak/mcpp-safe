// mcxx.msa:entities -- what the service answers with: entities, occurrences, symbols, completion,
// signatures, and what a workspace reports of itself (its backend, commands, failures, status).
export module mcxx.msa:entities;

import std;
import :basics;

export namespace mcxx::msa {

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

} // namespace mcxx::msa
