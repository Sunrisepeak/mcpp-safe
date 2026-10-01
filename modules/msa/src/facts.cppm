// mcxx.msa:facts -- MC3's facts (specs/mc3-facts.md): what the code of one file declares and does.
export module mcxx.msa:facts;

import std;
import :basics;

export namespace mcxx::msa {

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
    // Variables, members, parameters, aliases: the declared type; a function's, a method's: its return
    // type (MC3 0.5.0) (Kinds::declaration_types).
    std::string type;
    // Every class template the declared type names, qualified without inline namespaces
    // ("std::vector", "nlohmann::basic_json"): what library control (lib:std.vector) looks at
    // (Kinds::declaration_types).
    std::vector<std::string> templates;
    // A class's direct bases, by the names MC3 gives classes -- a specialization by its template's
    // (MC3 0.5.0; Kinds::declaration_types): where a member access finds what the class inherits.
    std::vector<std::string> bases;
    // A class template's, an alias template's or (0.7.0) a function template's parameters, in order (MC3 0.6.0; Kinds::declaration_types):
    // "class T", "class ...Ts", "class A = std::allocator<T>" (the default's names fully qualified),
    // "std::size_t N" (a non-type one, default left out), "template class C" (a template template one).
    // What a member's type written with them, or a specialization's defaulted argument, stands for.
    std::vector<std::string> template_parameters;
    // A function's, a method's (a constructor's, a conversion's) parameters' types, in order, as
    // declared: "const std::string &", "int =" for one with a default argument (MC3 0.7.0;
    // Kinds::declaration_types). Present for every function-like declaration, and only for those;
    // absent where the producer did not say (before 0.7.0). What a call's arguments choose an overload by.
    std::optional<std::vector<std::string>> parameters;
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
// MC3 0.9.0: a function's control flow (Kinds::control_flow) -- its blocks, what happens in each, in
// evaluation order, and where each goes next: what an analysis on the flow layer (MC1 `flow`) reads,
// whichever front end computed it (Clang's CFG today, MCIR later). Variables are the function's own
// locals and parameters, by entity; an event on anything else is not recorded.
enum class EventKind {
    declare,     // a local comes into being: `indeterminate` when nothing initializes it ([basic.indet])
    read,        // its value is read
    write,       // it is assigned to as a whole
    address,     // its address is taken or it is bound to a reference: from here on, anything may write it
    call,        // a call: `entity` the callee (or ""), `noreturn` when it does not return
    throw_,      // a throw-expression
    return_,     // a return statement
    scope_end,   // a local's lifetime ends
};
std::string_view to_string(EventKind kind) {
    switch (kind) {
    case EventKind::declare: return "declare";
    case EventKind::read: return "read";
    case EventKind::write: return "write";
    case EventKind::address: return "address";
    case EventKind::call: return "call";
    case EventKind::throw_: return "throw";
    case EventKind::return_: return "return";
    case EventKind::scope_end: return "scope-end";
    }
    return "declare";
}
std::optional<EventKind> parse_event_kind(std::string_view name) {
    for (const auto k : { EventKind::declare, EventKind::read, EventKind::write, EventKind::address, EventKind::call, EventKind::throw_,
                          EventKind::return_, EventKind::scope_end })
        if (to_string(k) == name) return k;
    return std::nullopt;
}

struct Event {
    EventKind kind { EventKind::declare };
    Range range;
    std::string entity;               // the variable's (or the callee's) id
    std::string name;                 // the variable's name, or the callee's qualified name
    bool indeterminate { false };     // declare: default-initialization leaves it indeterminate
    bool noreturn { false };          // call: the callee does not return ([[noreturn]], [dcl.attr.noreturn])
};

struct Block {
    std::vector<Event> events;
    std::vector<std::uint32_t> successors;   // indices into Flow::blocks
};

struct Flow : Place {                 // range: the function's definition
    std::string entity;               // the function's id
    std::string function;             // and its qualified name
    // Whether the flow below was computed. A template's pattern is not (its code depends on what it is
    // instantiated with): known is false and there are no blocks -- "ask someone else", not "no flow".
    bool known { true };
    bool returns_value { false };     // its return type is not void, and it is not main or a coroutine
    bool noreturn { false };          // declared [[noreturn]]
    Range end;                        // its body's closing brace: where flowing off the end happens
    std::uint32_t entry { 0 };
    std::uint32_t exit { 0 };
    std::vector<Block> blocks;
};

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
    // Every kind but control_flow, which costs a CFG per function: a feature asks for it by name (MC3-4.1-2).
    all = (1u << 13) - 1,
    control_flow = 1u << 13,
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
    { Kinds::imports, "imports" },             { Kinds::control_flow, "control-flow" },
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
    std::vector<Flow> control_flow;   // MC3 0.9.0
};

} // namespace fact

} // namespace mcxx::msa
