// mcxx.frontend:bodies, expressions: precedence and associativity, casts, calls, members, literals, new and
// delete, braced initializers, fold-expressions, and what the parse says it cannot read.
import std;
import mcxx.testing;
import mcxx.msa;
import mcxx.frontend;

namespace f = mcxx::frontend;
namespace ast = mcxx::frontend::ast;

namespace {

// The dump of the expression an expression-statement holds; "<n errors>" when it did not read.
std::string expr(std::string_view code, f::BodyOptions options = {}) {
    const auto fragment = f::parse_fragment(std::string { code } + ";", options);
    const auto& tree = fragment.tree;
    if (tree.roots.empty()) return "<no root>";
    const auto* body = tree.statement(tree.roots[0].node);
    if (body == nullptr || body->kind != ast::StmtKind::function_body || !body->a) return "<no body>";
    const auto& compound = tree[body->a];
    if (compound.list.count == 0) return "<empty>";
    const auto* statement = tree.statement(tree.list(compound.list)[0]);
    if (statement == nullptr || statement->kind != ast::StmtKind::expression) return "<" + ast::dump(tree, tree.list(compound.list)[0]) + ">";
    return ast::dump(tree, statement->e1.handle());
}

std::size_t failures(std::string_view code) { return f::parse_fragment(code).tree.stats.failed; }

void same(const std::string& actual, std::string_view expected, std::source_location where = std::source_location::current()) {
    mcxx::testing::expect(actual == expected, where) << "\n    got:      " << actual << "\n    expected: " << expected;
}

} // namespace

