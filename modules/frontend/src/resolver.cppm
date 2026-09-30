// mcxx.frontend partition :resolver (not exported) -- name lookup's and expression typing's working
// state over one file, shared by :lookup (references) and :declared (declarations' types).
// Declarations only; the definitions are in lookup.cpp (scopes, name lookup), typing.cpp (an
// expression's type, a specialization's members, aliases followed) and declared.cpp (what a declared
// type names: templates, pointers) (MC5 §8).
module mcxx.frontend:resolver;

import std;
import mcxx.msa;
import :lex;
import :preprocess;
import :syntax;
import :types;
import :lookup;
import :declared;

namespace mcxx::frontend::resolution {

bool scope_kind(msa::Kind k);
bool class_kind(msa::Kind k);
// What a name before `::` can be ([basic.lookup.qual]/1): a namespace, a type, a template.
bool names_scope(msa::Kind k);
bool function_kind(msa::Kind k);
// A word that never names a declaration.
bool keyword(std::string_view w);
std::string_view last_component(std::string_view q);
std::string scope_of(std::string_view q);
// A declared type's class name: cv, references, pointers and template arguments off
// ("const ns::S<int> &" -> "ns::S"). "" when it is not one (a builtin, a function type).
std::string class_name_of(std::string type);

// What a name resolves to.
struct Target {
    std::int32_t declaration { -1 };
    std::string qualified;
    msa::Kind kind { msa::Kind::unknown };
    std::uint32_t declared_at { 0 };   // a declaration of the file: its name's token (what is declared before a use)
    std::string type;                  // a variable's, a field's, a parameter's declared type; a function's return type
    std::int32_t imported { -1 };      // an imported one: its index in Imported::declarations
};

struct Bindings;

// A type as a declaration writes it, with where its names are looked up: the file's own at a token, an
// imported one in its scope; and what the template parameters it names stand for, when it is a
// member's type in a specialization (`const _Err &` of `std::expected<R, E>::error`).
struct Typed {
    std::string text;
    std::size_t at { 0 };
    std::string context;
    bool imported { false };
    std::string iterating;   // an iterator's: the type of the container it came from (its names looked up as text's are)
    std::shared_ptr<const Bindings> bindings;
    // Reached through a function template's deduction (std::move): the class is right, but Clang's
    // type keeps the template's sugar (libc++'s remove_reference_t), which MC3's templates list names.
    bool through_template { false };
};

// A template's parameters' arguments, by name: those of `owner` (a class template, an alias template).
struct Bindings {
    std::string owner;
    std::map<std::string, Typed, std::less<>> by_name;
    // The specialization they are for, as its object's type was written (`std::map<K, V>`): what a
    // member type named `iterator` among them iterates.
    std::optional<Typed> specialization;
};

// A template parameter, as MC3 0.6.0 writes one ("class T", "class ...Ts", "class A = std::allocator<T>",
// "std::size_t N", "template class C").
struct Parameter {
    enum class Sort : std::uint8_t { type, value, template_ };
    Sort sort { Sort::type };
    std::string name;
    bool pack { false };
    std::string default_type;   // a type parameter's default, as written
    std::string value_type;     // a value parameter's type, as written
};
// As MC3 0.6.0 writes it (parameter_of's inverse).
std::string written(const Parameter& p);
Parameter parameter_of(std::string_view written);

class Resolver {
public:
    Resolver(const Syntax& syntax, const Imported& imported);

    std::vector<Reference> run();
    // After run(): what may be written at a member access or a qualification being typed at `at`.
    std::optional<std::vector<Member>> members_at(msa::Position at);
    // What the using-declaration Syntax::usings[u] names: its qualified name (MC3 0.8.0).
    std::optional<std::string> using_target(std::size_t u);
    // What an imported using-declaration names (MC3 0.8.0's `type`), as in_scope() would find it.
    std::optional<Target> through_using(const Target& using_, int depth, bool scope_only);

