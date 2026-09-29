// mcxx.frontend:preprocess: the standard's own examples ([cpp.scope], [cpp.stringize], [cpp.concat],
// [cpp.subst]), conditionals per target, module lines, and what makes an answer not certain.
import std;
import mcxx.testing;
import mcxx.msa;
import mcxx.frontend;

namespace f = mcxx::frontend;

namespace {

std::vector<std::string> spellings(const f::Preprocessed& pp) {
    std::vector<std::string> out;
    for (const auto& t : pp.tokens) out.emplace_back(t.spelling);
    return out;
}

std::vector<std::string> spellings(std::string_view expected) {
    std::vector<std::string> out;
    for (const auto& t : f::lex(expected)) out.push_back(f::spelling(expected, t));
    return out;
}

bool gives(std::string_view source, std::string_view expected, const f::PreprocessOptions& options = {}) {
    const auto pp = f::preprocess(source, options);
    const auto got = spellings(pp);
    const auto want = spellings(expected);
    if (got != want) {
        std::string g;
        for (const auto& s : got) g += s + " ";
        std::println(std::cerr, "  got:  {}", g);
        for (const auto& d : pp.diagnostics) std::println(std::cerr, "  {}:{}: {}", d.at.line, d.at.column, d.message);
    }
    return got == want;
}

} // namespace

