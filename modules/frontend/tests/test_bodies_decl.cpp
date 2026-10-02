// mcxx.frontend:bodies, what the outline hands over: function bodies with constructor initializers and
// function-try-blocks, defaulted and deleted functions, initializers, default arguments, enumerators,
// static_assert, template bodies; and lambdas, requires-expressions and types as a body writes them.
import std;
import mcxx.testing;
import mcxx.msa;
import mcxx.frontend;

namespace f = mcxx::frontend;
namespace ast = mcxx::frontend::ast;

namespace {

std::string body(std::string_view code, f::BodyOptions options = {}) {
    const auto fragment = f::parse_fragment(code, options);
    const auto& tree = fragment.tree;
    if (tree.roots.empty()) return "<no root>";
    const auto* function = tree.statement(tree.roots[0].node);
    if (function == nullptr || function->kind != ast::StmtKind::function_body || !function->a) return "<no body>";
    std::string out;
    for (const auto h : tree.list(tree[function->a].list)) out += (out.empty() ? "" : " ") + ast::dump(tree, h);
    return out;
}

// What every root of a file reads as, one per root, joined with " | ".
std::string parts(std::string_view text, f::BodyOptions options = {}) {
    const auto syntax = f::parse(text);
    const auto tree = f::parse_bodies(syntax, options);
    std::string out;
    for (const auto& root : tree.roots) out += (out.empty() ? "" : " | ") + ast::dump(tree, root.node);
    return out;
}

void same(const std::string& actual, std::string_view expected, std::source_location where = std::source_location::current()) {
    mcxx::testing::expect(actual == expected, where) << "\n    got:      " << actual << "\n    expected: " << expected;
}

} // namespace

