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

    "what it cannot parse it skips, says where, and goes on"_test = [] {
        const auto syntax = f::parse("int good1;\n) ] garbage + + ;\nint good2;\nstruct S { void f( ; int kept; };\nint good3;\n");
        const auto names = outline("int good1;\n) ] garbage + + ;\nint good2;\nstruct S { void f( ; int kept; };\nint good3;\n");
        expect(!syntax.diagnostics.empty());
        expect(std::ranges::contains(names, "variable good1") && std::ranges::contains(names, "variable good2") &&
               std::ranges::contains(names, "variable good3"));
    };

    return report();
}
