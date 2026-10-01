// mcxx.frontend:bodies, C++26's syntax as the language extension reads it (A3.0.2): reflection's `^^` and
// splices, expansion statements, contracts, pack indexing, `= delete("why")`, structured binding packs,
// consteval blocks -- each with its MC1 feature id, and none of it read by the C++23 core alone.
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

// The distinct MC1 feature ids of every node a text's bodies make.
std::set<std::string> features(std::string_view code) {
    const auto fragment = f::parse_fragment(code);
    const auto& tree = fragment.tree;
    std::set<std::string> out;
    for (const auto& e : tree.expressions)
        if (const auto id = ast::feature_of(e); !id.empty()) out.emplace(id);
    for (const auto& s : tree.statements)
        if (const auto id = ast::feature_of(s); !id.empty()) out.emplace(id);
    for (const auto& t : tree.types)
        if (const auto id = ast::feature_of(t); !id.empty()) out.emplace(id);
    for (const auto& l : tree.locals)
        if (const auto id = ast::feature_of(l); !id.empty()) out.emplace(id);
    return out;
}

const f::BodyOptions CORE_ONLY { .extensions = {} };

} // namespace

int main() {
    using namespace mcxx::testing;

    "reflection: the reflect operator on a type, a template, a namespace, a value, the global namespace"_test = [] {
        same(body("auto r = ^^int;"), "(declaration (variable r (auto) = (reflect (builtin int))))");
        same(body("auto r = ^^std::vector;"), "(declaration (variable r (auto) = (reflect (id std::vector))))");
        same(body("auto r = ^^::;"), "(declaration (variable r (auto) = (reflect)))");
        same(body("auto r = ^^ns::member;"), "(declaration (variable r (auto) = (reflect (id ns::member))))");
        same(body("constexpr auto m = ^^T::value;"), "(declaration (variable m constexpr (auto) = (reflect (id T::value))))");
        same(body("auto r = f(^^S, ^^Ts);"), "(declaration (variable r (auto) = (call (id f) (reflect (named S)) (reflect (named Ts)))))");
    };

    "splices: an expression, a type, a nested-name prefix, a member access, a template argument"_test = [] {
        same(body("int y = [: r :];"), "(declaration (variable y (builtin int) = (splice (id r))))");
        same(body("typename [: r :] x;"), "(declaration (variable x (splice (id r))))");
        same(body("[: ^^int :] z;"), "(declaration (variable z (splice (reflect (builtin int)))))");
        same(body("auto v = o.[: m :];"), "(declaration (variable v (auto) = (member . (id o) [:(id m):])))");
        same(body("auto n = [: r :]::member;"), "(declaration (variable n (auto) = (id [:(id r):]::member)))");
        same(body("f<[: r :]>();"), "(expression (call (id f<(splice (id r))>)))");
        same(body("auto a = [: ^^int :](3);"), "(declaration (variable a (auto) = (call (splice (reflect (builtin int))) (integer 3))))");
    };

    "expansion statements: template for with a range, with an init-statement"_test = [] {
        same(body("template for (auto x : xs) f(x);"), "(template-for (variable x (auto)) : (id xs) (expression (call (id f) (id x))))");
        same(body("template for (constexpr auto m : members) { g(m); }"), "(template-for (variable m constexpr (auto)) : (id members) (compound (expression (call (id g) (id m)))))");
        same(body("template for (auto i = 0; auto x : v) h(x);"), "(template-for init (declaration (variable i (auto) = (integer 0))) (variable x (auto)) : (id v) (expression (call (id h) (id x))))");
    };

    "contracts: pre and post on a function, contract_assert, a result name"_test = [] {
        same(parts("int f(int x) pre(x > 0) post(r: r > x) { return x; }"), "(binary > (id x) (integer 0)) | (binary > (id r) (id x)) | (function-body (compound (return (id x))))");
        same(parts("auto g(int a) -> int pre(a != 0) { return a; }"), "(binary != (id a) (integer 0)) | (function-body (compound (return (id a))))");
        same(body("contract_assert(x > 0);"), "(contract-assert (binary > (id x) (integer 0)))");
        same(parts("void h(int* p) pre(p != nullptr) pre(*p > 0);"), "(binary != (id p) (null-pointer)) | (binary > (unary * (id p)) (integer 0))");
    };

    "pack indexing: an expression and a type"_test = [] {
        same(body("auto first = args...[0];"), "(declaration (variable first (auto) = (pack-index (id args) (integer 0))))");
        same(body("f(args...[N - 1], g(xs...[0]));"), "(expression (call (id f) (pack-index (id args) (binary - (id N) (integer 1))) (call (id g) (pack-index (id xs) (integer 0)))))");
        same(body("Ts...[1] x;"), "(declaration (variable x (pack-index (named Ts) (integer 1))))");
        same(body("using last = Ts...[sizeof...(Ts) - 1];"), "(declaration (type-alias last (pack-index (named Ts) (binary - (sizeof-pack Ts) (integer 1)))))");
    };

    "= delete with a reason"_test = [] {
        same(parts("void f() = delete(\"use g\"); struct S { S(int) = delete(\"no\"); };"), "(function-body delete reason (string \"use g\")) | (function-body delete reason (string \"no\"))");
    };

    "structured binding packs"_test = [] {
        same(body("auto [...xs] = t;"), "(declaration (structured-binding pack (auto) (binding xs pack) = (id t)))");
        same(body("auto [a, ...rest] = t;"), "(declaration (structured-binding pack (auto) (binding a) (binding rest pack) = (id t)))");
        same(body("auto& [...ys] = u;"), "(declaration (structured-binding & pack (auto) (binding ys pack) = (id u)))");
    };

    "consteval blocks: a statement, a member"_test = [] {
        same(body("consteval { f(); g(); }"), "(consteval-block (compound (expression (call (id f))) (expression (call (id g)))))");
        same(body("struct S { consteval { h(); } int x; };"), "(declaration (class S (consteval-block (compound (expression (call (id h))))) (field x (builtin int))))");
    };

    "each construct names its MC1 feature, the catalog's own ids"_test = [] {
        expect(features("auto r = ^^int; int y = [: r :];") == std::set<std::string> { "c++26:reflection" });
        expect(features("template for (auto x : xs) f(x);") == std::set<std::string> { "c++26:expansion-statements" });
        expect(features("contract_assert(a);") == std::set<std::string> { "c++26:contracts" });
        expect(features("auto a = args...[0]; Ts...[1] b;") == std::set<std::string> { "c++26:pack-indexing" });
        expect(features("auto [...xs] = t;").contains("c++26:structured-bindings-can-introduce-pack"));
        expect(features("struct S { S(int) = delete(\"no\"); };").contains("c++26:delete-with-reason"));
        expect(features("consteval { f(); }") == std::set<std::string> { "c++26:consteval-blocks" });
    };

    "the core alone reads C++23: without the extension none of it is syntax"_test = [] {
        for (const std::string_view code : { "auto r = ^^int;", "int y = [: r :];", "template for (auto x : xs) f(x);", "auto a = args...[0];" }) {
            const auto with = f::parse_fragment(code);
            const auto without = f::parse_fragment(code, CORE_ONLY);
            expect(with.tree.stats.failed == 0) << code;
            expect(without.tree.stats.failed == 1) << code;
        }
        // C++23 itself is the same either way.
        same(body("auto l = [](int a) { return a; };", CORE_ONLY), body("auto l = [](int a) { return a; };"));
    };

    "the extension names the ids it provides, for a profile that denies them"_test = [] {
        const auto extension = f::cpp26_syntax();
        expect(extension->name() == "mcxx.plugins.lang.cpp26");
        std::set<std::string> ids;
        for (const auto id : extension->features()) ids.emplace(id);
        expect(ids.contains("c++26:reflection") && ids.contains("c++26:contracts") && ids.contains("c++26:pack-indexing"));
        // Every id the tree can name is either in the catalog or proposed for it (and says which).
        std::size_t registered { 0 };
        for (const auto& feature : ast::syntax_features()) registered += feature.registered;
        expect(registered >= 19);
    };

    return report();
}
