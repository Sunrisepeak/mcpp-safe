// #embed and __has_embed (P1967R14, with P3540R3's offset parameter, ML-F): the papers' own examples, the
// parameters and their errors, what a resource that is empty or missing does, and the gate -- the feature's MC1
// id and paper named when the standard or the file's level does not have it.
import std;
import mcxx.testing;
import mcxx.frontend;

namespace f = mcxx::frontend;

namespace {

using Resources = std::map<std::string, std::string, std::less<>>;

// C++26 with the features on (`all`), unless asked otherwise; the resources are the map's, by the name between
// the delimiters.
f::PreprocessOptions options(Resources resources, int year = 29, f::FeatureLevel level = f::FeatureLevel::allow) {
    f::PreprocessOptions o;
    o.language.standard.year = year;
    for (const auto& feature : f::LANGUAGE_FEATURES) o.language.features[std::string { feature.id }] = level;
    o.read_resource = [resources = std::move(resources)](std::string_view header) -> std::optional<std::string> {
        const auto it = resources.find(header.substr(1, header.size() - 2));
        if (it == resources.end()) return std::nullopt;
        return it->second;
    };
    return o;
}

std::string text(const f::Preprocessed& pp) {
    std::string out;
    for (const auto& t : pp.tokens) out += (out.empty() ? "" : " ") + std::string { t.spelling };
    return out;
}

// What the source preprocesses to, as its tokens' spellings joined by a space; the errors, when asked, follow after " | ".
std::string run(std::string_view source, const f::PreprocessOptions& o, bool with_errors = false) {
    const auto pp = f::preprocess(source, o);
    std::string out { text(pp) };
    if (with_errors)
        for (const auto& d : pp.diagnostics)
            if (d.severity == f::Diagnostic::Severity::error) out += " | " + d.message;
    return out;
}

bool has_error(const f::Preprocessed& pp, std::string_view fragment) {
    return std::ranges::any_of(pp.diagnostics, [&](const auto& d) { return d.severity == f::Diagnostic::Severity::error && d.message.find(fragment) != std::string::npos; });
}

std::string bytes(std::initializer_list<int> values) {
    std::string out;
    for (const int v : values) out += static_cast<char>(v);
    return out;
}

} // namespace

