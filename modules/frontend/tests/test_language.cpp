// mcxx.frontend:standard and the identifiers of P3658R1 (ML-F): which standard a file is read as, which
// language features of a newer one are on in it, and the mathematical notation profile's characters in
// identifiers -- lexed as one identifier in every mode, as Clang does, and a use of c++29:unicode-identifier-
// recommendations that is a gate diagnostic when the feature is not on.
import std;
import mcxx.testing;
import mcxx.frontend;

namespace f = mcxx::frontend;

namespace {

f::PreprocessOptions options(int year, std::optional<f::FeatureLevel> level) {
    f::PreprocessOptions o;
    o.language.standard.year = year;
    if (level) o.language.features["c++29:unicode-identifier-recommendations"] = *level;
    return o;
}

std::vector<std::string> gates(std::string_view source, const f::PreprocessOptions& o) {
    std::vector<std::string> out;
    for (const auto& d : f::preprocess(source, o).diagnostics)
        if (d.feature == "c++29:unicode-identifier-recommendations") out.push_back(std::format("{}:{} {}", d.at.line, d.at.column, d.paper));
    return out;
}

std::vector<std::string> tokens(std::string_view source) {
    std::vector<std::string> out;
    for (const auto& t : f::lex(source)) out.push_back(f::spelling(source, t));
    return out;
}

} // namespace