    // A declaration's type (MC3's `type`, `templates`, `pointer`), after run().
    DeclaredType declared(std::size_t i);

private:
    const Syntax& syntax_;
    const Imported& imported_;
    const std::vector<PpToken>& t_;
    const std::vector<Declaration>& ds_;
    std::vector<std::string> qualified_;
    std::map<std::string, std::map<std::string, std::vector<Target>, std::less<>>, std::less<>> scopes_;
    std::unordered_map<std::string, std::vector<std::int32_t>> locals_;
    std::set<std::string, std::less<>> namespaces_;
    std::map<std::string, std::int32_t, std::less<>> classes_;
    std::set<std::string, std::less<>> imported_classes_;   // the classes the imports declare (their members are theirs)
    std::map<std::string, std::vector<std::string>, std::less<>> imported_bases_;   // an imported class's bases (MC3 0.5.0)
    // An imported template's parameters (MC3 0.6.0), an imported alias's templates (Clang's, canonical).
    std::map<std::string, std::vector<std::string>, std::less<>> imported_parameters_;
    std::map<std::string, std::vector<std::string>, std::less<>> imported_templates_;
    std::set<std::string, std::less<>> imported_pointers_;   // the imported aliases whose type holds a raw pointer
    // An unnamed namespace's members are its enclosing namespace's too ([namespace.unnamed]).
    std::map<std::string, std::vector<std::string>, std::less<>> transparent_;
    std::vector<std::int32_t> enclosing_;
    // The scope a class's members are named in: its qualified name, but a local class's members are
    // named through its function (`f(int)::Local::m`) while it is named `Local`.
    std::vector<std::string> members_;
    // The `->` that begins a trailing return type (a function's, a lambda's): a type follows, not a member.
    std::vector<char> trailing_;
    // What each using-directive nominates (nominated()), once.
    std::vector<std::optional<std::vector<std::string>>> nominated_;
    std::vector<std::optional<Target>> resolved_;
    std::map<std::int32_t, std::vector<std::string>> chains_;
    std::map<std::int32_t, std::vector<std::string>> bases_;
    int depth_ { 0 };   // how deep a name's resolution has gone through aliases and bases: a cycle ends
    std::map<std::size_t, std::optional<Typed>> deduced_;   // what `auto` stands for, by declaration
    std::map<std::size_t, Typed> objects_;   // a member access's object, by the member's name token
    bool deduced_placeholder_ { false };   // the name being resolved met a type F1 cannot deduce
    bool object_known_ { false };          // the member access being resolved had a typed object
    // The name being resolved names functions an overload resolution chooses among, in more than one
    // scope (an unnamed namespace's and its enclosing one's; std's and the C library's it re-exports):
    // which one needs the arguments' types, so it is not answered.
    bool overloaded_ { false };
    std::vector<Target> overload_candidates_;   // the functions of each scope, when overloaded_

    bool is(std::size_t k, Kind kind) const { return k < t_.size() && t_[k].kind == kind; }
    bool word(std::size_t k, std::string_view w) const { return k < t_.size() && t_[k].kind == Kind::raw_identifier && t_[k].spelling == w; }

    // -- lookup.cpp: scopes and name lookup

    std::size_t skip_attributes(std::size_t k) const;

    // Declared in a function (a parameter, a local, a lambda's): named by its block, not by a scope.
    bool function_local(std::size_t i) const;

    Target target_of(std::int32_t i) const;

    // A local or a parameter visible at k: declared before it, k within its block (a parameter: its
    // function or lambda); the innermost -- the latest declared -- of those.
    std::optional<Target> local(std::size_t k, const std::string& n, bool scope_only = false) const;

    // The scopes a name at k is looked up in, innermost first: classes (an out-of-line member's
    // class, its enclosing namespaces), the lexical namespaces, the global one.
    const std::vector<std::string>& chain_at(std::size_t k);

    // `n` declared in scope S, as a lookup at k sees it: a namespace's member declared before k (an
    // imported one always); a class's any member, and its bases'.
    std::optional<Target> in_scope(const std::string& scope, const std::string& n, std::size_t k, int depth = 0, bool scope_only = false);

    // A class's bases, as the scopes they name (those F1 resolves), looked up where the class is
    // declared -- not in the class, whose lookup asks its bases. Once per class.
    const std::vector<std::string>& bases_of(std::int32_t c);

    // The namespace an alias of the file names (`namespace a = b::c;`), looked up where it is written.
    std::optional<std::string> alias_target(const Target& alias);

    // A (qualified) class or namespace name as written at k: the scope it names.
    std::optional<std::string> scope_named(std::string_view written, std::size_t k);

    // A name written in an imported declaration, whose scope is `context`: its first component looked up
    // in `context` and the scopes around it, as a name written there is.
    std::optional<std::string> scope_named_in(std::string_view written, const std::string& context);

    // What a (qualified) name written in a type names -- its last component's declaration: looked up
    // where the type is written (the file's at its token, an imported one in its scope).
    std::optional<Target> named_in(std::string_view written, const Typed& where);

    // The scope a name that names one leads to: a namespace or a class itself, what a namespace alias
    // names, the class a type alias names -- the file's own looked up where it is declared, an
    // imported one in its own scope.
    std::optional<std::string> follow(const Target& found);

    // An imported class's kind (class, struct or union), as its interface says.
    msa::Kind imported_kind(const std::string& qualified) const;

    // Unqualified lookup of `n` at k: locals, then the scope chain, then using-directives and
    // using-declarations in effect.
    std::optional<Target> unqualified(const std::string& n, std::size_t k, bool scope_only = false);

    // The namespaces a using-directive at k nominates: its name looked up from where it is written.
    std::vector<std::string> nominated(const std::string& name, std::size_t k);

