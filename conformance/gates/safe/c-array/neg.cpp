template <class T, unsigned long N> struct array { T data[N]; };   // expect: c-array
array<int, 4> wrapped;
int* pointer = nullptr;
int value = 4;
struct Holder { array<char, 16> bytes; };
const char* name = "not an array variable";