int main() {
    using namespace mcxx::testing;

    "C++98 and C++03 come before every later standard, though their two digits are larger"_test = [] {
        f::Language old;
        old.standard = *f::parse_standard("c++03");
        old.features["c++26:embed"] = f::FeatureLevel::allow;
        expect(!old.has(f::EMBED) && !old.on(f::EMBED)) << "a C++03 file has no C++26 feature";
        f::Language now;
        now.standard = *f::parse_standard("c++2c");
        now.features["c++26:embed"] = f::FeatureLevel::allow;
        expect(now.on(f::EMBED) && !now.has(f::EMBED_OFFSET));
    };

    "-std's spellings: Clang's, by the standard's two digits"_test = [] {
        const auto year = [](std::string_view name) { const auto s = f::parse_standard(name); return s ? s->year : -1; };
        expect(year("c++98") == 98 && year("c++03") == 98 && year("gnu++98") == 98);
        expect(year("c++11") == 11 && year("c++0x") == 11 && year("c++14") == 14 && year("c++1y") == 14 && year("c++17") == 17 && year("c++1z") == 17);
        expect(year("c++20") == 20 && year("c++2a") == 20 && year("c++23") == 23 && year("c++2b") == 23);
        expect(year("c++26") == 26 && year("c++2c") == 26 && year("c++29") == 29 && year("c++2d") == 29);
        expect(f::parse_standard("gnu++2c")->gnu && !f::parse_standard("c++2c")->gnu);
        expect(!f::parse_standard("c17") && !f::parse_standard("c++latest") && !f::parse_standard("") && !f::parse_standard("c++") && !f::parse_standard("gnu99"));
    };

    "__cplusplus follows the standard, as Clang 23.1 has it"_test = [] {
        const auto value = [](std::string_view name) {
            f::PreprocessOptions o;
            o.language.standard = *f::parse_standard(name);
            const auto pp = f::preprocess("__cplusplus", o);
            return pp.tokens.empty() ? std::string {} : std::string { pp.tokens[0].spelling };
        };
        expect(value("c++98") == "199711L" && value("c++11") == "201103L" && value("c++14") == "201402L" && value("c++17") == "201703L");
        expect(value("c++20") == "202002L" && value("c++23") == "202302L" && value("c++26") == "202400L" && value("c++2d") == "202700L");
        expect(f::preprocess("__cplusplus", {}).tokens[0].spelling == "202302L") << "C++23 unless the command says otherwise";
    };

    "a feature is on when its standard is the file's or earlier, and its level is not deny"_test = [] {
        f::Language l;
        l.standard = *f::parse_standard("c++2c");
        l.features["c++26:embed"] = f::FeatureLevel::allow;
        l.features["c++29:embed-offset-parameter"] = f::FeatureLevel::allow;
        l.features["c++26:preprocessing-never-undefined"] = f::FeatureLevel::deny;
        expect(l.on(f::EMBED) && l.has(f::EMBED));
        expect(!l.on(f::EMBED_OFFSET) && !l.has(f::EMBED_OFFSET)) << "C++29's paper in a C++26 file, though its level is allow";
        expect(!l.on(f::PREPROCESSING_NEVER_UNDEFINED) && l.has(f::PREPROCESSING_NEVER_UNDEFINED)) << "deny";
        expect(!l.on(f::UNICODE_IDENTIFIERS) && l.level("anything") == f::FeatureLevel::deny) << "an id that is not there is deny";
        l.standard = *f::parse_standard("c++2d");
        expect(l.on(f::EMBED_OFFSET));
        // The four features are the registry's: ids and papers as plugins/lang/cpp26 and cpp29 have them.
        expect(f::EMBED.id == "c++26:embed" && f::EMBED.paper == "P1967R14" && f::EMBED_OFFSET.id == "c++29:embed-offset-parameter" && f::EMBED_OFFSET.paper == "P3540R3");
        expect(f::PREPROCESSING_NEVER_UNDEFINED.paper == "P2843R3" && f::UNICODE_IDENTIFIERS.id == "c++29:unicode-identifier-recommendations" &&
               f::UNICODE_IDENTIFIERS.paper == "P3658R1" && f::LANGUAGE_FEATURES.size() == 4);
    };

    "P3658R1's table: what the mathematical notation profile adds is one identifier, in every mode"_test = [] {
        // ∇f, ∂Ω, C∞ (invalid before), x², x₂ and the mathematical alphanumerics: valid under C++11 and again now.
        const std::string source { "int \xE2\x88\x87" "f = 1, \xE2\x88\x82" "\xCE\xA9 = 2, C\xE2\x88\x9E = 3, x\xC2\xB2 = 4, x\xE2\x82\x82 = 5, \xF0\x9D\x9B\x81" "f = 6, \xF0\x9D\x9C\x95" "\xCE\xA9 = 7;" };
        const auto t = tokens(source);
        expect(t.size() == 29 && t[1] == "\xE2\x88\x87" "f" && t[5] == "\xE2\x88\x82" "\xCE\xA9" && t[9] == "C\xE2\x88\x9E" && t[13] == "x\xC2\xB2" && t[17] == "x\xE2\x82\x82") << t.size();
        for (const auto& t : f::lex(source)) expect(t.kind != f::Kind::unknown) << "no token is unknown, in any mode";
    };

    "the gate: a use of the profile's characters is a gate diagnostic unless the feature is on (C++29, level not deny)"_test = [] {
        const std::string source { "int \xE2\x88\x87" "f;\nint x\xC2\xB2;\nint C\xE2\x88\x9E;\nint plain;\n" };
        // Not on: one diagnostic per identifier, naming the paper; the tokens are the same.
        for (const auto& o : { options(23, std::nullopt), options(29, f::FeatureLevel::deny), options(26, f::FeatureLevel::allow) }) {
            expect(gates(source, o) == std::vector<std::string> { "1:5 P3658R1", "2:5 P3658R1", "3:5 P3658R1" });
            expect(f::preprocess(source, o).tokens.size() == 12);
        }
        // On.
        expect(gates(source, options(29, f::FeatureLevel::allow)).empty());
        const auto warn = f::preprocess(source, options(29, f::FeatureLevel::warn));
        expect(warn.diagnostics.size() == 3 && warn.diagnostics[0].severity == f::Diagnostic::Severity::warning);
        const auto off = f::preprocess(source, options(23, std::nullopt));
        expect(off.diagnostics[0].severity == f::Diagnostic::Severity::error && off.diagnostics[0].message.find("c++29:unicode-identifier-recommendations") != std::string::npos &&
               off.diagnostics[0].message.find("read as C++23") != std::string::npos)
            << off.diagnostics[0].message;
    };

    "what XID_Start and XID_Continue already gave is not a use of the feature"_test = [] {
        // Hawaiʻi (U+02BB), ǃnu (U+01C3), fʹ (U+02B9), grad_𝑓 (U+1D453), xⁿ (U+207F), 𓋴𓅱𓎛𓏏𓆇: valid since C++11, and after P1949.
        const std::string source { "int Hawai\xCA\xBBi, \xC7\x83nu, f\xCA\xB9, grad_\xF0\x9D\x91\x93, x\xE2\x81\xBF, \xF0\x93\x8B\xB4\xF0\x93\x85\xB1;\n" };
        expect(gates(source, options(23, std::nullopt)).empty());
        // And an emoji is neither: the lexer says unknown, in every mode (the paper's last row stays invalid).
        const auto t = f::lex("\xF0\x9F\x9C\x85");
        expect(t.size() == 1 && t[0].kind == f::Kind::unknown);
        expect(gates("int \xF0\x9F\x9C\x85;\n", options(23, std::nullopt)).empty());
    };

    "universal-character-names count: \\u00B2 continues an identifier, \\u2207 starts one"_test = [] {
        const std::string source { "int x\\u00B2;\nint \\u2207f;\nint \\u00E9;\nint y\\u0301;\nint \\U0001D6C1g;\n" };
        const auto t = tokens(source);
        expect(t[1] == "x\\u00B2" && t[4] == "\\u2207f" && t[7] == "\\u00E9" && t[10] == "y\\u0301" && t[13] == "\\U0001D6C1g") << t.size();
        // x² ∇f 𝛁g use the profile; é and a combining acute are XID.
        expect(gates(source, options(23, std::nullopt)) == std::vector<std::string> { "1:5 P3658R1", "2:5 P3658R1", "5:5 P3658R1" });
        expect(gates(source, options(29, f::FeatureLevel::allow)).empty());
    };

    "uses_mathematical_notation reads one token: the first character against the start set, the rest against the continue set"_test = [] {
        const auto uses = [](std::string_view source) {
            const auto t = f::lex(source);
            return t.size() == 1 && t[0].kind == f::Kind::raw_identifier && f::uses_mathematical_notation(source, t[0]);
        };
        expect(uses("\xE2\x88\x87" "f") && uses("x\xC2\xB2") && uses("x\xE2\x82\x82") && uses("\xE2\x88\x9E") && uses("x\\u00B2") && uses("\\u2207"));
        expect(!uses("plain") && !uses("\xC3\xA9") && !uses("\\u00E9") && !uses("x\\u0301") && !uses("a\xCA\xBB" "b") && !uses("_x1$"));
        // Not an identifier at all: a superscript cannot start one, the lexer says unknown.
        const std::string two { "\xC2\xB2" "x" };
        const auto t = f::lex(two);
        expect(t.size() == 2 && t[0].kind == f::Kind::unknown && !f::uses_mathematical_notation(two, t[0]) && !f::uses_mathematical_notation(two, t[1]));
    };

    return report();
}
