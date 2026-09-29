// MC++'s own name lookup (mcxx.frontend:lookup) against what the Clang backend says the same names name
// (mcxx-probe --references on this source: every expectation here is Clang's).
import std;
import mcxx.testing;
import mcxx.msa;
import mcxx.frontend;

namespace f = mcxx::frontend;

int main() {
    using namespace mcxx::testing;

    "each name a file writes resolves to what Clang resolves it to"_test = [] {
        const std::string_view source {
            "namespace std { template <class T> struct unique_ptr { T* operator->() const; }; }\n"
            "namespace outer {\n"
            "struct Base { int inherited; };\n"
            "struct Widget : Base {\n"
            "    int size;\n"
            "    enum Color { red, green };\n"
            "    enum class Mode { fast, slow };\n"
            "    Widget(int s) : size(s) {}\n"
            "    int area() const { return size * inherited; }\n"
            "    int twice() const;\n"
            "    Color color = red;\n"
            "    Mode mode = Mode::fast;\n"
            "};\n"
            "int Widget::twice() const { return this->size + area(); }\n"
            "namespace { int hidden = 3; }\n"
            "namespace inner { int value = hidden; }\n"
            "template <class T> T identity(T v) { return v; }\n"
            "template <class T> struct Box { T item; T get() const { return item; } };\n"
            "}\n"
            "int use(int n) {\n"
            "    using namespace outer;\n"
            "    namespace in = outer::inner;\n"
            "    Widget w { n };\n"
            "    int x = w.size + in::value;\n"
            "    {\n"
            "        int x = n;\n"
            "        x += w.area();\n"
            "    }\n"
            "    std::unique_ptr<outer::Widget> p;\n"
            "    auto f = [y = x](int z) { return y + z; };\n"
            "    outer::Box<int> b { x };\n"
            "    return x + p->size + f(outer::identity(b.get())) + outer::Widget::red;\n"
            "}\n"
        };
        const auto syntax = f::parse(source);
        const auto references = f::references(syntax);
        struct Expected { std::uint32_t line, column; std::string_view name, target; mcxx::msa::Kind kind; };
        const std::vector<Expected> expected {
            { 0, 55, "T", "std::unique_ptr::T", mcxx::msa::Kind::template_parameter },
            { 3, 16, "Base", "outer::Base", mcxx::msa::Kind::struct_ },
            { 7, 4, "Widget", "outer::Widget", mcxx::msa::Kind::struct_ },
            { 7, 20, "size", "outer::Widget::size", mcxx::msa::Kind::field },
            { 7, 25, "s", "s", mcxx::msa::Kind::parameter },
            { 8, 30, "size", "outer::Widget::size", mcxx::msa::Kind::field },
            { 8, 37, "inherited", "outer::Base::inherited", mcxx::msa::Kind::field },
            { 10, 4, "Color", "outer::Widget::Color", mcxx::msa::Kind::enum_ },
            { 10, 18, "red", "outer::Widget::red", mcxx::msa::Kind::enumerator },
            { 11, 4, "Mode", "outer::Widget::Mode", mcxx::msa::Kind::enum_ },
            { 11, 16, "Mode", "outer::Widget::Mode", mcxx::msa::Kind::enum_ },
            { 11, 22, "fast", "outer::Widget::Mode::fast", mcxx::msa::Kind::enumerator },
            { 13, 4, "Widget", "outer::Widget", mcxx::msa::Kind::struct_ },
            { 13, 41, "size", "outer::Widget::size", mcxx::msa::Kind::field },
            { 13, 48, "area", "outer::Widget::area", mcxx::msa::Kind::method },
            { 15, 30, "hidden", "outer::(anonymous namespace)::hidden", mcxx::msa::Kind::variable },
            { 16, 19, "T", "T", mcxx::msa::Kind::template_parameter },
            { 16, 30, "T", "T", mcxx::msa::Kind::template_parameter },
            { 16, 44, "v", "v", mcxx::msa::Kind::parameter },
            { 17, 32, "T", "outer::Box::T", mcxx::msa::Kind::template_parameter },
            { 17, 40, "T", "outer::Box::T", mcxx::msa::Kind::template_parameter },
            { 17, 63, "item", "outer::Box::item", mcxx::msa::Kind::field },
            { 20, 20, "outer", "outer", mcxx::msa::Kind::namespace_ },
            { 21, 19, "outer", "outer", mcxx::msa::Kind::namespace_ },
            { 21, 26, "inner", "outer::inner", mcxx::msa::Kind::namespace_ },
            { 22, 4, "Widget", "outer::Widget", mcxx::msa::Kind::struct_ },
            { 22, 15, "n", "n", mcxx::msa::Kind::parameter },
            { 23, 12, "w", "w", mcxx::msa::Kind::variable },
            { 23, 14, "size", "outer::Widget::size", mcxx::msa::Kind::field },
            { 23, 21, "in", "in", mcxx::msa::Kind::namespace_alias },
            { 23, 25, "value", "outer::inner::value", mcxx::msa::Kind::variable },
            { 25, 16, "n", "n", mcxx::msa::Kind::parameter },
            { 26, 8, "x", "x", mcxx::msa::Kind::variable },
            { 26, 13, "w", "w", mcxx::msa::Kind::variable },
            { 26, 15, "area", "outer::Widget::area", mcxx::msa::Kind::method },
            { 28, 9, "unique_ptr", "std::unique_ptr", mcxx::msa::Kind::struct_ },
            { 28, 20, "outer", "outer", mcxx::msa::Kind::namespace_ },
            { 28, 27, "Widget", "outer::Widget", mcxx::msa::Kind::struct_ },
            { 29, 18, "x", "x", mcxx::msa::Kind::variable },
            { 29, 37, "y", "y", mcxx::msa::Kind::variable },
            { 29, 41, "z", "z", mcxx::msa::Kind::parameter },
            { 30, 11, "Box", "outer::Box", mcxx::msa::Kind::struct_ },
            { 30, 24, "x", "x", mcxx::msa::Kind::variable },
            { 31, 11, "x", "x", mcxx::msa::Kind::variable },
            { 31, 15, "p", "p", mcxx::msa::Kind::variable },
            { 31, 18, "size", "outer::Widget::size", mcxx::msa::Kind::field },
            { 31, 25, "f", "f", mcxx::msa::Kind::variable },
            { 31, 27, "outer", "outer", mcxx::msa::Kind::namespace_ },
            { 31, 34, "identity", "outer::identity", mcxx::msa::Kind::function },
            { 31, 43, "b", "b", mcxx::msa::Kind::variable },
            { 31, 45, "get", "outer::Box::get", mcxx::msa::Kind::method },
            { 31, 55, "outer", "outer", mcxx::msa::Kind::namespace_ },
            { 31, 62, "Widget", "outer::Widget", mcxx::msa::Kind::struct_ },
            { 31, 70, "red", "outer::Widget::red", mcxx::msa::Kind::enumerator },
        };
        for (const auto& e : expected) {
            const auto it = std::ranges::find_if(references, [&](const f::Reference& r) { return r.range.begin == mcxx::msa::Position { e.line, e.column }; });
            expect(it != references.end() && it->target == e.target && it->kind == e.kind)
                << std::format("{}:{} {}: {} (Clang: {})", e.line + 1, e.column + 1, e.name, it != references.end() ? it->target : "not resolved", e.target);
        }
        expect(references.size() >= expected.size());
    };

    return report();
}
