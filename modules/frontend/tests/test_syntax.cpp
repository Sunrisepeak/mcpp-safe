// mcxx.frontend:syntax: the outline, as Clang's; what it decides without types; that it goes on.
import std;
import mcxx.testing;
import mcxx.msa;
import mcxx.frontend;

namespace f = mcxx::frontend;
namespace msa = mcxx::msa;

namespace {

// "kind name" for every symbol, depth first, children indented by a dot.
std::vector<std::string> outline(std::string_view text) {
    const auto syntax = f::parse(text);
    std::vector<std::string> out;
    const std::function<void(const std::vector<msa::Symbol>&, std::string)> walk = [&](const std::vector<msa::Symbol>& ss, std::string indent) {
        for (const auto& s : ss) {
            out.push_back(indent + std::string { msa::to_string(s.kind) } + " " + s.name);
            walk(s.children, indent + ".");
        }
    };
    walk(f::symbols(syntax), "");
    return out;
}

msa::Symbol first(std::string_view text) { return f::symbols(f::parse(text)).at(0); }

std::string text(std::string_view source, const msa::Range& r) {
    std::vector<std::size_t> starts { 0 };
    for (std::size_t i { 0 }; i < source.size(); ++i)
        if (source[i] == '\n') starts.push_back(i + 1);
    const auto at = [&](const msa::Position& p) { return starts[p.line] + p.column; };
    return std::string { source.substr(at(r.begin), at(r.end) - at(r.begin)) };
}

using list = std::vector<std::string>;

} // namespace

