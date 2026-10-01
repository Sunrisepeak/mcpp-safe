// mcxx.frontend:bodies, statements and the declarations a block holds: the control statements, labels,
// declarations told from expressions, structured bindings, local types, and the recovery that keeps the tree.
import std;
import mcxx.testing;
import mcxx.msa;
import mcxx.frontend;

namespace f = mcxx::frontend;
namespace ast = mcxx::frontend::ast;

namespace {

// The statements of a function body, each as the dump gives it, one space between.
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

void same(const std::string& actual, std::string_view expected, std::source_location where = std::source_location::current()) {
    mcxx::testing::expect(actual == expected, where) << "\n    got:      " << actual << "\n    expected: " << expected;
}

} // namespace

int main() {
    using namespace mcxx::testing;

    "if, else, constexpr, consteval, an init-statement, a declaration as the condition"_test = [] {
        same(body("if (a) b(); else c();"), "(if (id a) (expression (call (id b))) else (expression (call (id c))))");
        same(body("if (a) { b(); } else if (c) { d(); } else { e(); }"), "(if (id a) (compound (expression (call (id b)))) else (if (id c) (compound (expression (call (id d)))) else (compound (expression (call (id e))))))");
        same(body("if constexpr (N > 1) f();"), "(if constexpr (binary > (id N) (integer 1)) (expression (call (id f))))");
        same(body("if consteval { f(); } else { g(); }"), "(if consteval (compound (expression (call (id f)))) else (compound (expression (call (id g)))))");
        same(body("if !consteval { g(); }"), "(if !consteval (compound (expression (call (id g)))))");
        same(body("if (int i = f(); i > 0) g(i);"), "(if init (declaration (variable i (builtin int) = (call (id f)))) (binary > (id i) (integer 0)) (expression (call (id g) (id i))))");
        same(body("if (auto p = f()) g(*p);"), "(if (variable p (auto) = (call (id f))) (expression (call (id g) (unary * (id p)))))");
        same(body("if (T x{1}) g(x);"), "(if (variable x (named T) {(init-list (integer 1))}) (expression (call (id g) (id x))))");
        same(body("if (auto [a, b] = f()) g(a);"), "(if (structured-binding (auto) (binding a) (binding b) = (call (id f))) (expression (call (id g) (id a))))");
        same(body("if ([[maybe_unused]] auto* p = f()) g();"), "(if (variable p (pointer (auto)) = (call (id f))) (expression (call (id g))))");
    };

    "switch with cases, a range, default, fallthrough, an init-statement"_test = [] {
        same(body("switch (x) { case 1: a(); break; case 2: case 3: b(); break; default: c(); }"), "(switch (id x) (compound (case (integer 1) (expression (call (id a)))) (break) (case (integer 2) (case (integer 3) (expression (call (id b))))) (break) (default (expression (call (id c))))))");
        same(body("switch (x) { case 1 ... 5: a(); }"), "(switch (id x) (compound (case (integer 1) ... (integer 5) (expression (call (id a))))))");
        same(body("switch (int i = f(); i) { case A::b: [[fallthrough]]; default: break; }"), "(switch init (declaration (variable i (builtin int) = (call (id f)))) (id i) (compound (case (id A::b) (attributed (null))) (default (break))))");
    };

    "loops: while, do, for, range-based for with an init and a structured binding"_test = [] {
        same(body("while (a < b) ++a;"), "(while (binary < (id a) (id b)) (expression (unary ++ (id a))))");
        same(body("while (auto x = next()) use(x);"), "(while (variable x (auto) = (call (id next))) (expression (call (id use) (id x))))");
        same(body("do { a(); } while (b);"), "(do (compound (expression (call (id a)))) (id b))");
        same(body("for (int i = 0; i < n; ++i) f(i);"), "(for (declaration (variable i (builtin int) = (integer 0))) (binary < (id i) (id n)) (unary ++ (id i)) (expression (call (id f) (id i))))");
        same(body("for (;;) break;"), "(for (null) _ _ (break))");
        same(body("for (int i = 0, j = 1; i < j; ++i, --j) { }"), "(for (declaration (variable i (builtin int) = (integer 0)) (variable j (builtin int) = (integer 1))) (binary < (id i) (id j)) (binary , (unary ++ (id i)) (unary -- (id j))) (compound))");
        same(body("for (auto& x : v) f(x);"), "(range-for (variable x (lvalue-ref (auto))) : (id v) (expression (call (id f) (id x))))");
        same(body("for (const auto& [k, w] : m) f(k, w);"), "(range-for (structured-binding & (auto const) (binding k) (binding w)) : (id m) (expression (call (id f) (id k) (id w))))");
        same(body("for (auto v = f(); auto& x : v) g(x);"), "(range-for init (declaration (variable v (auto) = (call (id f)))) (variable x (lvalue-ref (auto))) : (id v) (expression (call (id g) (id x))))");
        same(body("for (int x : {1, 2, 3}) f(x);"), "(range-for (variable x (builtin int)) : (init-list (integer 1) (integer 2) (integer 3)) (expression (call (id f) (id x))))");
        same(body("for (auto it = s.begin(), ie = s.end(); it != ie; ++it) { }"), "(for (declaration (variable it (auto) = (call (member . (id s) begin))) (variable ie (auto) = (call (member . (id s) end)))) (binary != (id it) (id ie)) (unary ++ (id it)) (compound))");
    };

    "jumps, labels, return, co_return, goto and computed goto"_test = [] {
        same(body("return;"), "(return _)");
        same(body("return a, b;"), "(return (binary , (id a) (id b)))");
        same(body("return {1, 2};"), "(return (init-list (integer 1) (integer 2)))");
        same(body("co_return x;"), "(co-return (id x))");
        same(body("again: f(); goto again;"), "(labeled again (expression (call (id f)))) (goto again)");
        same(body("goto *p;"), "(goto * (id p))");
        same(body("for (;;) { if (a) continue; break; }"), "(for (null) _ _ (compound (if (id a) (continue)) (break)))");
        same(body("end:"), "(labeled end _)");
    };

    "try blocks and handlers"_test = [] {
        same(body("try { f(); } catch (const E& e) { g(e); } catch (...) { h(); }"), "(try (compound (expression (call (id f)))) (handler (variable e (lvalue-ref (named E const))) (compound (expression (call (id g) (id e))))) (handler ... (compound (expression (call (id h))))))");
        same(body("try { } catch (int) { }"), "(try (compound) (handler (variable (builtin int)) (compound)))");
    };

    "asm and static_assert are statements"_test = [] {
        same(body("asm volatile(\"nop\");"), "(asm)");
        same(body("static_assert(sizeof(int) == 4, \"msg\");"), "(static-assert (binary == (sizeof (builtin int)) (integer 4)) (string \"msg\"))");
    };

    "a statement that could be a declaration or an expression is told by what is known"_test = [] {
        same(body("a * b;"), "(declaration (variable b (pointer (named a))))");
        same(body("int x = 1; x * y;"), "(declaration (variable x (builtin int) = (integer 1))) (expression (binary * (id x) (id y)))");
        same(body("f(x);"), "(expression (call (id f) (id x)))");
        same(body("T(x);"), "(expression (call (id T) (id x)))");
        same(body("struct T {}; T(x);"), "(declaration (class T)) (declaration (variable x (named T)))");
        same(body("std::lock_guard<std::mutex> lock(m);"), "(declaration (variable lock (named std::lock_guard<(named std::mutex)>) ((init-list (id m)))))");
        same(body("Foo x(1);"), "(declaration (variable x (named Foo) ((init-list (integer 1)))))");
        same(body("Foo x(a, b);"), "(declaration (variable x (named Foo) ((init-list (id a) (id b)))))");
        same(body("Foo x();"), "(declaration (function x (function (named Foo) ())))");
        same(body("Foo x(Bar b);"), "(declaration (function x (function (named Foo) ((parameter b (named Bar))))))");
        same(body("int x = 1; Foo y(x);"), "(declaration (variable x (builtin int) = (integer 1))) (declaration (variable y (named Foo) ((init-list (id x)))))");
        same(body("x = y;"), "(expression (assign = (id x) (id y)))");
        same(body("std::cout << 1;"), "(expression (binary << (id std::cout) (integer 1)))");
        same(body("using T = int; T x;"), "(declaration (type-alias T (builtin int))) (declaration (variable x (named T)))");
        same(body("a < b > c;"), "(declaration (variable c (named a<(named b)>)))");
    };

    "declarations: specifiers, several declarators, initializer forms, auto, arrays, function pointers"_test = [] {
        same(body("static const int a = 1, *b = nullptr, c[3] = {1, 2, 3};"), "(declaration (variable a static (builtin int const) = (integer 1)) (variable b static (pointer (builtin int const)) = (null-pointer)) (variable c static (array (builtin int const) (integer 3)) = (init-list (integer 1) (integer 2) (integer 3))))");
        same(body("auto x = 3; auto& y = x; auto&& z = f(); decltype(auto) w = g();"), "(declaration (variable x (auto) = (integer 3))) (declaration (variable y (lvalue-ref (auto)) = (id x))) (declaration (variable z (rvalue-ref (auto)) = (call (id f)))) (declaration (variable w (decltype-auto) = (call (id g))))");
        same(body("int (*fp)(int, char) = nullptr;"), "(declaration (variable fp (pointer (function (builtin int) ((parameter (builtin int)) (parameter (builtin char))))) = (null-pointer)))");
        same(body("int (&ra)[3] = a;"), "(declaration (variable ra (lvalue-ref (array (builtin int) (integer 3))) = (id a)))");
        same(body("void (*signal(int, void (*)(int)))(int);"), "(declaration (function signal (function (pointer (function (builtin void) ((parameter (builtin int))))) ((parameter (builtin int)) (parameter (pointer (function (builtin void) ((parameter (builtin int))))))))))");
        same(body("int T::*pm = &T::m; void (T::*pmf)(int) const = nullptr;"), "(declaration (variable pm (member-pointer T (builtin int)) = (unary & (id T::m)))) (declaration (variable pmf (member-pointer T (function (builtin void) ((parameter (builtin int))) const)) = (null-pointer)))");
        same(body("thread_local int t; extern int e; register int r; constexpr int c = 1;"), "(declaration (variable t (builtin int))) (declaration (variable e (builtin int))) (declaration (variable r (builtin int))) (declaration (variable c constexpr (builtin int) = (integer 1)))");
        same(body("int i(1), j{2}, k = 3;"), "(declaration (variable i (builtin int) ((init-list (integer 1)))) (variable j (builtin int) {(init-list (integer 2))}) (variable k (builtin int) = (integer 3)))");
        same(body("const char* const names[] = {\"a\", \"b\"};"), "(declaration (variable names (array (pointer (builtin char const) const) _) = (init-list (string \"a\") (string \"b\"))))");
        same(body("unsigned long long n = 1ull; long double d = 1.0L; signed char sc; unsigned u;"), "(declaration (variable n (builtin unsigned long long) = (integer 1ull))) (declaration (variable d (builtin long double) = (floating 1.0L))) (declaration (variable sc (builtin signed char))) (declaration (variable u (builtin unsigned int)))");
        same(body("std::vector v{1, 2, 3}; std::array<int, 3> a{};"), "(declaration (variable v (named std::vector) {(init-list (integer 1) (integer 2) (integer 3))})) (declaration (variable a (named std::array<(builtin int), (integer 3)>) {(init-list)}))");
        same(body("typename T::value_type x;"), "(declaration (variable x (elaborated typename T::value_type)))");
        same(body("[[maybe_unused]] int unused = 0;"), "(attributed (declaration (variable unused (builtin int) = (integer 0))))");
    };

    "structured bindings: by value, by reference, with attributes"_test = [] {
        same(body("auto [a, b] = f();"), "(declaration (structured-binding (auto) (binding a) (binding b) = (call (id f))))");
        same(body("const auto& [k, v] = *it;"), "(declaration (structured-binding & (auto const) (binding k) (binding v) = (unary * (id it))))");
        same(body("auto&& [x, y, z] = t;"), "(declaration (structured-binding && (auto) (binding x) (binding y) (binding z) = (id t)))");
    };

    "local types: aliases, classes with members, enums, unions, using"_test = [] {
        same(body("typedef int I; using J = long; typedef int (*Fn)(int);"), "(declaration (type-alias I typedef (builtin int))) (declaration (type-alias J (builtin long))) (declaration (type-alias Fn typedef (pointer (function (builtin int) ((parameter (builtin int)))))))");
        same(body("struct S { int a; void f() { a = 1; } S() : a(0) {} ~S() {} };"), "(declaration (class S (field a (builtin int)) (function f (function (builtin void) ()) (function-body (compound (expression (assign = (id a) (integer 1)))))) (function S (function _ ()) (function-body (member-init a (integer 0)) (compound))) (function ~S (function _ ()) (function-body (compound)))))");
        same(body("enum E { x, y = 2, z }; enum class F : unsigned char { a, b };"), "(declaration (enum E (enumerator x) (enumerator y = (integer 2)) (enumerator z))) (declaration (enum F (builtin unsigned char) (enumerator a) (enumerator b)))");
        same(body("union U { int i; float f; };"), "(declaration (class U (field i (builtin int)) (field f (builtin float))))");
        same(body("struct D : public B, virtual C { int m = 1; static int s; operator bool() const { return true; } };"), "(declaration (class D :(named B) :(named C) (field m (builtin int) = (integer 1)) (variable s static (builtin int)) (function operator (builtin bool) (function _ () const) (function-body (compound (return (boolean true)))))))");
        same(body("using namespace std; using std::swap; namespace fs = std::filesystem;"), "(declaration (using-directive std)) (declaration (using-declaration std::swap)) (declaration (namespace-alias fs = (id std::filesystem)))");
        same(body("class K { public: int a; private: int b; protected: int c; };"), "(declaration (class K (access public) (field a (builtin int)) (access private) (field b (builtin int)) (access protected) (field c (builtin int))))");
        same(body("struct { int a; } anon;"), "(declaration (class (field a (builtin int))) (variable anon (elaborated struct _ (class (field a (builtin int))))))");
    };

    "a block is a scope: a name declared in it is not visible after it"_test = [] {
        same(body("{ int a; a * b; }"), "(compound (declaration (variable a (builtin int))) (expression (binary * (id a) (id b))))");
        same(body("{ int a; } a * b;"), "(compound (declaration (variable a (builtin int)))) (declaration (variable b (pointer (named a))))");
    };

    "recovery: a statement that cannot be read is one error node, the rest of the block is read"_test = [] {
        const auto fragment = f::parse_fragment("int a = 1; b = = 2; int c = 3; if ( ) { } int d = 4;");
        expect(fragment.tree.stats.failed == 1);
        const auto dumped = ast::dump(fragment.tree, fragment.tree.roots[0].node);
        expect(dumped.find("(variable a") != std::string::npos);
        expect(dumped.find("(variable c") != std::string::npos);
        expect(dumped.find("(variable d") != std::string::npos);
        expect(dumped.find("(error)") != std::string::npos);
        expect(fragment.tree.diagnostics.size() >= 2);
        // Whatever it makes, the tree is sound.
        expect(ast::validate(fragment.tree).empty());
    };

    "text that is not code still gives a tree and never loops"_test = [] {
        for (const std::string_view junk : { "}} {{ ((", "case: default: ;;;", "int ( ( ( ;", "x = [ ] ( { ;", "a ? b :", "for (", "if", "template <", "struct {", "#" }) {
            const auto fragment = f::parse_fragment(junk);
            expect(!fragment.tree.roots.empty()) << junk;
            expect(ast::validate(fragment.tree).empty()) << junk;
        }
    };

    return report();
}
