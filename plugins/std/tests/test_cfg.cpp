// [[mcpp::cfg]] as text: predicates, what is blanked, and that nothing moves.
import std;
import mcxx.testing;
import mcxx.plugin;
import mcxx.plugins.cfg;

namespace plugin = mcxx::plugin;

namespace {

plugin::Target linux_x64() {
    return { .triple = "x86_64-unknown-linux-gnu", .os = "linux", .family = "unix", .arch = "x86_64", .env = "gnu", .pointer_width = 64,
             .endian = "little", .features = { "SIMD" }, .debug_assertions = true };
}

std::string run(std::string_view text, const plugin::Target& target = linux_x64()) {
    mcxx::plugins::cfg::Filter filter;
    const auto out = filter.filter({ "a.cpp", target }, text);
    return out.text ? *out.text : std::string { text };
}

std::string squash(std::string_view text) {   // spaces collapsed, for reading the result
    std::string out;
    for (const char c : text) {
        if (c == ' ' && (out.empty() || out.back() == ' ' || out.back() == '\n')) continue;
        out += c;
    }
    return out;
}

} // namespace

int main() {
    using namespace mcxx::testing;
    const plugin::Target target { linux_x64() };

    "predicates"_test = [&] {
        using mcxx::plugins::cfg::evaluate;
        expect(evaluate("unix", target) == true && evaluate("windows", target) == false && evaluate("linux", target) == true);
        expect(evaluate("target_os = \"linux\"", target) == true && evaluate("os = \"macos\"", target) == false);
        expect(evaluate("all(target_arch = \"x86_64\", target_pointer_width = \"64\", target_endian = \"little\")", target) == true);
        expect(evaluate("any(windows, target_env = \"gnu\")", target) == true);
        expect(evaluate("not(debug_assertions)", target) == false);
        expect(evaluate("feature = \"simd\"", target) == true && evaluate("feature = \"avx-512\"", target) == false);
        expect(!evaluate("target_colour = \"red\"", target).has_value());
        expect(!evaluate("not(a, b)", target).has_value());
        expect(!evaluate("unix unix", target).has_value());
    };

    "a declaration the target does not have is blanked, with its export; the rest stays in place"_test = [&] {
        const std::string text {
            "export module plat;\n"
            "export [[mcpp::cfg(windows)]] void open_console() { AllocConsole(); }\n"
            "[[mcpp::cfg(unix)]] int fd = 0;\n"
            "[[nodiscard]] [[mcpp::cfg(target_os = \"macos\")]] int kq();\n"
            "[[mcpp::cfg(windows)]] struct Handle { void* h; } handle;\n"
            "[[mcpp::cfg(windows)]] namespace win { int x = 1; }\n"
            "int after = 2;\n" };
        const std::string out { run(text) };
        expect(out.size() == text.size() && std::ranges::count(out, '\n') == std::ranges::count(text, '\n')) << "nothing moves";
        expect(squash(out) == "export module plat;\n\nint fd = 0;\n\n\n\nint after = 2;\n") << squash(out);
    };

    "functions, constructors with initializer lists, templates in them, statements"_test = [&] {
        const std::string text {
            "struct S {\n"
            "  [[mcpp::cfg(windows)]] S() : a{1}, b(2) { Win(); }\n"
            "  int a, b;\n"
            "};\n"
            "int f(int n) {\n"
            "  [[mcpp::cfg(windows)]] n += WinOnly();\n"
            "  [[mcpp::cfg(unix)]] n += 1;\n"
            "  return n;\n"
            "}\n"
            "[[mcpp::cfg(windows)]] auto g() -> int { return Win(); }\n"
            "[[mcpp::cfg(windows)]] const char* s = \"not ;here\";\n"
            "int tail;\n" };
        expect(squash(run(text)) == "struct S {\n\nint a, b;\n};\nint f(int n) {\n\nn += 1;\nreturn n;\n}\n\n\nint tail;\n") << squash(run(text));
    };

    "a predicate that cannot be read is an error at the attribute"_test = [&] {
        mcxx::plugins::cfg::Filter filter;
        const auto out = filter.filter({ "a.cpp", target }, "\n[[mcpp::cfg(colour = \"red\")]] int x;\n");
        expect(fatal(out.problems.size() == 1));
        expect(out.problems[0].range.begin.line == 1 && out.problems[0].message.contains("unknown key `colour`")) << out.problems[0].message;
    };

    "cfg inside comments and strings is not an attribute"_test = [&] {
        const std::string text { "// [[mcpp::cfg(windows)]] int a;\nconst char* b = \"[[mcpp::cfg(windows)]] int c;\";\n" };
        expect(run(text) == text);
    };

    return report();
}