int main() {
    using namespace mcxx::testing;

    "classes and their members: constructors, destructors, operators, conversions, fields, statics"_test = [] {
        expect(outline(R"(export module m;
export class Widget final : public Base<int> {
public:
    Widget() = default;
    explicit Widget(int size);
    ~Widget();
    Widget& operator=(const Widget&) = delete;
    bool operator==(const Widget&) const = default;
    explicit operator bool() const noexcept;
    int size() const { return size_; }
    static constexpr int limit = 4;
private:
    int size_ { 0 }, spare : 3;
    friend class Other;
    friend void swap(Widget&, Widget&) {}
};
)") == list { "class Widget", ".constructor Widget", ".constructor Widget", ".destructor ~Widget", ".method operator=", ".method operator==",
               ".conversion operator bool", ".method size", ".variable limit", ".field size_", ".field spare" });
    };

    "namespaces nest, unnamed ones and what they hold are not listed, enums hold their enumerators"_test = [] {
        expect(outline(R"(namespace a::b { int x; }
namespace { int hidden; struct H {}; }
inline namespace v1 { enum class Color : unsigned char { red, green = 2 }; enum { anonymous }; }
namespace alias = a::b;
)") == list { "namespace a", ".namespace b", "..variable x", "namespace v1", ".enum Color", "..enumerator red", "..enumerator green",
               "namespace alias alias" });
    };

    "templates, aliases, concepts; a class template's constructors are named with its parameters"_test = [] {
        const std::string_view source { R"(template <class T, std::size_t N = 4, class... Rest>
struct Box {
    Box();
    ~Box();
    template <class U> Box(U u) requires std::convertible_to<U, T>;
};
template <class T> using Ptr = T*;
using Size = std::size_t;
template <class T> concept Small = sizeof(T) <= 8;
typedef struct { int a; } Plain;
template <class T> T twice(T t) { return t + t; }
)" };
        expect(outline(source) == list { "struct Box", ".constructor Box<T, N, Rest...>", ".destructor ~Box<T, N, Rest...>", ".constructor Box<T, N, Rest...>",
                                         "type alias Ptr", "type alias Size", "concept Small", "type alias Plain", "function twice" });
        const auto syntax = f::parse(source);
        const auto ptr = f::symbols(syntax)[1];
        expect(text(source, ptr.selection) == "using" && text(source, ptr.range) == "template <class T> using Ptr = T*");   // as Clang places them
    };

    "out-of-line members are methods, `ns::f` a function; `= default` ends the range only in a class"_test = [] {
        const std::string_view source { "namespace ns { void f(); }\nvoid ns::f() {}\nWidget::Widget() = default;\nint Widget::size() const { return 1; }\n" };
        expect(outline(source) == list { "namespace ns", ".function f", "function f", "constructor Widget", "method size" });
        const auto symbols = f::symbols(f::parse(source));
        expect(text(source, symbols[2].range) == "Widget::Widget()");
        const std::string_view in_class { "struct S { S() = default; };" };
        expect(text(in_class, first(in_class).children.at(0).range) == "S() = default");
    };

    "a parenthesized list after a name: parameters, unless it holds an expression"_test = [] {
        expect(outline(R"(std::string token(std::size_t bytes = 16);
Config load(Path path, Mode mode = {});
Widget w(42);
std::string s("text");
Engine e(nullptr, true);
void f(Callback);
int (*pointer)(int);
void (*signal(int, void (*)(int)))(int);
)") == list { "function token", "function load", "variable w", "variable s", "variable e", "function f", "variable pointer", "function signal" });
    };

    "export blocks and extern \"C\" hold declarations of the scope around them"_test = [] {
        expect(outline("export { int a; namespace n { int b; } }\nextern \"C\" { int c(void); }\nextern \"C\" int d;\n") ==
               list { "variable a", "namespace n", ".variable b", "function c", "variable d" });
    };

    "a declaration in a function's body is local; a parameter is not (MC3 0.3.0)"_test = [] {
        const auto facts = f::facts(f::parse("namespace n { int g; int f(int p) { int x = p; struct L { int m; }; return x; } }\n"));
        const auto local = [&](std::string_view qualified) {
            const auto it = std::ranges::find_if(facts.declarations, [&](const auto& d) { return d.qualified_name == qualified; });
            return it != facts.declarations.end() && it->local;
        };
        const auto named = [&](std::string_view qualified) { return std::ranges::contains(facts.declarations, qualified, &msa::fact::Declaration::qualified_name); };
        expect(!local("n::g") && !local("n::f") && named("p") && !local("p")) << "a namespace's declarations, a parameter";
        expect(local("x") && local("L")) << "a local variable, a local class";
        expect(!named("n::p") && !named("n::x")) << "what a function declares is named by its name alone, as Clang names it";
    };

    "a structured binding and an init-capture are variables; an unnamed namespace and parameter are where Clang places them"_test = [] {
        const std::string_view source { "namespace {\nvoid f(int, char* = nullptr) {\n  auto [a, b] = g();\n  for (const auto& [k, v] : m) {}\n"
                                        "  auto l = [x = h(), &y = z, w{new int}](int q) { return q; };\n}\n}\n" };
        const auto facts = f::facts(f::parse(source));
        const auto at = [&](std::string_view qualified) -> const msa::fact::Declaration* {
            const auto it = std::ranges::find(facts.declarations, qualified, &msa::fact::Declaration::qualified_name);
            return it == facts.declarations.end() ? nullptr : &*it;
        };
        const auto* ab = at("[a, b]");
        expect(ab != nullptr && ab->kind == msa::Kind::variable && ab->name.begin == msa::Position { 2, 7 } && ab->local);
        expect(at("[k, v]") != nullptr) << "in a range-based for";
        for (const auto* name : { "x", "y", "w" }) expect(at(name) != nullptr && at(name)->local) << name;
        expect(std::ranges::count(facts.allocations, false, &msa::fact::Allocation::is_delete) == 1) << "an init-capture's initializer is read";
        const auto* ns = at("(anonymous namespace)");
        expect(ns != nullptr && ns->name.begin == msa::Position { 0, 10 }) << "at its `{`";
        std::vector<msa::Position> unnamed;
        for (const auto& d : facts.declarations)
            if (d.kind == msa::Kind::parameter && d.qualified_name.empty()) unnamed.push_back(d.name.begin);
        expect(unnamed == std::vector<msa::Position> { { 1, 10 }, { 1, 18 } }) << "after the declarator: the `,`, the `=`";
    };

    "a body is a block whatever precedes its `{`; a control statement's `;` is inside its parentheses"_test = [] {
        for (const std::string_view source : { "void f() { auto t = [] { for (int i { 0 }; i < 3; ++i) { int dir {}; } }; }\n",
                                              "void f() { auto t = [] { if (int i { 0 }; i) { int dir {}; } }; }\n",
                                              "void f() { for (int i { 0 };;) { int dir {}; } }\n",
                                              "struct S { bool f() const { int dir {}; return dir; } };\n",
                                              "struct S { S() : a { 1 } { int dir {}; } int a; };\n",
                                              "auto f() noexcept -> int { int dir {}; return dir; }\n" }) {
            const auto facts = f::facts(f::parse(source));
            expect(std::ranges::contains(facts.declarations, std::string_view { "dir" }, &msa::fact::Declaration::qualified_name)) << source;
        }
    };

    "in a condition an expression is not taken for a declaration; a catch's parameter is one"_test = [] {
        const auto facts = f::facts(f::parse("void f(bool a, bool b) {\n  if (a && b) {}\n  while (a & b) {}\n  if (g(a) && h(b)) {}\n  a && b;\n"
                                             "  if (auto& r = x; r) {}\n  if (int n(3); n) {}\n  for (auto&& e : v) {}\n  try {} catch (const E& caught) {}\n"
                                             "  if (auto f = g(); f && ok(*f)) {}\n}\n"));
        std::vector<std::string> names;
        for (const auto& d : facts.declarations)
            if (d.kind == msa::Kind::variable) names.push_back(d.qualified_name);
        expect(names == std::vector<std::string> { "r", "n", "e", "caught", "f" }) << std::format("{}", names);
    };

    "what a lambda declares is local, wherever the lambda is; an alias template is at its name"_test = [] {
        const std::string_view source { "namespace n {\nconstexpr auto T { [](int p) { int table {}; for (int i { 0 }; i < 2; ++i) {} return table; }(1) };\n"
                                        "template <class T> using Vec = V<T>;\n}\n" };
        const auto facts = f::facts(f::parse(source));
        const auto find = [&](std::string_view name) { return std::ranges::find(facts.declarations, name, &msa::fact::Declaration::qualified_name); };
        expect(find("table") != facts.declarations.end() && find("table")->local && find("i")->local) << "in a lambda initializing a namespace's variable";
        expect(find("p") != facts.declarations.end() && !find("p")->local) << "a parameter is not local";
        expect(find("n::T") != facts.declarations.end() && !find("n::T")->local);
        const auto vec = find("n::Vec");
        expect(vec != facts.declarations.end() && vec->name.begin == msa::Position { 2, 25 }) << "its name, not its `using`";
    };

    "what it cannot parse it skips, says where, and goes on"_test = [] {
        const auto syntax = f::parse("int good1;\n) ] garbage + + ;\nint good2;\nstruct S { void f( ; int kept; };\nint good3;\n");
        const auto names = outline("int good1;\n) ] garbage + + ;\nint good2;\nstruct S { void f( ; int kept; };\nint good3;\n");
        expect(!syntax.diagnostics.empty());
        expect(std::ranges::contains(names, "variable good1") && std::ranges::contains(names, "variable good2") &&
               std::ranges::contains(names, "variable good3"));
    };

    return report();
}
