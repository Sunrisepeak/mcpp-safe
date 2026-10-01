// P2843R3, preprocessing is never undefined (ML-F): what the paper makes ill-formed with a diagnostic is an
// error when the feature (MC1 c++26:preprocessing-never-undefined) is on in the file, and nothing it was not
// before when it is not; what it defines (a new-line in a stringized raw string literal, a form feed in a //
// comment) is defined always, as Clang has it.
import std;
import mcxx.testing;
import mcxx.frontend;

namespace f = mcxx::frontend;

namespace {

constexpr std::string_view ID { "c++26:preprocessing-never-undefined" };

f::PreprocessOptions options(int year = 26, std::optional<f::FeatureLevel> level = f::FeatureLevel::allow) {
    f::PreprocessOptions o;
    o.language.standard.year = year;
    if (level) o.language.features[std::string { ID }] = *level;
    return o;
}

// The feature's diagnostics of a source, as "L: message" (the line of the directive, the id and the paper checked).
std::vector<std::string> forbidden(std::string_view source, const f::PreprocessOptions& o) {
    std::vector<std::string> out;
    for (const auto& d : f::preprocess(source, o).diagnostics)
        if (d.feature == ID) {
            out.push_back(std::format("{}: {}", d.at.line, d.message));
            if (d.paper != "P2843R3") out.push_back("wrong paper " + d.paper);
        }
    return out;
}

bool forbids(std::string_view source, const f::PreprocessOptions& o, std::string_view fragment, std::uint32_t line = 1) {
    return std::ranges::any_of(forbidden(source, o), [&](const std::string& s) { return s.starts_with(std::format("{}: ", line)) && s.find(fragment) != std::string::npos; });
}

std::string text(std::string_view source, const f::PreprocessOptions& o) {
    std::string out;
    for (const auto& t : f::preprocess(source, o).tokens) out += (out.empty() ? "" : " ") + std::string { t.spelling };
    return out;
}

} // namespace