int main() {
    using namespace mcxx::testing;

    "[cpp.scope] example 3: redefinition, rescanning, placemarkers, stringizing"_test = [] {
        expect(gives(R"(#define x 3
#define f(a) f(x * (a))
#undef x
#define x 2
#define g f
#define z z[0]
#define h g(~
#define m(a) a(w)
#define w 0,1
#define t(a) a
#define p() int
#define q(x) x
#define r(x,y) x ## y
#define str(x) # x
f(y+1) + f(f(z)) % t(t(g)(0) + t)(1);
g(x+(3,4)-w) | h 5) & m
(f)^m(m);
p() i[q()] = { q(1), r(2,3), r(4,), r(,5), r(,) };
char c[2][6] = { str(hello), str() };
)",
                     R"(f(2 * (y+1)) + f(2 * (f(2 * (z[0])))) % f(2 * (0)) + t(1);
f(2 * (2+(3,4)-0,1)) | f(2 * (~ 5)) & f(2 * (0,1))^m(0,1);
int i[] = { 1, 23, 4, 5, };
char c[2][6] = { "hello", "" };)"));
    };

    "[cpp.scope] example 4: # and ## together"_test = [] {
        expect(gives(R"(#define str(s) # s
#define xstr(s) str(s)
#define debug(s, t) printf("x" # s "= %d, x" # t "= %s", \
 x ## s, x ## t)
#define INCFILE(n) vers ## n
#define glue(a, b) a ## b
#define xglue(a, b) glue(a, b)
#define HIGHLOW "hello"
#define LOW LOW ", world"
debug(1, 2);
fputs(str(strncmp("abc\0d", "abc", '\4') // this goes away
 == 0) str(: @\n), s);
xstr(INCFILE(2).h)
glue(HIGH, LOW);
xglue(HIGH, LOW)
)",
                     R"(printf("x" "1" "= %d, x" "2" "= %s", x1, x2);
fputs("strncmp(\"abc\\0d\", \"abc\", '\\4') == 0" ": @\n", s);
"vers2.h"
"hello";
"hello" ", world")"));
    };

    "[cpp.scope] example 5: placemarkers"_test = [] {
        expect(gives("#define t(x,y,z) x ## y ## z\nint j[] = { t(1,2,3), t(,4,5), t(6,,7), t(8,9,),\n t(10,,), t(,11,), t(,,12), t(,,) };",
                     "int j[] = { 123, 45, 67, 89, 10, 11, 12, };"));
    };

    "[cpp.subst] __VA_OPT__"_test = [] {
        expect(gives(R"(#define F(...) f(0 __VA_OPT__(,) __VA_ARGS__)
#define G(X, ...) f(0, X __VA_OPT__(,) __VA_ARGS__)
#define SDEF(sname, ...) S sname __VA_OPT__(= { __VA_ARGS__ })
#define EMP
F(a,b,c) F() F(EMP) G(a,b,c) G(a,) G(a) SDEF(foo); SDEF(bar, 1, 2);
)",
                     "f(0, a, b, c) f(0) f(0) f(0, a, b, c) f(0, a) f(0, a) S foo; S bar = { 1, 2 };"));
        expect(gives(R"(#define H2(X, Y, ...) __VA_OPT__(X ## Y,) __VA_ARGS__
H2(a, b, c, d)
#define H3(X, ...) #__VA_OPT__(X##X X##X)
H3(, 0)
#define H4(X, ...) __VA_OPT__(a X ## X) ## b
H4(, 1)
#define H5A(...) __VA_OPT__()/**/__VA_OPT__()
#define H5B(X) a ## X ## b
#define H5C(X) H5B(X)
H5C(H5A())
)",
                     R"(ab, c, d "" a b ab)"));
    };

    "a macro is not expanded inside its own expansion; GNU , ## __VA_ARGS__ drops the comma"_test = [] {
        expect(gives("#define foo foo + 1\n#define bar(x) bar(x)\nfoo bar(bar(2))", "foo + 1 bar(bar(2))"));
        expect(gives("#define e(fmt, ...) p(fmt, ## __VA_ARGS__)\ne(\"a\") e(\"b\", 1)", R"(p("a") p("b", 1))"));
        expect(gives("#define L __LINE__\nint a = L;\n\nint b = __LINE__;", "int a = 2; int b = 4;"));
    };

    "conditionals follow the target's predefined macros"_test = [] {
        const std::string_view source { R"(#if defined(_WIN32)
windows
#elif defined(__APPLE__)
apple
#elif defined(__linux__) && __x86_64__
linux_x64
#else
other
#endif
#if __cplusplus >= 202302L && (1 ? 2 : 1/0) == 2 && -1 < 0u == 0
cxx23
#endif
)" };
        expect(gives(source, "linux_x64 cxx23"));
        expect(gives(source, "windows cxx23", { .target = "x86_64-pc-windows-msvc" }));
        expect(gives(source, "apple cxx23", { .target = "aarch64-apple-darwin" }));
        const auto pp = f::preprocess(source);
        // Branches not taken one after another are one stretch.
        expect(pp.certain && pp.skipped == std::vector<std::pair<std::uint32_t, std::uint32_t>> { { 2, 4 }, { 8, 8 } });
    };

    "the module declaration, the global module fragment and the imports"_test = [] {
        const auto pp = f::preprocess(R"(module;
#include <cstdio>
#define X 1
export module m.core:part [[deprecated]];
import std;
export import :other;
import <vector>;
auto module = import(1);
module :private;
)");
        expect(pp.module.present && pp.module.name == "m.core" && pp.module.partition == "part" && pp.module.exported);
        expect(pp.module.global_module_fragment && pp.module.private_fragment);
        expect(pp.includes.size() == 1 && pp.includes[0].global_module_fragment && pp.includes[0].header == "<cstdio>");
        expect(pp.imports.size() == 3 && pp.imports[0].name == "std" && pp.imports[1].name == ":other" && pp.imports[1].exported &&
               pp.imports[2].name == "<vector>");
        expect(pp.certain);
        const auto facts = f::facts(pp);
        expect(facts.macros.size() == 1 && facts.macros[0].name == "X" && facts.macros[0].range.begin.line == 2 &&
               facts.macros[0].range.begin.column == 8);
        expect(facts.includes.size() == 1 && facts.includes[0].range.end.column == 17);
    };

    "not certain: a header's macro in a condition, __has_include, #line; certain: another target's macro"_test = [] {
        expect(!f::preprocess("#include <errno.h>\n#if defined(EBADARCH)\n#endif").certain);
        expect(f::preprocess("#if defined(EBADARCH)\n#endif").certain);   // nothing included: not a macro
        expect(f::preprocess("#include <errno.h>\n#if defined(_WIN32) || defined(__APPLE__)\n#endif").certain);
        expect(f::preprocess("#include <errno.h>\n#undef FOO\n#ifdef FOO\n#endif").certain);
        expect(!f::preprocess("#if __has_include(<x.h>)\n#endif").certain);
        expect(f::preprocess("#if __has_cpp_attribute(nodiscard) == 201907L\nyes\n#endif").tokens.size() == 1);
        expect(!f::preprocess("#line 10\n").certain);
        const auto purview = f::preprocess("export module m;\n#include <cstdio>\n");
        expect(!purview.certain && !purview.includes[0].global_module_fragment);
        expect(f::preprocess("#include <x.h>\n#if FOO\n#endif", { .header_macros = { "#define FOO 1" }, .header_macros_complete = true }).certain);
    };

    "#error in a group taken is an error; one not taken is nothing"_test = [] {
        const auto a = f::preprocess("#if 0\n#error no\n#else\n#error yes it is\n#endif\n");
        expect(a.diagnostics.size() == 1 && a.diagnostics[0].message == "#error yes it is");
        expect(f::preprocess("#if 1\n#else\n#bogus\n#endif\n").diagnostics.empty());
        expect(f::preprocess("#bogus\n").diagnostics.size() == 1);
    };

    return report();
}