int main() {
    using namespace mcxx::testing;

    "precedence: multiplication binds tighter than addition, comparison looser, assignment loosest and to the right"_test = [] {
        same(expr("a + b * c"), "(binary + (id a) (binary * (id b) (id c)))");
        same(expr("a * b + c"), "(binary + (binary * (id a) (id b)) (id c))");
        same(expr("a - b - c"), "(binary - (binary - (id a) (id b)) (id c))");
        same(expr("r = a < b == c > d"), "(assign = (id r) (binary == (binary < (id a) (id b)) (binary > (id c) (id d))))");
        same(expr("a = b = c"), "(assign = (id a) (assign = (id b) (id c)))");
        same(expr("a += b ? c : d"), "(assign += (id a) (conditional (id b) (id c) (id d)))");
        same(expr("a || b && c | d ^ e & f"), "(binary || (id a) (binary && (id b) (binary | (id c) (binary ^ (id d) (binary & (id e) (id f))))))");
        same(expr("a << 1 + 2"), "(binary << (id a) (binary + (integer 1) (integer 2)))");
        same(expr("a <=> b"), "(binary <=> (id a) (id b))");
        same(expr("a, b, c"), "(binary , (binary , (id a) (id b)) (id c))");
        same(expr("a ? b : c ? d : e"), "(conditional (id a) (id b) (conditional (id c) (id d) (id e)))");
        same(expr("a ?: c"), "(conditional (id a) _ (id c))");
    };

    "unary and postfix operators"_test = [] {
        same(expr("-a * !b"), "(binary * (unary - (id a)) (unary ! (id b)))");
        same(expr("*p++"), "(unary * (unary post++ (id p)))");
        same(expr("&x[1].y->z"), "(unary & (member -> (member . (subscript (id x) (integer 1)) y) z))");
        same(expr("++i + j--"), "(binary + (unary ++ (id i)) (unary post-- (id j)))");
        same(expr("~a & b"), "(binary & (unary ~ (id a)) (id b))");
        same(expr("a.*pm + b->*pm"), "(binary + (binary .* (id a) (id pm)) (binary ->* (id b) (id pm)))");
        same(expr("f(a, b)(c)"), "(call (call (id f) (id a) (id b)) (id c))");
        same(expr("a[1, 2]"), "(subscript (id a) (integer 1) (integer 2))");
        same(expr("a[]"), "(subscript (id a))");
        same(expr("not a and b or c"), "(binary || (binary && (unary ! (id a)) (id b)) (id c))");
    };

    "literals: integers, floating, characters, strings joined, booleans, nullptr, user-defined"_test = [] {
        same(expr("42"), "(integer 42)");
        same(expr("0x1F + 1'000"), "(binary + (integer 0x1F) (integer 1'000))");
        same(expr("1.5f"), "(floating 1.5f)");
        same(expr("1e3"), "(floating 1e3)");
        same(expr("0x1p3"), "(floating 0x1p3)");
        same(expr("'a'"), "(character 'a')");
        same(expr("\"ab\" \"cd\""), "(string \"ab\" \"cd\")");
        same(expr("true && nullptr"), "(binary && (boolean true) (null-pointer))");
        const auto udl = f::parse_fragment("auto d = 10_km; auto s = \"x\"_s; auto t = 5ms;");
        std::size_t user_defined { 0 };
        for (const auto& e : udl.tree.expressions) user_defined += e.has(ast::Expr::user_defined);
        expect(user_defined == 3);
        same(expr("this->x"), "(member -> (this) x)");
    };

    "names: qualified, with template arguments, operators, destructors"_test = [] {
        same(expr("f(std::vector<int>::size_type)"), "(call (id f) (id std::vector<(builtin int)>::size_type))");
        same(expr("f(::a::b)"), "(call (id f) (id ::a::b))");
        same(expr("a.operator()(1)"), "(call (member . (id a) operator()) (integer 1))");
        same(expr("p->~T()"), "(call (member -> (id p) ~T))");
        same(expr("x.template f<int>(1)"), "(call (member . (id x) template f<(builtin int)>) (integer 1))");
        same(expr("operator+(a, b)"), "(call (id operator+) (id a) (id b))");
    };

    "a `<` after a name: arguments when the shape says so, a comparison when it does not"_test = [] {
        same(expr("f<int>(x)"), "(call (id f<(builtin int)>) (id x))");
        same(expr("a < b"), "(binary < (id a) (id b))");
        same(expr("a < b && c > d"), "(binary && (binary < (id a) (id b)) (binary > (id c) (id d)))");
        same(expr("r = a < b > c"), "(assign = (id r) (binary > (binary < (id a) (id b)) (id c)))");
        same(expr("x = std::is_same_v<A, B>"), "(assign = (id x) (id std::is_same_v<(named A), (named B)>))");
        same(expr("g<A<B>>(1)"), "(call (id g<(named A<(named B)>)>) (integer 1))");
        same(expr("make<int, 3>()"), "(call (id make<(builtin int), (integer 3)>))");
        expect(expr("f(a < b, c > (d))").starts_with("(call (id f)"));
    };

    "casts: named, C-style when the type is certain, functional"_test = [] {
        same(expr("static_cast<int>(x)"), "(cast static_cast (builtin int) (id x))");
        same(expr("reinterpret_cast<char*>(p)"), "(cast reinterpret_cast (pointer (builtin char)) (id p))");
        same(expr("dynamic_cast<const Base&>(r)"), "(cast dynamic_cast (lvalue-ref (named Base const)) (id r))");
        same(expr("(int)x + 1"), "(binary + (cast-c (builtin int) (id x)) (integer 1))");
        same(expr("(unsigned long)-1"), "(cast-c (builtin unsigned long) (unary - (integer 1)))");
        same(expr("(char*)&x"), "(cast-c (pointer (builtin char)) (unary & (id x)))");
        same(expr("(a) + b"), "(binary + (paren (id a)) (id b))");
        same(expr("(a)(b)"), "(call (paren (id a)) (id b))");
        same(expr("r = int(x)"), "(assign = (id r) (cast-functional (builtin int) (id x)))");
        same(expr("unsigned{3}"), "(cast-functional-brace (builtin unsigned int) (integer 3))");
        same(expr("std::string(\"s\")"), "(call (id std::string) (string \"s\"))");
        same(expr("r = auto(x)"), "(assign = (id r) (cast-functional (auto) (id x)))");
        same(expr("r = decltype(x)(y)"), "(assign = (id r) (cast-functional (decltype (id x)) (id y)))");
        same(expr("typename T::type(1)"), "(cast-functional (elaborated typename T::type) (integer 1))");
    };

    "sizeof, alignof, noexcept, typeid and the like take a type or an expression"_test = [] {
        same(expr("sizeof(int)"), "(sizeof (builtin int))");
        same(expr("sizeof x"), "(sizeof (id x))");
        same(expr("sizeof(x) + 1"), "(binary + (sizeof (paren (id x))) (integer 1))");
        same(expr("sizeof(int*)"), "(sizeof (pointer (builtin int)))");
        same(expr("sizeof...(Ts)"), "(sizeof-pack Ts)");
        same(expr("alignof(double)"), "(alignof (builtin double))");
        same(expr("noexcept(f())"), "(noexcept (call (id f)))");
        same(expr("typeid(int)"), "(typeid (builtin int))");
        same(expr("typeid(*p)"), "(typeid (unary * (id p)))");
    };

    "new and delete: placement, arrays, initializers"_test = [] {
        same(expr("new int"), "(new (builtin int))");
        same(expr("new int(3)"), "(new (builtin int) ((paren-list (integer 3))))");
        same(expr("new int[n]"), "(new array (builtin int) [(id n)])");
        same(expr("new T{1, 2}"), "(new (named T) {(init-list (integer 1) (integer 2))})");
        same(expr("new (buf) T(1)"), "(new (named T) @(id buf) ((paren-list (integer 1))))");
        same(expr("::new int"), "(new global (builtin int))");
        same(expr("new (int)"), "(new (builtin int))");
        same(expr("delete p"), "(delete (id p))");
        same(expr("delete[] p"), "(delete array (id p))");
        same(expr("::delete p"), "(delete global (id p))");
    };

    "braced initializers: designated, nested, with packs"_test = [] {
        same(expr("T{1, 2}"), "(cast-functional-brace (named T) (integer 1) (integer 2))");
        same(expr("x = {1, {2, 3}}"), "(assign = (id x) (init-list (integer 1) (init-list (integer 2) (integer 3))))");
        same(expr("T{.a = 1, .b{2}}"), "(cast-functional-brace (named T) (designated .a = (integer 1)) (designated .b = (init-list (integer 2))))");
        same(expr("f(args...)"), "(call (id f) (pack-expansion (id args)))");
        same(expr("T{xs...}"), "(cast-functional-brace (named T) (pack-expansion (id xs)))");
    };

    "fold-expressions: unary left and right, binary"_test = [] {
        same(expr("(args + ...)"), "(fold + right (id args))");
        same(expr("(... + args)"), "(fold + left (id args))");
        same(expr("(0 + ... + args)"), "(fold + right (integer 0) (id args))");
        same(expr("(std::cout << ... << args)"), "(fold << right (id std::cout) (id args))");
        same(expr("(f(args), ...)"), "(fold , right (call (id f) (id args)))");
        same(expr("(a && ...)"), "(fold && right (id a))");
    };

    "throw, co_await, co_yield, conditional with throw, statement expressions"_test = [] {
        same(expr("throw 1"), "(throw (integer 1))");
        same(expr("throw"), "(throw _)");
        same(expr("a ? throw 1 : 2"), "(conditional (id a) (throw (integer 1)) (integer 2))");
        same(expr("co_await f()"), "(co-await (call (id f)))");
        same(expr("co_yield 1"), "(co-yield (integer 1))");
        expect(expr("({ int y = 1; y; })").starts_with("(statement-expression (compound (declaration"));
        same(expr("&&label"), "(label-address label)");
    };

    "builtins and type traits take types or expressions"_test = [] {
        same(expr("__builtin_offsetof(S, m)"), "(builtin __builtin_offsetof (named S) (id m))");
        same(expr("__is_same(A, B)"), "(builtin __is_same (named A) (named B))");
        same(expr("__builtin_va_arg(ap, int)"), "(builtin __builtin_va_arg (id ap) (builtin int))");
        same(expr("__builtin_expect(x, 1)"), "(builtin __builtin_expect (id x) (integer 1))");
    };

    "an expression that cannot be read is an error node and one diagnostic, the rest of the body read on"_test = [] {
        const auto fragment = f::parse_fragment("int x = 1; y = ( ; int z = 2;");
        expect(fragment.tree.stats.failed == 1);
        expect(fragment.tree.stats.errors >= 1);
        const auto dumped = ast::dump(fragment.tree, fragment.tree.roots[0].node);
        expect(dumped.find("(error)") != std::string::npos);
        expect(dumped.find("(variable z") != std::string::npos);
        expect(failures("int ok() { return 1 + 2 * 3; }") == 0);
    };

    "alternative tokens and splits of `>>` in template arguments"_test = [] {
        same(expr("r = a bitand b"), "(assign = (id r) (binary & (id a) (id b)))");
        same(expr("x = std::vector<std::vector<int>>{}"), "(assign = (id x) (cast-functional-brace (named std::vector<(named std::vector<(builtin int)>)>)))");
        same(expr("a >> b"), "(binary >> (id a) (id b))");
        same(expr("a >>= 1"), "(assign >>= (id a) (integer 1))");
    };

    return report();
}