int main() {
    using namespace mcxx::testing;

    "[cpp.predefined]/4: #define and #undef of a predefined macro, or of defined, is ill-formed"_test = [] {
        const auto o = options();
        expect(forbids("#define __cplusplus 12345678L\n", o, "`#define __cplusplus` names a predefined macro"));
        expect(forbids("#undef __cplusplus\n", o, "`#undef __cplusplus`"));
        expect(forbids("#undef __LINE__\n", o, "`#undef __LINE__`"));
        expect(forbids("#define __FILE__ \"x\"\n", o, "`#define __FILE__`"));
        expect(forbids("#undef defined\n", o, "`#undef defined`"));
        expect(forbids("#define __STDC_EMBED_FOUND__ 3\n", o, "__STDC_EMBED_FOUND__"));
        expect(forbids("#define __cpp_lib_xyz 1\n#define __cpp_pp_embed 1\n", o, "`#define __cpp_pp_embed`", 2));
        expect(forbids("#define __has_include 1\n", o, "__has_include"));
        // `defined` as a macro name is an error with or without the feature (Clang, GCC and EDG say so).
        const auto pp = f::preprocess("#define defined 1\n", options(23, std::nullopt));
        expect(std::ranges::any_of(pp.diagnostics, [](const auto& d) { return d.severity == f::Diagnostic::Severity::error && d.message.find("`defined`") != std::string::npos; }));
    };

    "[macro.names]: a keyword, an identifier with special meaning or an attribute-token is not a macro name"_test = [] {
        const auto o = options();
        expect(forbids("#define public private\n", o, "`#define public` names a keyword"));
        expect(forbids("#undef public\n", o, "`#undef public`"));
        expect(forbids("#undef private\n", o, "`#undef private`"));
        expect(forbids("#define and\n", o, "`#define and` names a keyword")) << "an alternative token";
        expect(forbids("#define final\n", o, "an identifier with special meaning"));
        expect(forbids("#undef override\n", o, "an identifier with special meaning"));
        expect(forbids("#define noreturn\n", o, "an attribute-token"));
        expect(forbids("#undef nodiscard\n", o, "an attribute-token"));
        expect(forbids("#define contract_assert(x) x\n", o, "a keyword")) << "C++26's";
        // [macro.names]: `likely` and `unlikely` may be function-like macros, not object-like ones.
        expect(forbidden("#define likely(x) x\n#define unlikely() 0\n", o).empty());
        expect(forbids("#define likely 1\n", o, "an attribute-token"));
        expect(forbids("#define unlikely  0\n", o, "an attribute-token"));
        expect(forbidden("#define EXPORT 1\n#define _Foo 2\n#undef EXPORT\n#define __cppx 1\n", o).empty()) << "any other name";
    };

    "without the feature nothing is new: the same text, no diagnostic about it"_test = [] {
        for (const auto& o : { options(23, std::nullopt), options(26, f::FeatureLevel::deny), options(23, f::FeatureLevel::allow) }) {
            expect(forbidden("#define __cplusplus 5\n#define public private\n#undef override\n#line 0\n", o).empty());
            expect(forbidden("#define BACKSLASH \\\\\n#define S(a) #a\n#define T(a) S(a)\nconst char* x = T(BACKSLASH);\n", o).empty());
            expect(forbidden("#define F(x) x\nF(\n#if 1\n1\n#endif\n)\n", o).empty());
        }
        // It follows the level and the standard: warn is a warning, deny and C++23 are nothing.
        const auto pp = f::preprocess("#define public private\n", options(26, f::FeatureLevel::warn));
        expect(pp.diagnostics.size() == 1 && pp.diagnostics[0].severity == f::Diagnostic::Severity::warning && pp.diagnostics[0].feature == ID);
    };

    "[cpp.line]: a line number from 1 to 2147483647, and one of the two forms after macro replacement"_test = [] {
        const auto o = options();
        // The paper's list of what the compilers reject, and what they accept with a warning.
        expect(forbids("#line\n", o, "takes a digit sequence"));
        expect(forbids("#line sdf\n", o, "not `sdf`"));
        expect(forbids("#line \"xyz\"\n", o, "not `\"xyz\"`"));
        expect(forbids("#line -32\n", o, "not `-`"));
        expect(forbids("#line 0x123\n", o, "not `0x123`"));
        expect(forbids("#line (123)\n", o, "not `(`"));
        expect(forbidden("#line 09\n", o).empty()) << "a decimal digit sequence, not an ill-formed octal number";
        expect(forbids("#line 10 2 3\n", o, "at most one string literal"));
        expect(forbidden("#line 11 \"2\"\n", o).empty());
        expect(forbids("#line 11 \"2\" 3\n", o, "at most one string literal"));
        expect(forbids("#line 12 \"4\" \"5\"\n", o, "at most one string literal")) << "adjacent string literals are not concatenated";
        expect(forbids("#line 13 \"6\" \"7\" \"8\"\n", o, "at most one string literal"));
        expect(forbids("#line 0\n", o, "from 1 to 2147483647, not 0"));
        expect(forbidden("#line 2147483647\n", o).empty());
        expect(forbids("#line 2147483648\n", o, "not 2147483648"));
        expect(forbids("#line 99999999999999\n", o, "from 1 to 2147483647"));
        // The directive is processed as normal text before it is read.
        expect(forbidden("#define N 12\n#line N\n", options()).empty());
        expect(forbids("#define N 0\n#line N\n", o, "not 0", 2));
        expect(forbids("#define NAME sdf\n#line NAME \"f\"\n", o, "not `sdf`", 2));
    };

    "[cpp.replace.general]/13: a directive in a macro invocation's arguments is ill-formed"_test = [] {
        const auto o = options();
        // The paper's DECLARE_CONSTRUCTOR: the IFNDR form, and its well-formed workaround.
        const std::string head { "#define DECLARE_CONSTRUCTOR( CLASS, TYPE, PARAM) CLASS (TYPE PARAM);\nstruct Any {\n  template <class T>\n" };
        const std::string ifndr { head + "  DECLARE_CONSTRUCTOR( Any\n#if defined __cpp_rvalue_references\n   , T &&\n#else\n   , T const &\n#endif\n   , arg_name\n   );\n};\n" };
        expect(forbids(ifndr, o, "`#if` is a directive in the arguments of a macro invocation", 5));
        expect(forbids(ifndr, o, "`#else`", 7));
        expect(forbids(ifndr, o, "`#endif`", 9));
        const std::string workaround { "#define DECLARE_CONSTRUCTOR( CLASS, TYPE, PARAM) CLASS (TYPE PARAM);\nstruct Any {\n"
                                       "#if defined __cpp_rvalue_references\n  template <class T>\n  DECLARE_CONSTRUCTOR( Any, T &&, arg_name );\n#else\n"
                                       "  template <class T>\n  DECLARE_CONSTRUCTOR( Any, T const &, arg_name );\n#endif\n};\n" };
        expect(forbidden(workaround, o).empty()) << "directives around the invocation are fine";
        expect(forbids("#define F(a, b) a b\nF(1,\n#define X 2\n2)\n", o, "`#define`", 3));
        expect(forbids("#define F(a) a\nF(\n#pragma once\n1)\n", o, "`#pragma`", 3));
        // Between a function-like macro's name and the next token it is not (yet) an invocation.
        expect(forbidden("#define F(a) a\nF\n#if 1\n(1)\n#endif\n", o).empty());
        // The same text, read without the feature, is what it always was.
        expect(text("#define F(a, b) a b\nF(1,\n#define X 2\n2)\n", options(23, std::nullopt)) == "1 2");
    };

    "[cpp.stringize]/2: a string that would end in a lone backslash is ill-formed; a new-line becomes \\n"_test = [] {
        const std::string backslash { "#define TO_TEXT(a) #a\n#define TEXT(a) TO_TEXT(a)\n#define BACKSLASH \\\\\n\nint main() {\n    const char *x = TEXT(BACKSLASH);\n}\n" };
        // As Clang, the backslash is ignored and the literal is empty; with the feature it is an error as well.
        expect(text(backslash, options()) == "int main ( ) { const char * x = \"\" ; }");
        expect(forbids(backslash, options(), "invalid string literal", 6));
        expect(text(backslash, options(23, std::nullopt)) == "int main ( ) { const char * x = \"\" ; }");
        const auto pp = f::preprocess(backslash, options(23, std::nullopt));
        expect(std::ranges::any_of(pp.diagnostics, [](const auto& d) { return d.severity == f::Diagnostic::Severity::warning && d.message.find("ignoring final") != std::string::npos; }));
        // Two backslashes are one escaped backslash: valid.
        expect(text("#define S(a) #a\nS(\\\\)\n", options()) == "\"\\\\\"");
        expect(forbidden("#define S(a) #a\nS(\"\\\\\")\n", options()).empty());
        // CWG1709, P2843R3: a new-line in a raw string literal is \n in the string (Clang's reading, always).
        for (const auto& o : { options(), options(23, std::nullopt) })
            expect(text("#define S(x) #x\nconst char* a = S(R\"(a\nb)\");\n", o) == "const char * a = \"R\\\"(a\\nb)\\\"\" ;");
    };

    "[cpp.concat]/3: a ## that does not form a token is an error, as Clang and GCC have it"_test = [] {
        const std::string source { "#define DO_CONCAT(a,b) a##b\n#define CONCAT(a,b) DO_CONCAT(a,b)\n#define MINUS -\nint main() {\n    int word = 0;\n    auto x = CONCAT(MINUS, word);\n}\n" };
        for (const auto& o : { options(), options(23, std::nullopt) }) {
            const auto pp = f::preprocess(source, o);
            expect(std::ranges::any_of(pp.diagnostics, [](const auto& d) { return d.severity == f::Diagnostic::Severity::error && d.message.find("pasting formed '-word'") != std::string::npos; }));
        }
    };

    "[lex.comment]: a form feed or a vertical tab in a // comment is well-formed, whatever follows it"_test = [] {
        const auto o = options();
        const std::string source { "int a; // a \f b\nint b; // \v c\nint c; /* \f */ int d;\n" };
        const auto pp = f::preprocess(source, o);
        expect(pp.diagnostics.empty() && text(source, o) == "int a ; int b ; int c ; int d ;");
    };

    "[cpp.include], [cpp.cond]: what the paper keeps IFNDR is not made an error"_test = [] {
        // Not diagnosed: a macro that expands to defined, in the cases every compiler takes the same way.
        const auto o = options();
        expect(forbidden("#define D defined\n#define X 1\n#if D X\n#endif\n", o).empty());
        expect(forbidden("#define _Reserved 1\n#define __reserved 2\n", o).empty()) << "reserved identifiers stay IFNDR";
    };

    return report();
}
