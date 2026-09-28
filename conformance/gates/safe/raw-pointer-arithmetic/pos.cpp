int plus(int* p) { return *(p + 1); }                       // expect: raw-pointer-arithmetic
int minus(int* p, int* q) { return static_cast<int>(p - q); } // expect: raw-pointer-arithmetic
void step(int* p) { ++p; }                                  // expect: raw-pointer-arithmetic
void advance(const char* s) { s += 2; }                     // expect: raw-pointer-arithmetic
int index(int* p) { return p[3]; }                          // expect: raw-pointer-arithmetic
