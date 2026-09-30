// What MC++'s own front end says a declaration's type names (mcxx.frontend:declared): MC3's `templates`
// and `pointer` as the Clang backend gives them for the same source -- the canonical type's, a
// specialization's defaulted arguments included, a dependent one's as written -- and what it says it
// cannot tell.
import std;
import mcxx.testing;
import mcxx.msa;
import mcxx.frontend;

namespace f = mcxx::frontend;

int main() {
    using namespace mcxx::testing;

    "a declared type's templates: aliases followed, defaults filled in, a dependent one as written"_test = [] {
        // What std's interface says (MC3 0.6.0): templates with their parameters, an alias with Clang's templates.
        using K = mcxx::msa::Kind;
        const auto decl = [](std::string qualified, K kind, std::string type = {}, std::vector<std::string> parameters = {},
                             std::vector<std::string> templates = {}) {
            mcxx::msa::fact::Declaration d;
            d.qualified_name = std::move(qualified);
            d.kind = kind;
            d.type = std::move(type);
            d.template_parameters = std::move(parameters);
            d.templates = std::move(templates);
            d.exported = true;
            return d;
        };
        f::Imported imported;
        imported.declarations = {
            decl("std::allocator", K::class_, {}, { "class _Tp" }), decl("std::char_traits", K::struct_, {}, { "class _CharT" }),
            decl("std::basic_string", K::class_, {}, { "class _CharT", "class _Traits = std::char_traits<_CharT>", "class _Allocator = std::allocator<_CharT>" }),
            decl("std::string", K::type_alias, "basic_string<char>", {}, { "std::basic_string", "std::char_traits", "std::allocator" }),
            decl("std::vector", K::class_, {}, { "class _Tp", "class _Allocator = std::allocator<_Tp>" }),
            decl("std::vector::iterator", K::type_alias, "__wrap_iter<pointer>", {}, { "std::__wrap_iter" }),
            decl("std::tuple", K::class_, {}, { "class ..._Tp" }), decl("std::array", K::struct_, {}, { "class _Tp", "size_t _Size" }),
        };
        const std::string_view source {
            "std::vector<std::string> names;\n"
            "std::vector<int*> pointers;\n"
            "std::tuple<std::string, int> both;\n"
            "std::array<std::string, 3> three;\n"
            "template <class T> struct Box { std::vector<T> items; const Box& self; };\n"
            "auto copy = names;\n"
            "auto closure = [] { return names; };\n"
            "int a = 1, b = 2;\n"
            "auto sum = a + b;\n"
            "std::vector<int>::iterator at;\n"
        };
        const auto parsed = f::parse(source);
        const auto types = f::declared_types(parsed, imported);
        const auto of = [&](std::string_view name) -> const f::DeclaredType& {
            for (std::size_t i { 0 }; i < parsed.declarations.size(); ++i)
                if (parsed.declarations[i].name == name) return types[i];
            static const f::DeclaredType none;
            return none;
        };
        using V = std::vector<std::string>;
        expect(of("names").templates == V { "std::vector", "std::basic_string", "std::char_traits", "std::allocator" }) << "the allocator's argument, a default";
        expect(of("pointers").templates == V { "std::vector", "std::allocator" } && of("pointers").pointer) << "a pointer in a defaulted argument too";
        expect(of("both").templates == V { "std::tuple" }) << "a pack is one argument, not looked into";
        expect(of("three").templates == V { "std::array", "std::basic_string", "std::char_traits", "std::allocator" }) << "a value argument is none";
        expect(of("items").templates == V { "std::vector" }) << "dependent: the arguments as written";
        expect(of("self").templates.empty() && of("self").type == "const Box<T> &") << "the injected-class-name: " << of("self").type;
        expect(!of("copy").type_certain && of("copy").why == "deduced" && of("copy").templates_certain &&
               of("copy").templates == V { "std::vector", "std::basic_string", "std::char_traits", "std::allocator" })
            << "a deduced type: its text not F1's to print, what it names known";
        expect(of("closure").templates_certain && of("closure").templates.empty()) << "a closure type names no template";
        expect(!of("sum").templates_certain) << "an operator between operands is not typed";
        expect(!of("at").templates_certain) << "a specialization's member type is its own definition's";
    };

    "an init-capture is deduced, a parameter's array written, a class defined in its declaration named"_test = [] {
        // libc++'s std module: `using std::uint64_t;`, which names the C library's `::uint64_t` (MC2 1.6.0).
        using K = mcxx::msa::Kind;
        f::Imported imported;
        mcxx::msa::fact::Declaration renamed;
        renamed.qualified_name = "std::uint64_t";
        renamed.kind = K::using_declaration;
        renamed.type = "uint64_t";
        mcxx::msa::fact::Declaration c;
        c.qualified_name = "uint64_t";
        c.kind = K::type_alias;
        c.type = "__uint64_t";
        imported.declarations = { renamed, c };
        const std::string_view source {
            "struct Session { int id; };\n"
            "Session session;\n"
            "auto run = [copied = session, &held = session] { return copied.id + held.id; };\n"
            "int main(int argc, char* argv[]) { return 0; }\n"
            "enum class Kind { a, b } kind { Kind::a };\n"
            "struct { int n; } rows[] = { { 1 }, { 2 } };\n"
            "std::uint64_t count;\n"
        };
        const auto parsed = f::parse(source);
        const auto types = f::declared_types(parsed, imported);
        const auto of = [&](std::string_view name) -> const f::DeclaredType& {
            for (std::size_t i { 0 }; i < parsed.declarations.size(); ++i)
                if (parsed.declarations[i].name == name) return types[i];
            static const f::DeclaredType none;
            return none;
        };
        expect(!of("copied").type_certain && of("copied").why == "deduced" && of("copied").templates_certain && of("copied").templates.empty())
            << "an init-capture's type is deduced as `auto`'s: " << of("copied").why;
        expect(of("held").why == "deduced" && of("held").templates_certain) << "`&x = e`: `auto&`'s";
        expect(of("argv").type_certain && of("argv").type == "char *[]" && of("argv").pointer) << "as written: " << of("argv").type;
        expect(of("kind").type_certain && of("kind").type == "enum Kind" && of("kind").templates_certain) << of("kind").type;
        expect(of("rows").type_certain && of("rows").type == "struct (unnamed)[2]" && of("rows").templates_certain) << of("rows").type;
        expect(of("count").type_certain && of("count").templates_certain && of("count").templates.empty() && !of("count").pointer)
            << "std::uint64_t through the using-declaration: " << of("count").why;
    };

    "an alias template that names no specialization is what Clang finds in the sugar"_test = [] {
        // Clang's collect_templates: a canonical specialization's template first; otherwise the alias
        // template's specialization the type is written with (libc++'s `shared_ptr<T>::get()` returns
        // `remove_extent_t<T> *`).
        const std::string_view source {
            "template <class T> using Same = T;\n"
            "template <class T> struct Box { T value; };\n"
            "template <class T> using Boxed = Box<T>;\n"
            "template <class T> struct Holder { using element_type = Same<T>; element_type* get(); };\n"
            "struct Task {};\n"
            "Same<Task> direct;\n"
            "Boxed<Task> boxed;\n"
            "Holder<Task> holder;\n"
            "auto got = holder.get();\n"
        };
        const auto parsed = f::parse(source);
        const auto types = f::declared_types(parsed, {});
        const auto of = [&](std::string_view name) -> const f::DeclaredType& {
            for (std::size_t i { 0 }; i < parsed.declarations.size(); ++i)
                if (parsed.declarations[i].name == name) return types[i];
            static const f::DeclaredType none;
            return none;
        };
        using V = std::vector<std::string>;
        expect(of("direct").templates_certain && of("direct").templates == V { "Same" }) << "the alias template itself";
        expect(of("boxed").templates == V { "Box" }) << "a specialization: its template, not the alias";
        expect(of("got").templates_certain && of("got").templates == V { "Same" } && of("got").pointer) << "deduced through the member alias";
    };

    "a file read as its module's interface carries its classes' bases and its templates' parameters"_test = [] {
        const std::string_view source {
            "export module shapes;\n"
            "export namespace shapes {\n"
            "struct Point { int x; };\n"
            "struct Circle : Point { int radius; };\n"
            "template <class T, class A = Point, unsigned N = 3, class... Rest> struct Box { T value; };\n"
            "template <class T> using Boxed = Box<T>;\n"
            "int area(const Circle& c, int scale = 1);\n"
            "enum class Level { notice, error };\n"
            "namespace geometry = shapes;\n"
            "namespace detail { struct Hidden { int v; }; }\n"
            "using detail::Hidden;\n"
            "}\n"
        };
        const auto facts = f::facts(f::parse(source), f::Imported {});
        const auto of = [&](std::string_view name) -> const mcxx::msa::fact::Declaration* {
            for (const auto& d : facts.declarations)
                if (d.qualified_name == name) return &d;
            return nullptr;
        };
        using V = std::vector<std::string>;
        expect(fatal(of("shapes::Circle") != nullptr && of("shapes::Box") != nullptr && of("shapes::Boxed") != nullptr));
        expect(of("shapes::Circle")->bases == V { "shapes::Point" }) << "MC3 0.5.0's bases, the scope the base names";
        expect(of("shapes::Box")->template_parameters == V { "class T", "class A = Point", "unsigned int N", "class ...Rest" })
            << "MC3 0.6.0's template-parameters, as the Clang backend writes them";
        expect(of("shapes::Boxed")->template_parameters == V { "class T" });
        expect(of("shapes::area") != nullptr && of("shapes::area")->parameters == V { "const Circle &", "int =" }) << "MC3 0.7.0's parameters: "
            << (of("shapes::area") && of("shapes::area")->parameters ? std::format("{}", *of("shapes::area")->parameters) : std::string { "none" });
        // MC3 0.8.0: an enumerator's type is its enumeration, a namespace alias's the namespace it names.
        expect(of("shapes::Level::error") != nullptr && of("shapes::Level::error")->kind == mcxx::msa::Kind::enumerator &&
               of("shapes::Level::error")->type == "shapes::Level" && of("shapes::Level::error")->exported);
        expect(of("shapes::geometry") != nullptr && of("shapes::geometry")->kind == mcxx::msa::Kind::namespace_alias &&
               of("shapes::geometry")->type == "shapes")
            << (of("shapes::geometry") ? of("shapes::geometry")->type : std::string { "none" });
        // A using-declaration's: what it names.
        expect(of("shapes::Hidden") != nullptr && of("shapes::Hidden")->kind == mcxx::msa::Kind::using_declaration &&
               of("shapes::Hidden")->type == "shapes::detail::Hidden" && of("shapes::Hidden")->exported)
            << (of("shapes::Hidden") ? of("shapes::Hidden")->type : std::string { "none" });
    };

    "names and types as Clang has them: friends, local classes, unnamed ones, arrays, pointers"_test = [] {
        const std::string_view source {
            "namespace ns {\n"
            "struct P {\n"
            "    int x;\n"
            "    friend bool operator<(const P& a, const P& b) { return a.x < b.x; }\n"
            "};\n"
            "void f(int n) {\n"
            "    auto g = [](int lambdas) { return lambdas; };\n"
            "    struct Local { int field; };\n"
            "    struct { int c; } unnamed;\n"
            "}\n"
            "const int table[] = { 1, 2, 3, };\n"
            "int (*callbacks[])(int) = { nullptr };\n"
            "struct S { int m; };\n"
            "int S::* member;\n"
            "void (*handler)(int*);\n"
            "}\n"
        };
        const auto facts = f::facts(f::parse(source), f::Imported {});
        const auto of = [&](std::string_view name) -> const mcxx::msa::fact::Declaration* {
            for (const auto& d : facts.declarations)
                if (d.qualified_name == name) return &d;
            return nullptr;
        };
        using K = mcxx::msa::Kind;
        expect(of("ns::operator<") != nullptr && of("ns::operator<")->kind == K::function) << "a friend is its class's namespace's";
        expect(of("a") != nullptr && of("ns::P::a") == nullptr) << "and its parameters are its own";
        expect(of("ns::f(int)::Local::field") != nullptr) << "a local class is named with its function's parameters, not a lambda's";
        expect(of("ns::f(int)::(unnamed struct)::c") != nullptr) << "an unnamed struct";
        expect(of("ns::table") != nullptr && of("ns::table")->type == "const int[3]") << "the bound its initializer gives: " << (of("ns::table") ? of("ns::table")->type : "none");
        expect(of("ns::member") != nullptr && !of("ns::member")->pointer && of("ns::member")->type == "int S::*")
            << "a member pointer is no pointer: " << (of("ns::member") ? of("ns::member")->type : "none");
        expect(of("ns::handler") != nullptr && of("ns::handler")->pointer) << "a pointer to a function is one";
    };

    return report();
}
