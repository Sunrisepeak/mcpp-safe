double half(int v) { return (double)v / 2; }                      // expect: c-style-cast
long address(int* p) { return (long)p; }                          // expect: c-style-cast reinterpret-cast
int* mutable_of(const int* p) { return (int*)p; }                 // expect: c-style-cast -- a const_cast, unsaid
unsigned char low(int v) { return (unsigned char)v; }             // expect: c-style-cast
int truncate(double d) { return int(d); }                         // expect: c-style-cast -- T(x) to a scalar is (T)x
