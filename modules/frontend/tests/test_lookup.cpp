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

    "a member access's object typed: auto from a call, a construction, a range, a copy; a field's pointer"_test = [] {
        const std::string_view source {
            "namespace ns {\n"
            "struct State { int input; int output; };\n"
            "struct Holder { State state; State* pointer; };\n"
            "State make() { return {}; }\n"
            "int f() {\n"
            "    auto s = make();\n"
            "    int a = s.input;\n"
            "    auto h = Holder {};\n"
            "    int b = h.state.output;\n"
            "    const auto& r = h;\n"
            "    int c = r.pointer->input;\n"
            "    State arr[3];\n"
            "    for (const auto& e : arr) a += e.output;\n"
            "    auto [first, second] = Holder {};\n"
            "    return a + b + c + make().input + arr[1].output + first.input + second->output;\n"
            "}\n"
            "}\n"
        };
        const auto references = f::references(f::parse(source));
        const auto at = [&](std::uint32_t line, std::uint32_t column) {
            const auto it = std::ranges::find_if(references, [&](const f::Reference& r) { return r.range.begin == mcxx::msa::Position { line, column }; });
            return it != references.end() ? it->target : std::string { "not resolved" };
        };
        expect(at(6, 14) == "ns::State::input") << "auto from a call: " << at(6, 14);
        expect(at(8, 14) == "ns::Holder::state" && at(8, 20) == "ns::State::output") << "auto from a construction, then a field's type";
        expect(at(10, 14) == "ns::Holder::pointer" && at(10, 23) == "ns::State::input") << "auto& from a copy, then -> through a pointer";
        expect(at(12, 37) == "ns::State::output") << "a range-for's element: " << at(12, 37);
        expect(at(14, 30) == "ns::State::input" && at(14, 45) == "ns::State::output") << "a call's result, a subscript's element";
        expect(at(14, 60) == "ns::State::input" && at(14, 76) == "ns::State::output") << "structured bindings: a class's fields in order: " << at(14, 60) << " " << at(14, 76);
    };

    "an iterator a container's member function returns reaches the container's element"_test = [] {
        const std::string_view source {
            "namespace std {\n"
            "template <class T> struct vector { using iterator = T*; iterator begin(); iterator find(int); T& operator[](unsigned); };\n"
            "}\n"
            "namespace ns { struct Item { int size; }; }\n"
            "int f(std::vector<ns::Item>& items) {\n"
            "    auto it = items.find(1);\n"
            "    std::vector<ns::Item>::iterator first = items.begin();\n"
            "    return it->size + first->size + (*it).size;\n"
            "}\n"
        };
        const auto references = f::references(f::parse(source));
        const auto at = [&](std::uint32_t line, std::uint32_t column) {
            const auto it = std::ranges::find_if(references, [&](const f::Reference& r) { return r.range.begin == mcxx::msa::Position { line, column }; });
            return it != references.end() ? it->target : std::string { "not resolved" };
        };
        expect(at(7, 15) == "ns::Item::size") << "auto from find(): " << at(7, 15);
        expect(at(7, 29) == "ns::Item::size") << "a declared C::iterator: " << at(7, 29);
        expect(at(7, 42) == "ns::Item::size") << "(*it).size: " << at(7, 42);
    };

    "an imported alias names its class, and a member is found in an imported class's base (MC3 0.5.0)"_test = [] {
        // What lib's MC2 1.3 interface says: an alias, a class and its base, the base's member.
        using K = mcxx::msa::Kind;
        const auto decl = [](std::string qualified, K kind, std::string type = {}, std::vector<std::string> bases = {}) {
            mcxx::msa::fact::Declaration d;
            d.qualified_name = std::move(qualified);
            d.kind = kind;
            d.type = std::move(type);
            d.bases = std::move(bases);
            d.exported = true;
            return d;
        };
        f::Imported imported;
        imported.declarations = { decl("lib", K::namespace_), decl("lib::Base", K::class_), decl("lib::Base::value", K::field, "int"),
                                  decl("lib::Base::get", K::method, "int"), decl("lib::Derived", K::class_, {}, { "lib::Base" }),
                                  decl("lib::Handle", K::type_alias, "Derived") };
        const std::string_view source {
            "int use(lib::Handle h) {\n"
            "    return h.value + h.get();\n"
            "}\n"
        };
        const auto references = f::references(f::parse(source), imported);
        const auto at = [&](std::uint32_t line, std::uint32_t column) {
            const auto it = std::ranges::find_if(references, [&](const f::Reference& r) { return r.range.begin == mcxx::msa::Position { line, column }; });
            return it != references.end() ? it->target : std::string { "not resolved" };
        };
        expect(at(1, 13) == "lib::Base::value") << "through the alias (named in lib) and Derived's base: " << at(1, 13);
        expect(at(1, 23) == "lib::Base::get") << at(1, 23);
    };

    "a member of a specialization is typed with its template's arguments, and their defaults (MC3 0.6.0)"_test = [] {
        // What std's and lib's interfaces say: templates with their parameters, members typed in them.
        using K = mcxx::msa::Kind;
        const auto decl = [](std::string qualified, K kind, std::string type = {}, std::vector<std::string> parameters = {}) {
            mcxx::msa::fact::Declaration d;
            d.qualified_name = std::move(qualified);
            d.kind = kind;
            d.type = std::move(type);
            d.template_parameters = std::move(parameters);
            d.exported = true;
            return d;
        };
        f::Imported imported;
        imported.declarations = {
            decl("std::expected", K::class_, {}, { "class _Tp", "class _Err" }), decl("std::expected::error", K::method, "const _Err &"),
            decl("std::expected::value", K::method, "_Tp &"), decl("std::expected::operator->", K::method, "_Tp *"),
            decl("std::allocator", K::class_, {}, { "class _Tp" }),
            decl("std::vector", K::class_, {}, { "class _Tp", "class _Allocator = std::allocator<_Tp>" }),
            decl("std::vector::value_type", K::type_alias, "_Tp"), decl("std::vector::reference", K::type_alias, "value_type &"),
            decl("std::vector::front", K::method, "reference"), decl("std::vector::get_allocator", K::method, "_Allocator"),
            decl("std::allocator::allocate", K::method, "_Tp *"),
            decl("lib::Run", K::struct_), decl("lib::Run::code", K::field, "int"), decl("lib::Error", K::struct_),
            decl("lib::Error::message", K::field, "int"), decl("lib::Result", K::type_alias, "std::expected<T, Error>", { "class T" }),
            decl("lib::stamp", K::function, "lib::Run"), decl("lib::stamp", K::function, "void"),
        };
        const std::string_view source {
            "int use(lib::Result<lib::Run> r, std::vector<lib::Run> v) {\n"
            "    auto s = lib::stamp();\n"
            "    return r->code + r.error().message + r.value().code + v.front().code + v.get_allocator().allocate(1)->code + s.code;\n"
            "}\n"
        };
        const auto references = f::references(f::parse(source), imported);
        const auto at = [&](std::uint32_t line, std::uint32_t column) {
            const auto it = std::ranges::find_if(references, [&](const f::Reference& r) { return r.range.begin == mcxx::msa::Position { line, column }; });
            return it == references.end() ? std::string { "not resolved" } : it->certain ? it->target : "uncertain: " + it->why;
        };
        expect(at(2, 14) == "lib::Run::code") << "operator-> of an alias template's expected: " << at(2, 14);
        expect(at(2, 31) == "lib::Error::message") << "error() gives the second argument: " << at(2, 31);
        expect(at(2, 51) == "lib::Run::code") << at(2, 51);
        expect(at(2, 68) == "lib::Run::code") << "front() through member aliases: " << at(2, 68);
        expect(at(2, 106) == "lib::Run::code") << "a defaulted argument, std::allocator<_Tp>: " << at(2, 106);
        // Overloads whose return types differ: which one is called needs the arguments' types.
        expect(at(2, 115).starts_with("uncertain")) << at(2, 115);
    };

    "a function overloaded across scopes is left to overload resolution"_test = [] {
        using K = mcxx::msa::Kind;
        const auto decl = [](std::string qualified, K kind, std::string type = {}) {
            mcxx::msa::fact::Declaration d;
            d.qualified_name = std::move(qualified);
            d.kind = kind;
            d.type = std::move(type);
            d.exported = true;
            return d;
        };
        f::Imported imported;
        // What std's interface reaches: std's own tolower and the C library's, which std re-exports.
        imported.declarations = { decl("std", K::namespace_), decl("std::tolower", K::function, "_CharT"), decl("tolower", K::function, "int"),
                                  decl("std::size", K::function, "auto") };
        const std::string_view source {
            "namespace ns {\n"
            "int to_json(int v);\n"
            "namespace { int to_json(double v) { return 0; } }\n"
            "int use() { return to_json(1.0) + std::tolower(65); }\n"
            "int other() { return use(); }\n"
            "}\n"
        };
        const auto references = f::references(f::parse(source), imported);
        const auto named = [&](std::uint32_t line, std::uint32_t column) {
            return std::ranges::any_of(references, [&](const f::Reference& r) { return r.range.begin == mcxx::msa::Position { line, column } && r.certain; });
        };
        expect(!named(3, 19)) << "to_json: the enclosing namespace's and the unnamed one's";
        expect(!named(3, 39)) << "std::tolower: std's and the C library's";
        expect(named(4, 21)) << "a function with no overload elsewhere is answered";
    };

    return report();
}