int main() {
    using namespace mcxx::testing;

    "[cpp.embed.gen]: the directive is a comma-delimited list of integer literals, one per byte"_test = [] {
        const auto o = options({ { "i.dat", bytes({ 42 }) }, { "d.bin", bytes({ 104, 101, 0, 255 }) } });
        expect(run("int i = {\n#embed \"i.dat\"\n}; // well-formed if i.dat produces a single value\n", o) == "int i = { 42 } ;");
        expect(run("int i2 =\n#embed \"i.dat\"\n; // also well-formed\n", o) == "int i2 = 42 ;");
        expect(run("unsigned char d[] = {\n#embed <d.bin>\n};", o) == "unsigned char d [ ] = { 104 , 101 , 0 , 255 } ;");
        const auto pp = f::preprocess("#embed <d.bin>\n#embed \"i.dat\"\n", o);
        expect(pp.embeds.size() == 2 && pp.embeds[0].header == "<d.bin>" && pp.embeds[0].found && pp.embeds[0].count == 4 && pp.embeds[1].count == 1);
        expect(pp.certain && pp.diagnostics.empty());
    };

    "a \"name\" that is not found is searched as <name> ([cpp.embed.gen])"_test = [] {
        // The host's reader above takes both: the second try is the preprocessor's own. Here only <x.bin> is there.
        f::PreprocessOptions o { options({}) };
        o.read_resource = [](std::string_view header) -> std::optional<std::string> { return header == "<x.bin>" ? std::optional<std::string> { bytes({ 7 }) } : std::nullopt; };
        expect(run("#embed \"x.bin\"\n", o) == "7");
        expect(run("#embed <x.bin>\n", o) == "7");
        expect(run("#embed \"y.bin\"\n", o, true).find("file not found") != std::string::npos);
    };

    "[cpp.embed.param.limit]: limit is a constant-expression, and the count is at most the resource's"_test = [] {
        const auto o = options({ { "data.dat", bytes({ 1, 2, 3, 4, 5, 6 }) }, { "jump.wav", bytes({ 1, 2, 3, 4, 5 }) } });
        expect(run("constexpr unsigned char sound_signature[] = {\n#embed <jump.wav> limit(2+2)\n};\nstatic_assert(sizeof(sound_signature) == 4);\n", o) ==
               "constexpr unsigned char sound_signature [ ] = { 1 , 2 , 3 , 4 } ; static_assert ( sizeof ( sound_signature ) == 4 ) ;");
        expect(run("#embed <data.dat> limit(100)\n", o) == "1 , 2 , 3 , 4 , 5 , 6");
        expect(run("#embed <data.dat> limit(0)\n", o).empty());
        // The limit is read as normal text once: a macro in it is replaced, and what it names is not replaced again.
        expect(run("#define LIM 3\n#embed <data.dat> limit(LIM)\n", o) == "1 , 2 , 3");
        // [Example] __has_include is fine in a #embed's limit.
        expect(run("#embed <data.dat> limit(__has_include(\"a.h\"))\n", o) == "");
    };

    "[cpp.embed.param.prefix], suffix: placed around the list, ignored when the resource is empty"_test = [] {
        const auto o = options({ { "ches.glsl", bytes({ 97, 98 }) }, { "empty.glsl", {} } });
        const std::string source { "constexpr unsigned char whl[] = {\n#embed \"%s\" \\\n  prefix(0xEF, 0xBB, 0xBF, ) /* a sequence of bytes */ \\\n  suffix(,)\n  0\n};\n" };
        const auto with = [&](std::string_view name) {
            std::string s { source };
            s.replace(s.find("%s"), 2, name);
            return s;
        };
        expect(run(with("ches.glsl"), o) == "constexpr unsigned char whl [ ] = { 0xEF , 0xBB , 0xBF , 97 , 98 , 0 } ;");
        expect(run(with("empty.glsl"), o) == "constexpr unsigned char whl [ ] = { 0 } ;") << "always null terminated";
    };

    "[cpp.embed.param.if.empty]: if_empty replaces an empty resource, limit(0) makes one empty"_test = [] {
        const auto o = options({ { "uwurandom", bytes({ 9, 9, 9 }) }, { "single_byte", bytes({ 1 }) }, { "nothing", {} } });
        expect(run("#embed <uwurandom> if_empty(42203) limit(0)\n", o) == "42203");
        expect(run("#embed <uwurandom> if_empty(42203)\n", o) == "9 , 9 , 9");
        expect(run("#embed <nothing> if_empty(1, 2)\n", o) == "1 , 2");
        expect(run("#embed <nothing> prefix(1) suffix(2)\n", o).empty());
        expect(run("#embed <single_byte> offset(1) if_empty(42203)\n", o) == "42203") << "P3540R3's example";
    };

    "[cpp.embed.gen]: the parameters are replaced as normal text, once, and the third form's directive whole"_test = [] {
        const auto o = options({ { ":3c", bytes({ 1 }) } });
        const std::string macros { "#define prefix(ARG) suffix(ARG)\n#define THE_ADDITION \"teehee\"\n#define THE_RESOURCE \":3c\"\n" };
        expect(run(macros + "#embed \":3c\"        prefix(THE_ADDITION)\n", o) == "1 \"teehee\"");
        expect(run(macros + "#embed THE_RESOURCE prefix(THE_ADDITION)\n", o) == "1 \"teehee\"");
        // What the prefix gave is not a macro invocation again.
        expect(run("#define X 9\n#embed \":3c\" prefix(X)\n", o) == "9 1");
        expect(run("#define X 9\n#define Y X\n#embed \":3c\" prefix(Y)\n", o) == "9 1");
    };

    "[cpp.embed.param.offset] (P3540R3): offset skips elements, before limit is applied"_test = [] {
        const auto o = options({ { "jump.wav", bytes({ 10, 11, 12, 13, 14 }) } });
        expect(run("#embed <jump.wav> offset(2)\n", o) == "12 , 13 , 14");
        expect(run("#embed <jump.wav> offset(1) limit(1)\n", o) == "11");
        expect(run("#embed <jump.wav> limit(2) offset(1)\n", o) == "11 , 12") << "the order does not matter";
        expect(run("#embed <jump.wav> limit(3) offset(3)\n", o) == "13 , 14") << "limit is not applied before offset";
        expect(run("#embed <jump.wav> offset(0)\n", o) == "10 , 11 , 12 , 13 , 14");
        expect(run("#embed <jump.wav> offset(5)\n", o, true) == "") << "at the size: empty";
        expect(run("#embed <jump.wav> offset(99) if_empty(7)\n", o) == "7") << "past it: empty";
        // The vendor spelling Clang and GCC have: no gate.
        expect(run("#embed <jump.wav> clang::offset(3)\n", options({ { "jump.wav", bytes({ 10, 11, 12, 13, 14 }) } }, 23, f::FeatureLevel::deny)) == "13 , 14");
    };

    "__has_embed: 1 for a resource with elements, 2 for an empty one, 0 for none or a parameter not supported"_test = [] {
        const auto o = options({ { "data.dat", bytes({ 1, 2, 3 }) }, { "nothing", {} } });
        expect(run("#if __has_embed(<data.dat>) == __STDC_EMBED_FOUND__\nfound\n#endif\n", o) == "found");
        expect(run("#if __has_embed(<nothing>) == __STDC_EMBED_EMPTY__\nempty\n#endif\n", o) == "empty");
        expect(run("#if __has_embed(<gone>) == __STDC_EMBED_NOT_FOUND__\ngone\n#endif\n", o) == "gone");
        expect(run("#if __has_embed(\"data.dat\" limit(0)) == 2\nzero\n#endif\n", o) == "zero") << "limit(0) makes it empty, in __has_embed too";
        expect(run("#if __has_embed(<data.dat> acme::open_mode(\"x\"))\nyes\n#else\nunsupported\n#endif\n", o) == "unsupported") << "an unrecognized parameter is not an error";
        expect(run("#undef DATA_LIMIT\n#if __has_embed(<data.dat> limit(DATA_LIMIT)) == 2\nzero\n#endif\n", o) == "zero") << "[Example] DATA_LIMIT is 0";
        expect(run("#if defined(__has_embed) && defined __has_embed\nyes\n#endif\n", o) == "yes") << "the standard treats it as a defined macro";
        // The third form's directive: macros replaced first.
        expect(run("#define NAME <data.dat>\n#define P limit(1)\n#if __has_embed(NAME P) == 1\nok\n#endif\n", o) == "ok");
        const auto pp = f::preprocess("#if __has_embed(<data.dat> limit(__has_include(\"a.h\")))\n#endif\n", o);
        expect(has_error(pp, "cannot appear")) << "[Example] __has_include cannot appear in a __has_embed";
        expect(run("#if __has_embed(<data.dat> offset(1)) == 1\noffset\n#endif\n", o) == "offset") << "P3540R3 in a __has_embed";
    };

    "__has_embed, the example of the paper: a resource that is empty by limit"_test = [] {
        const auto o = options({ { "/owo/uwurandom", bytes({ 3, 3 }) } });
        expect(run("int infinity_zero () {\n#if __has_embed(</owo/uwurandom> limit(0) prefix(some tokens)) == __STDC_EMBED_EMPTY__\n  return 0;\n#else\n#error \"The resource does not exist\"\n#endif\n}\n", o) ==
               "int infinity_zero ( ) { return 0 ; }");
    };

    "ill-formed #embed: unknown parameter, a parameter twice, a negative limit, defined, no resource, no name"_test = [] {
        const auto o = options({ { "d", bytes({ 1, 2 }) } });
        expect(has_error(f::preprocess("#embed <d> foo::bar(3)\n", o), "unknown embed parameter `foo::bar`"));
        expect(has_error(f::preprocess("#embed <d> limit(1) limit(2)\n", o), "more than once"));
        expect(has_error(f::preprocess("#embed <d> limit(-1)\n", o), "negative"));
        expect(has_error(f::preprocess("#embed <d> limit(defined(X))\n", o), "`defined` cannot appear"));
        expect(has_error(f::preprocess("#embed <d> limit\n", o), "takes arguments in parentheses"));
        expect(has_error(f::preprocess("#embed <d> limit(1\n", o), "missing ')'"));
        expect(has_error(f::preprocess("#embed <gone>\n", o), "file not found"));
        expect(has_error(f::preprocess("#embed\n", o), "expected \"FILENAME\" or <FILENAME>"));
        expect(has_error(f::preprocess("#embed d\n", o), "expected \"FILENAME\" or <FILENAME>"));
        expect(has_error(f::preprocess("#embed \"a\" \"b\"\n", o), "unknown") || has_error(f::preprocess("#embed \"a\" \"b\"\n", o), "expected")) << "adjacent string literals are not concatenated";
        expect(run("#embed <d> if_empty(1) if_empty(2)\n", o, true).find("more than once") != std::string::npos);
    };

    "without a reader the host has no resources: what a directive says is not known"_test = [] {
        f::PreprocessOptions o;
        o.language.standard.year = 26;
        o.language.features["c++26:embed"] = f::FeatureLevel::allow;
        const auto pp = f::preprocess("int a[] = {\n#embed <d>\n};\n#if __has_embed(<d>)\n#endif\n", o);
        expect(!pp.certain && pp.embeds.empty() && text(pp) == "int a [ ] = { } ;");
    };

    "the gate: #embed under C++23, or with the feature's level deny, is a gate diagnostic naming the id and the paper"_test = [] {
        const std::string source { "int a[] = {\n#embed <d>\n};\n" };
        // Not enabled in the file: an error that names c++26:embed and P1967R14; the directive still reads (recovery).
        auto o = options({ { "d", bytes({ 5 }) } }, 26, f::FeatureLevel::deny);
        auto pp = f::preprocess(source, o);
        expect(text(pp) == "int a [ ] = { 5 } ;");
        expect(pp.diagnostics.size() == 1 && pp.diagnostics[0].feature == "c++26:embed" && pp.diagnostics[0].paper == "P1967R14" &&
               pp.diagnostics[0].severity == f::Diagnostic::Severity::error && pp.diagnostics[0].message.find("not enabled") != std::string::npos)
            << (pp.diagnostics.empty() ? std::string {} : pp.diagnostics[0].message);
        // The standard of the file is C++23: the same id, and why.
        o = options({ { "d", bytes({ 5 }) } }, 23);
        pp = f::preprocess(source, o);
        expect(pp.diagnostics.size() == 1 && pp.diagnostics[0].feature == "c++26:embed" && pp.diagnostics[0].message.find("read as C++23") != std::string::npos)
            << (pp.diagnostics.empty() ? std::string {} : pp.diagnostics[0].message);
        // No language at all (what every caller before ML-F passes): the same gate.
        f::PreprocessOptions plain;
        plain.read_resource = o.read_resource;
        expect(f::preprocess(source, plain).diagnostics.front().feature == "c++26:embed");
        // On: no diagnostic, and the feature-test macro is there. At level warn, a warning.
        o = options({ { "d", bytes({ 5 }) } }, 26);
        pp = f::preprocess(source + "#if __cpp_pp_embed >= 202502L\nmacro\n#endif\n", o);
        expect(pp.diagnostics.empty() && text(pp).ends_with("macro")) << text(pp);
        o = options({ { "d", bytes({ 5 }) } }, 26, f::FeatureLevel::warn);
        pp = f::preprocess(source, o);
        expect(pp.diagnostics.size() == 1 && pp.diagnostics[0].severity == f::Diagnostic::Severity::warning && pp.diagnostics[0].feature == "c++26:embed");
        expect(f::preprocess("#if defined(__cpp_pp_embed)\nyes\n#endif\n", options({}, 23)).tokens.empty()) << "off: no feature-test macro";
    };

    "the gate of offset: C++29's, its own id; the vendor's clang::offset has none"_test = [] {
        const std::string source { "int a[] = {\n#embed <d> offset(1)\n};\n" };
        // C++26 with #embed on: offset is P3540R3's, C++29, and the file is read as C++26.
        auto o = options({ { "d", bytes({ 5, 6 }) } }, 26);
        auto pp = f::preprocess(source, o);
        expect(text(pp) == "int a [ ] = { 6 } ;");
        expect(pp.diagnostics.size() == 1 && pp.diagnostics[0].feature == "c++29:embed-offset-parameter" && pp.diagnostics[0].paper == "P3540R3" &&
               pp.diagnostics[0].message.find("read as C++26") != std::string::npos)
            << (pp.diagnostics.empty() ? std::string {} : pp.diagnostics[0].message);
        // C++29 with its level deny, #embed allowed.
        o = options({ { "d", bytes({ 5, 6 }) } }, 29);
        o.language.features["c++29:embed-offset-parameter"] = f::FeatureLevel::deny;
        pp = f::preprocess(source, o);
        expect(pp.diagnostics.size() == 1 && pp.diagnostics[0].feature == "c++29:embed-offset-parameter" && pp.diagnostics[0].message.find("not enabled") != std::string::npos);
        // Both on: nothing.
        expect(f::preprocess(source, options({ { "d", bytes({ 5, 6 }) } }, 29)).diagnostics.empty());
        expect(f::preprocess("int a[] = {\n#embed <d> clang::offset(1)\n};\n", options({ { "d", bytes({ 5, 6 }) } }, 26)).diagnostics.empty());
    };

    return report();
}
