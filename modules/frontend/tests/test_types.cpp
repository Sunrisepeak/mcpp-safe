// A declaration's type as text (mcxx.frontend:types), against what the Clang backend gives for the same
// declarations (mcxx-probe --facts on this source: every expectation here is Clang's).
import std;
import mcxx.testing;
import mcxx.msa;
import mcxx.frontend;

namespace f = mcxx::frontend;

int main() {
    using namespace mcxx::testing;

    "each declaration's type as Clang prints the type as written"_test = [] {
        const std::string_view source {
            "namespace lib {\n"
            "using size_t = unsigned long;\n"
            "struct string {};\n"
            "template <class T, class A = int> struct vec {};\n"
            "template <class K, class V> struct map {};\n"
            "template <class F> struct function {};\n"
            "template <class T, size_t N> struct array {};\n"
            "template <class T> struct uptr {};\n"
            "template <class T> struct opt { using value_type = T; };\n"
            "}\n"
            "namespace ns { struct Widget { int v; }; using Handler = lib::function<void(int level)>; }\n"
            "lib::string a14;\n"
            "const lib::string& a15 = a14;\n"
            "lib::vec<int*> a16;\n"
            "lib::vec<lib::vec<int>> a17;\n"
            "lib::map<lib::string, lib::vec<int>> a18;\n"
            "lib::function<void(int)> a19;\n"
            "lib::array<char, 16> a20;\n"
            "lib::size_t a21;\n"
            "ns::Handler a22;\n"
            "lib::uptr<ns::Widget> a26;\n"
            "lib::function<int(const char*, ...)> a28;\n"
            "lib::function<void() noexcept> a29;\n"
            "lib::array<int, 2 * 8> a30;\n"
            "::lib::string a31;\n"
            "lib::opt<int>::value_type a32;\n"
            "typedef int (*Callback)(void*, lib::size_t);\n"
            "void f(const lib::string& s, lib::vec<int>&& v, lib::map<int, int> const& m);\n"
            "lib::vec<int*> k();\n"
            "constexpr int c1 = 3;\n"
            "constexpr const char* c2 = \"x\";\n"
            "constexpr lib::size_t c3 = 4;\n"
            "struct S { static constexpr int limit = 3; lib::vec<int> items[2]; };\n"
            "template <class T> void g(typename lib::opt<T>::value_type x, T* p, const T& r);\n"
            "unsigned long long int big;\n"
            "long double ld;\n"
            "wchar_t wc;\n"
            "bool flag;\n"
            "decltype(big) same;\n"
            "int *const volatile cvp = nullptr;\n"
            "lib::function<int(int argc, char* argv[])> a33;\n"
            "lib::function<void(int values[3], const char* names[])> a34;\n"
        };
        const auto facts = f::facts(f::parse(source));
        const std::vector<std::pair<std::string_view, std::string_view>> expected {
            { "a14", "lib::string" },
            { "a15", "const lib::string &" },
            { "a16", "lib::vec<int *>" },
            { "a17", "lib::vec<lib::vec<int>>" },
            { "a18", "lib::map<lib::string, lib::vec<int>>" },
            { "a19", "lib::function<void (int)>" },
            { "a20", "lib::array<char, 16>" },
            { "a21", "lib::size_t" },
            { "a22", "ns::Handler" },
            { "a26", "lib::uptr<ns::Widget>" },
            { "a28", "lib::function<int (const char *, ...)>" },
            { "a29", "lib::function<void () noexcept>" },
            { "a30", "lib::array<int, 2 * 8>" },
            { "a31", "::lib::string" },
            { "a32", "lib::opt<int>::value_type" },
            { "Callback", "int (*)(void *, lib::size_t)" },
            { "s", "const lib::string &" },
            { "v", "lib::vec<int> &&" },
            { "m", "const lib::map<int, int> &" },
            { "c1", "const int" },
            { "c2", "const char *const" },
            { "c3", "const lib::size_t" },
            { "S::limit", "const int" },
            { "S::items", "lib::vec<int>[2]" },
            { "x", "typename lib::opt<T>::value_type" },
            { "p", "T *" },
            { "r", "const T &" },
            { "big", "unsigned long long" },
            { "ld", "long double" },
            { "wc", "wchar_t" },
            { "flag", "bool" },
            { "same", "decltype(big)" },
            { "cvp", "int *const volatile" },
            { "a33", "lib::function<int (int, char **)>" },
            { "a34", "lib::function<void (int *, const char **)>" },
        };
        for (const auto& [name, type] : expected) {
            const auto it = std::ranges::find(facts.declarations, name, &mcxx::msa::fact::Declaration::qualified_name);
            expect(it != facts.declarations.end() && it->type == type) << std::format("{}: {} (Clang: {})", name, it != facts.declarations.end() ? it->type : "missing", type);
        }
    };

    "what the tokens do not say is not guessed: a deduced type, a bound that is not a literal"_test = [] {
        const auto facts = f::facts(f::parse("constexpr int N = 4;\nauto a = 1;\nconst auto& b = a;\nint c[N];\nstd::lock_guard d { m };\n"));
        for (const auto* name : { "a", "b", "c" }) {
            const auto it = std::ranges::find(facts.declarations, std::string_view { name }, &mcxx::msa::fact::Declaration::qualified_name);
            expect(it != facts.declarations.end() && it->type.empty()) << name;
        }
    };

    return report();
}