    // The class a member access's object has: the expression before `.` or `->` at k - 1, typed; `->`
    // through what it points to. The object is kept (objects_) for the member's type.
    std::optional<std::string> object_class(std::size_t k);

    std::optional<Target> resolve(std::size_t k);
    std::optional<Target> resolve_(std::size_t k);

    // -- typing.cpp: what an expression's type is

    // `chosen`: a function the call's arguments chose among its overloads (calls.cpp); otherwise one
    // whose overloads' return types differ is not typed.
    std::optional<Typed> typed(const Target& t, int depth = 0, bool chosen = false);

    // Whether a type names an iterator: `...iterator`, the last component of its name.
    static bool iterator_name(std::string_view text);

    // A type written `C::iterator` (`const_iterator`, ...): an iterator of C.
    Typed iterator_of(Typed type) const;

    // Whether a type's text is a placeholder (`auto`, `decltype(auto)`), however qualified.
    static bool placeholder_text(std::string_view type);

    // Whether a declaration's type is deduced: `auto` among its specifiers (`decltype(auto)` too).
    bool placeholder(const Declaration& d) const;

    // What `auto` stands for in a variable the file declares: its initializer's type (a name, a
    // call, a construction), or the element of the range a range-for's variable ranges over. Once
    // per declaration.
    std::optional<Typed> deduce(std::size_t i, int depth);

    // The type of an initializer's expression [begin, end]: a postfix expression's (expression_type),
    // what `*e` points to, `&e`'s pointer, a lambda's closure; nothing for an operator between operands.
    std::optional<Typed> initializer_type(std::size_t begin, std::size_t end, int depth);
    // A conditional expression's type, its `?` at `question`: its branches' when they have one.
    std::optional<Typed> conditional_type(std::size_t question, std::size_t end, int depth);

    // A structured binding's name (`auto [a, b] = e;`, `for (auto& [k, v] : m)`): its place among the
    // names, of what the group is deduced to be -- a pair's or a tuple's argument there, an array's
    // element, the file's own class's field in that place.
    std::optional<Typed> binding_type(std::size_t i, int depth);

    // What a structured binding's group (`[a, b]`, the declaration `group`) is deduced to be: its
    // initializer's type, or the element of its range.
    std::optional<Typed> binding_group(std::size_t group, int depth);

    // What a range-for over a value of this type gives: a sequence's element, a map's pair.
    std::optional<Typed> range_element(const Typed& type);

    // The class a type names (cv, references, pointers and arguments off; aliases followed).
    std::optional<std::string> class_of(const Typed& type);

    // A type's text without its top-level cv-qualifiers and reference.
    static std::string bare(std::string text);

    // The top-level template arguments of a type's text ("std::map<K, std::vector<V>>" -> K, std::vector<V>).
    static std::vector<std::string> arguments_of(const std::string& text);

    // `text` read where `from` is (its place, its bindings): a part of `from`'s text.
    static Typed derived(const Typed& from, std::string text);

    // The type itself, not a name for it: a bound template parameter is its argument, an alias what
    // it names (an alias template's with its arguments bound) -- pointers kept, cv and references off.
    Typed unbound(Typed type, int depth = 0);

    // A class template's (or an alias template's) parameters: an imported one's from its interface
    // (MC3 0.6.0), the file's own from its template-head. Empty when not known or not a template.
    std::vector<Parameter> parameters_of(const Target& t);
    std::vector<Parameter> parameters_of(const std::string& qualified);

    // What the parameters of `owner` (a class template) stand for in `type` (a specialization of
    // it): its arguments, then the parameters' defaults. None when `type` is not one of `owner`.
    std::shared_ptr<const Bindings> bindings_for(const Typed& type, const std::string& owner);

    // A member's type, read in its object's specialization (`object`, what the member access's
    // object has): its class's parameters bound.
    std::optional<Typed> member_typed(const Target& member, const Typed& object, int depth, bool chosen = false);

    // The type of a class's member function `name` (`operator->`, `operator[]`), in `type`.
    std::optional<Typed> operator_typed(const Typed& type, std::string_view name);

    // What `*` (or `->`) of a value of this type reaches: a pointer's pointee; the element of the
    // standard's smart pointers and optional ([unique.ptr], [util.smartptr.shared], [optional]); what
    // a class's own `operator->` gives.
    std::optional<Typed> pointee(const Typed& type);

    // What `[i]` of a value of this type is: a pointer's or an array's element; the element of the
    // standard's sequences, a map's mapped type; what a class's own `operator[]` gives.
    std::optional<Typed> element(const Typed& type);

    // The token that opens the bracket closing at `close` (`)`, `]` or `}`), or none; and the one that
    // closes the bracket opening at `open`.
    std::optional<std::size_t> opening(std::size_t close) const;
    std::optional<std::size_t> opening_forward(std::size_t open) const;

