// mcxx.frontend:lex on the cases that are easy to get wrong.
import std;
import mcxx.testing;
import mcxx.frontend;

namespace f = mcxx::frontend;

namespace {

std::vector<std::string> kinds(std::string_view text) {
    std::vector<std::string> out;
    for (const auto& t : f::lex(text)) out.emplace_back(f::name(t.kind));
    return out;
}

std::vector<std::string> texts(std::string_view text) {
    std::vector<std::string> out;
    for (const auto& t : f::lex(text)) out.emplace_back(text.substr(t.begin, t.end - t.begin));
    return out;
}

} // namespace

int main() {
    using namespace mcxx::testing;

    "punctuators, longest first; digraphs are their tokens; <:: is < ::"_test = [] {
        expect(kinds("a<=>b ->* .* ... :: >>= <% %> <: :> %: %:%:") ==
               std::vector<std::string> { "raw_identifier", "spaceship", "raw_identifier", "arrowstar", "periodstar", "ellipsis", "coloncolon",
                                          "greatergreaterequal", "l_brace", "r_brace", "l_square", "r_square", "hash", "hashhash" });
        expect(kinds("std::vector<::std::string> x; a<:::b") ==
               std::vector<std::string> { "raw_identifier", "coloncolon", "raw_identifier", "less", "coloncolon", "raw_identifier", "coloncolon",
                                          "raw_identifier", "greater", "raw_identifier", "semi", "raw_identifier", "l_square", "coloncolon", "raw_identifier" });
    };

    "literals: every prefix, raw strings, user-defined suffixes, digit separators"_test = [] {
        expect(kinds(R"(L"a" u8"b" u"c" U"d" 'e' L'f' u8'g' u'h' U'i')") ==
               std::vector<std::string> { "wide_string_literal", "utf8_string_literal", "utf16_string_literal", "utf32_string_literal", "char_constant",
                                          "wide_char_constant", "utf8_char_constant", "utf16_char_constant", "utf32_char_constant" });
        expect(texts("R\"x(a)\"\n)x\"  12_km 1'000'000 0x1.8p+3 1.5e-3f \"s\"sv") ==
               std::vector<std::string> { "R\"x(a)\"\n)x\"", "12_km", "1'000'000", "0x1.8p+3", "1.5e-3f", "\"s\"sv" });
    };

    "a backslash before a line break joins the lines, even inside a token; not inside a raw string"_test = [] {
        const std::string text { "int ab\\\ncd = 1;\n#define X \\\n  2\nauto r = R\"(a\\\nb)\";" };
        const auto t = texts(text);
        expect(t[1] == "ab\\\ncd" && f::spelling(text, f::lex(text)[1]) == "abcd");
        expect(std::ranges::find(t, "R\"(a\\\nb)\"") != t.end());
    };

    "lines and columns are the token's first byte; the first on a line says so"_test = [] {
        const auto t = f::lex("int a;\n  b = 1; // x\n");
        expect(t[3].line == 2 && t[3].column == 3 && t[3].start_of_line && !t[4].start_of_line);
        expect(f::lex("/* a\nb */ x", { .comments = true }).size() == 2);
    };

    return report();
}