int main() {
    using namespace mcxx::testing;

    "a function's body: a block, constructor initializers, a function-try-block, = default, = delete, = 0"_test = [] {
        same(parts("int f(int a) { return a + 1; }"), "(function-body (compound (return (binary + (id a) (integer 1)))))");
        same(parts("struct S { int a, b; S() : a(1), b{2} { } };"), "(function-body (member-init a (integer 1)) (member-init-brace b (integer 2)) (compound))");
        same(parts("struct S : B { S(int x) : B(x), m(x...) {} int m; };"), "(function-body (member-init B (id x)) (member-init m (pack-expansion (id x))) (compound))");
        same(parts("S::S() try : a(1) { f(); } catch (...) { g(); }"), "(function-body (member-init a (integer 1)) (try (compound (expression (call (id f)))) (handler ... (compound (expression (call (id g)))))))");
        same(parts("struct S { S() = default; S(const S&) = delete; virtual void f() = 0; };"), "(function-body default) | (function-body delete) | (function-body pure)");
        same(parts("void f() noexcept(true) { }"), "(boolean true) | (function-body (compound))");
    };

    "initializers, default arguments, enumerators, bit-fields, static_assert are roots too"_test = [] {
        same(parts("int x = 1 + 2; int y{3}; int z(4);"), "(binary + (integer 1) (integer 2)) | (init-list (integer 3)) | (paren-list (integer 4))");
        same(parts("void f(int a = g(1), int b = h<int, 3>());"), "(call (id g) (integer 1)) | (call (id h<(builtin int), (integer 3)>))");
        same(parts("enum E { a = 1, b = a << 2, c };"), "(integer 1) | (binary << (id a) (integer 2))");
        same(parts("struct S { int bits : 3 + 1; int n = f(); };"), "(binary + (integer 3) (integer 1)) | (call (id f))");
        same(parts("static_assert(sizeof(int) == 4); static_assert(true, \"m\");"), "(static-assert (binary == (sizeof (builtin int)) (integer 4))) | (static-assert (boolean true) (string \"m\"))");
        same(parts("template <class T> concept C = requires(T t) { t.f(); } && sizeof(T) > 1;"), "(binary && (requires ((parameter t (named T))) (simple (call (member . (id t) f)))) (binary > (sizeof (named T)) (integer 1)))");
        same(parts("const int k = a < b ? 1 : 2, m = f<1, 2>(3);"), "(conditional (binary < (id a) (id b)) (integer 1) (integer 2)) | (call (id f<(integer 1), (integer 2)>) (integer 3))");
    };

    "a template's bodies know its parameters: T is a type, N a value, TT a template"_test = [] {
        same(parts("template <class T, int N> void f() { T * p; N * q; }"), "(function-body (compound (declaration (variable p (pointer (named T)))) (expression (binary * (id N) (id q)))))");
        same(parts("template <template <class> class TT, class U> void g() { TT<U> x; }"), "(function-body (compound (declaration (variable x (named TT<(named U)>)))))");
        same(parts("template <class T> struct S { T v; S() : v() {} };"), "(function-body (member-init v) (compound))");
        same(parts("template <std::integral T> T twice(T t) { return t + t; }"), "(function-body (compound (return (binary + (id t) (id t)))))");
        same(parts("template <class... Ts> void h(Ts... ts) { f(ts...); (g(ts), ...); }"), "(function-body (compound (expression (call (id f) (pack-expansion (id ts)))) (expression (fold , right (call (id g) (id ts))))))");
    };

    "lambdas: every capture form, parameters, specifiers, return type, template parameters"_test = [] {
        same(body("auto l = [] {};"), "(declaration (variable l (auto) = (lambda [] (function _ ()) (compound))))");
        same(body("auto l = [=] { return a; }; auto m = [&] { return b; };"), "(declaration (variable l (auto) = (lambda [(capture =)] (function _ ()) (compound (return (id a)))))) (declaration (variable m (auto) = (lambda [(capture &)] (function _ ()) (compound (return (id b))))))");
        same(body("auto l = [a, &b, this, *this, c = 1, &d = e, ...ps, ...qs = f()] {};"), "(declaration (variable l (auto) = (lambda [(capture a), (capture &b), (capture this), (capture *this), (capture c = (integer 1)), (capture &d = (id e)), (capture ps pack), (capture qs pack = (call (id f)))] (function _ ()) (compound))))");
        same(body("auto l = [x](int a, auto b) mutable noexcept -> int { return a; };"), "(declaration (variable l (auto) = (lambda [(capture x)] ((parameter a (builtin int)) (parameter b (auto))) (function (builtin int) ((parameter a (builtin int)) (parameter b (auto))) noexcept) mutable (compound (return (id a))))))");
        same(body("auto l = []<class T>(T t) requires std::integral<T> { return t; };"), "(declaration (variable l (auto) = (lambda [] <(template-parameter T)> ((parameter t (named T))) (function _ ((parameter t (named T)))) requires (id std::integral<(named T)>) (compound (return (id t))))))");
        same(body("auto l = [](int a) constexpr { return a; }; auto m = [] static { };"), "(declaration (variable l (auto) = (lambda [] ((parameter a (builtin int))) (function _ ((parameter a (builtin int)))) constexpr (compound (return (id a)))))) (declaration (variable m (auto) = (lambda [] (function _ ()) static (compound))))");
        same(body("auto r = [](int a) { return a; }(3);"), "(declaration (variable r (auto) = (call (lambda [] ((parameter a (builtin int))) (function _ ((parameter a (builtin int)))) (compound (return (id a)))) (integer 3))))");
        same(body("f([&](int i) { g(i); });"), "(expression (call (id f) (lambda [(capture &)] ((parameter i (builtin int))) (function _ ((parameter i (builtin int)))) (compound (expression (call (id g) (id i)))))))");
        same(body("auto l = [i = 0]() mutable { return ++i; };"), "(declaration (variable l (auto) = (lambda [(capture i = (integer 0))] () (function _ ()) mutable (compound (return (unary ++ (id i)))))))");
    };

    "requires-expressions: simple, type, compound, nested requirements"_test = [] {
        same(body("bool b = requires { a + b; };"), "(declaration (variable b (builtin bool) = (requires (simple (binary + (id a) (id b))))))");
        same(body("bool b = requires(T t, U u) { t.f(u); typename T::type; { t + u } noexcept -> std::same_as<int>; requires sizeof(T) > 1; };"), "(declaration (variable b (builtin bool) = (requires ((parameter t (named T)) (parameter u (named U))) (simple (call (member . (id t) f) (id u))) (type (named T::type)) (compound (binary + (id t) (id u)) noexcept -> std::same_as<(builtin int)>) (nested (binary > (sizeof (paren (id T))) (integer 1))))))");
        same(body("bool b = requires { { f() } -> std::convertible_to<bool>; };"), "(declaration (variable b (builtin bool) = (requires (compound (call (id f)) -> std::convertible_to<(builtin bool)>))))");
    };

    "types as a body writes them: placeholders, decltype, function types, packs"_test = [] {
        same(body("std::function<void(int, char)> f;"), "(declaration (variable f (named std::function<(function (builtin void) ((parameter (builtin int)) (parameter (builtin char))))>)))");
        same(body("std::function<int(const std::string&, Ts&&...)> g;"), "(declaration (variable g (named std::function<(function (builtin int) ((parameter (lvalue-ref (named std::string const))) (parameter pack (rvalue-ref (named Ts)))))>)))");
        same(body("using F = int (*)(double); using G = void (&)(int); using A = int[3][4];"), "(declaration (type-alias F (pointer (function (builtin int) ((parameter (builtin double))))))) (declaration (type-alias G (lvalue-ref (function (builtin void) ((parameter (builtin int))))))) (declaration (type-alias A (array (array (builtin int) (integer 4)) (integer 3))))");
        same(body("decltype(a + b) c = a + b; decltype(auto) d = f();"), "(declaration (variable c (decltype (binary + (id a) (id b))) = (binary + (id a) (id b)))) (declaration (variable d (decltype-auto) = (call (id f))))");
        same(body("auto f = [](auto&&... args) -> decltype(auto) { return g(std::forward<decltype(args)>(args)...); };"), "(declaration (variable f (auto) = (lambda [] ((parameter args pack (rvalue-ref (auto)))) (function (decltype-auto) ((parameter args pack (rvalue-ref (auto))))) (compound (return (call (id g) (pack-expansion (call (id std::forward<(decltype (id args))>) (id args)))))))))");
        same(body("std::pair<int, std::vector<std::pair<int, int>>> p;"), "(declaration (variable p (named std::pair<(builtin int), (named std::vector<(named std::pair<(builtin int), (builtin int)>)>)>)))");
        same(body("using X = typename std::remove_reference<T>::type;"), "(declaration (type-alias X (elaborated typename std::remove_reference<(named T)>::type)))");
        same(body("const volatile unsigned long int* p;"), "(declaration (variable p (pointer (builtin unsigned long const volatile))))");
        same(body("int Foo::*mp; int (Foo::*mf)(int);"), "(declaration (variable mp (member-pointer Foo (builtin int)))) (declaration (variable mf (member-pointer Foo (function (builtin int) ((parameter (builtin int)))))))");
    };

    "local classes: methods read with their own scope, constructors, operators, conversions, nested types"_test = [] {
        same(body("struct S { S(int x) : v(x) {} int get() const { return v; } bool operator<(const S& o) const { return v < o.v; } explicit operator int() { return v; } int v; };"), "(declaration (class S (function S (function _ ((parameter x (builtin int)))) (function-body (member-init v (id x)) (compound))) (function get (function (builtin int) () const) (function-body (compound (return (id v))))) (function operator< (function (builtin bool) ((parameter o (lvalue-ref (named S const)))) const) (function-body (compound (return (binary < (id v) (member . (id o) v)))))) (function operator (builtin int) (function _ ()) (function-body (compound (return (id v))))) (field v (builtin int))))");
        same(body("struct R { struct In { int x; }; In i; enum { a, b }; static constexpr int n = 3; };"), "(declaration (class R (class In (field x (builtin int))) (field i (named In)) (enum (enumerator a) (enumerator b)) (variable n static constexpr (builtin int) = (integer 3))))");
        same(body("struct S { friend class T; friend void f(S&); using V = int; V v; static_assert(sizeof(V) == 4); };"), "(declaration (class S (friend (elaborated class T)) (function f friend (function (builtin void) ((parameter (lvalue-ref (named S)))))) (type-alias V (builtin int)) (field v (named V)) (static-assert (binary == (sizeof (named V)) (integer 4)))))");
    };

    "the outline's declaration a local names is found by its token (Local::outline)"_test = [] {
        const auto syntax = f::parse("void f() { struct L { int m; }; int local = 1; }");
        const auto tree = f::parse_bodies(syntax);
        std::size_t linked { 0 };
        for (const auto& l : tree.locals)
            if (l.outline >= 0) {
                ++linked;
                expect(syntax.declarations[static_cast<std::size_t>(l.outline)].name_token == l.name_token);
            }
        expect(linked >= 3);   // L, m, local
    };

    return report();
}
