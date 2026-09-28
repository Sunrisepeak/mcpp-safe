namespace std { template <class T> struct vector { T* data; }; }
std::vector<int> numbers;                                  // expect: lib:std.vector
struct Holder { std::vector<char> bytes; };                // expect: lib:std.vector
int count(const std::vector<int>& v) { return 0; }         // expect: lib:std.vector
std::vector<std::vector<int>> grid;                        // expect: lib:std.vector
using Row = std::vector<double>;                           // expect: lib:std.vector
