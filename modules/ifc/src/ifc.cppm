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
export module mcxx.ifc;

import std;
import mcxx.msa;

export namespace mcxx::ifc {

// 1.5.0: a function's parameters (MC3 0.7.0); 1.4.0: a template's parameters (MC3 0.6.0); 1.3.0:
// bases, and a function's return type (MC3 0.5.0); 1.2.0 adds reachable declarations, 1.1.0
// re-exports; 1.0 to 1.4 files are read too.
inline constexpr std::string_view MC2_VERSION { "1.7.0" };
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
    // (1.6.0) Its other imports: what a unit of the same module that imports it sees too
    // ([module.import]/7: an implementation unit sees what its interface imports).
    std::vector<std::string> imports;
    // What else an importer reaches through the unit (1.2.0): the declarations its exported
    // using-declarations name, the public members of the classes among them, the enumerators of the
    // enumerations it exports or reaches; exported, with no ranges (they are in other files).
    std::vector<msa::fact::Declaration> reachable;
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
