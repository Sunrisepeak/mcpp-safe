namespace std {
template <class E> class initializer_list {
    const E* b; unsigned long n;
    constexpr initializer_list(const E* b, unsigned long n) : b(b), n(n) {}
public:
    constexpr initializer_list() : b(nullptr), n(0) {}
};
}
namespace nlohmann { inline namespace json_abi_v3_12_0 {
template <class T = int> class basic_json {
public:
    basic_json(decltype(nullptr) = nullptr) {}
    basic_json(int) {}
    basic_json(std::initializer_list<basic_json>) {}
    basic_json(const basic_json&) = default;
};
} using json = basic_json<>; }
using Json = nlohmann::json;
Json one { 1 };                                            // expect: json-brace-init
Json copy_list = { 2 };                                    // expect: json-brace-init
struct Pending { Json result { nullptr }; };               // expect: json-brace-init
Json wrap(const Json& j) { Json w { j }; return w; }       // expect: json-brace-init
nlohmann::basic_json<> spelled { 3 };                      // expect: json-brace-init
