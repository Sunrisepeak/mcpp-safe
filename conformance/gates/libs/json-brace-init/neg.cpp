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
Json copy = 1;
Json object { { 1, 2 } };
Json two { 1, 2 };
Json empty {};
Json direct(4);
struct Pending { Json result = nullptr; };