    // The type a construction names, written before the `{` or `(` at `open`: `ns::T`, `std::vector<int>`.
    std::optional<Typed> type_named_before(std::size_t open);

    // A lambda whose captures open at `introducer` (`[`), read no further than `limit`: its trailing
    // return type's `->` (0: none, its return type deduced from its body) and its body's `{` and `}`.
    struct Lambda {
        std::size_t arrow { 0 }, body { 0 }, end { 0 };
    };
    std::optional<Lambda> lambda_at(std::size_t introducer, std::size_t limit) const;
    // Whether the `[` at j begins a lambda: not a subscript's, an attribute's, `operator[]`'s, `new T[n]`'s, `delete[]`'s.
    bool introduces_lambda(std::size_t j) const;
    // What a body's first `return` (not a nested lambda's, not a local class's) gives: a lambda's
    // deduced return type.
    std::optional<Typed> first_return(std::size_t body, std::size_t end, int depth);
    // Whether the range [begin, end] is `... | std::views::split(d)` or `std::views::split(r, d)`.
    bool splits(std::size_t begin, std::size_t end) const;
    // What calling the object named at `name` gives: a lambda's closure's return type, a
    // std::function's R.
    std::optional<Typed> called(std::size_t name, int depth);

    // What a `return` at k returns: the innermost lambda's trailing return type it is in, or its
    // function's declared return type. Nothing when that is deduced.
    std::optional<Typed> returned_at(std::size_t k);

    // The type of the expression that ends at token `end`: what a member access's object has. A
    // name (a variable, a parameter, a field, `this`), a call (its function's return type), a
    // subscript (its element), a parenthesized expression. Nothing when it is not one of these.
    std::optional<Typed> expression_type(std::size_t end, int depth = 0);

    // -- calls.cpp: what a call gives, its overload chosen by its arguments

    // A call's argument, as far as F1 tells it: a literal's kind, or an expression's type.
    struct Argument {
        enum class Sort : std::uint8_t { unknown, string_literal, integer, floating, boolean, null_pointer, typed };
        Sort sort { Sort::unknown };
        std::optional<Typed> type;
    };

    // The functions a call of `callee` may be: its overloads in its scope (the same name and kind).
    std::vector<Target> overloads(const Target& callee) const;

    // A function's parameters' types (MC3 0.7.0: " =" after one with a default argument): an imported
    // one's from its interface, the file's own from its parameters. None when not known.
    std::optional<std::vector<std::string>> function_parameters(const Target& f) const;

    // The type of a call of `callee` -- its name at `callee_token`, its arguments between the
    // parentheses `open` and `close`, a member's object `object`: when its overloads' return types
    // differ, the one its arguments choose (only when exactly one fits); a function template's return
    // type written as one of its parameters' types is that argument's.
    std::optional<Typed> call_typed(const Target& callee, std::size_t open, std::size_t close, const Typed* object, int depth);

    // The arguments between `open` and `close`, each told apart.
    std::vector<Argument> arguments(std::size_t open, std::size_t close, int depth);
    // How many there are, told apart as arguments() tells them; whether one of `candidates` takes as many.
    std::size_t argument_count(std::size_t open, std::size_t close) const;
    // The type of the parameter a call's argument at `open` (a braced list: `f({ .n = 1 })`,
    // `v.push_back({ ... })`) initializes, when every overload that takes the call has one there.
    std::optional<Typed> argument_type(std::size_t open);
    bool takes(const std::vector<Target>& candidates, std::size_t count) const;

    // Of the functions `candidates` a call at `name` (its `(` next) names, the one its arguments fit,
    // when exactly one does.
    std::optional<Target> chosen_by_arguments(const std::vector<Target>& candidates, std::size_t name);

    // Whether a parameter of type `parameter` (read where `where` is) may take `argument`; a function
    // template's own parameters (`templates`) take anything.
    bool accepts(const std::string& parameter, const Typed& where, const Argument& argument, const std::vector<Parameter>& templates);

    // -- declared.cpp: what a declared type names

    // MC3's `templates` of a type and whether it `holds` a raw pointer, as Clang has them: the
    // canonical type's (aliases followed, a specialization's defaulted arguments too), a dependent
    // one's as written. False when F1 cannot tell.
    bool collect(const Typed& type, std::vector<std::string>& templates, bool& pointer, int depth = 0);

    // Whether a type names a template parameter it does not bind (one of the file's templates'),
    // or `auto`: dependent.
    bool dependent(const Typed& type, int depth = 0);

    // The class template a type written without arguments names, when it is one: the injected-class-name
    // inside it (`inside`), or a specialization whose arguments are deduced outside it.
    std::optional<Target> template_named(const Typed& type, bool& inside);
};

} // namespace mcxx::frontend::resolution
