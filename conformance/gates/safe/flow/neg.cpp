[[noreturn]] void halt();
void fill(int& out);
int both(bool c) { int x; if (c) x = 1; else x = 2; return x; }   // expect: uninitialized
int filled() { int y; fill(y); return y; }                          // expect: uninitialized
int pointed() { int z; int* p = &z; *p = 3; return z; }             // expect: uninitialized
int initialized() { int a = 0; return a; }
int checked(int v) {
    if (v > 0) return 1;
    halt();
}
int thrown(int v) {
    if (v > 0) return 1;
    throw v;
}
int forever() { for (;;) {} }
int main() {}
[[noreturn]] void stop() { halt(); }
[[noreturn]] void fail() { throw 1; }
template <class T> T twice(T t) { T u; return u + t; }
