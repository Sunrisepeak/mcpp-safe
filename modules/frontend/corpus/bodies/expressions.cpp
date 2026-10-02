// Expressions on builtin types only (no overloaded operators, no headers): self-contained C++23 that Clang
// reads without errors, so its AST is the reference for kinds and ranges (tools/checks/bodiesdiff.py).
struct P {
    int x;
    int y;
    static int shared;
    int get(int a) const { return a + x; }
    P() : x(0), y(0) {}
    P(int a, int b) : x(a), y(b) {}
};
int P::shared = 4;
int global = 3;
int free_fn(int a, int b = 2) { return a - b; }
template <class T> T twice(T v) { return v + v; }

int arithmetic(int a, int b) {
    int c = a * 2 + b / 3 - a % 5;
    c += a << 1;
    c -= b >> 2;
    c *= 2; c /= 3; c %= 4; c &= 7; c |= 8; c ^= 1; c <<= 1; c >>= 1;
    int d = -a + +b + ~c + !a;
    int e = a & b | c ^ d;
    bool f = a < b && b <= c || a > b && b >= c;
    bool g = a == b || a != b;
    int h = a > 0 ? a : -a;
    int i = (a + b) * (c - d);
    int j = (a, b, c);
    return c + d + e + f + g + h + i + j;
}

int access(int a, int* p, int (&arr)[3], P obj, P* po) {
    int b = arr[1] + *p + obj.x + po->y + obj.get(a) + po->get(2);
    int c = P::shared + global + free_fn(1) + free_fn(1, 2);
    int* q = &a;
    int** qq = &q;
    int d = **qq + (*q) + q[0];
    P local(1, 2);
    P braced{3, 4};
    P copy = P();
    return b + c + d + local.x + braced.y + copy.x;
}

int casts_and_sizes(int a, double dv, char* cp) {
    int b = (int)dv + static_cast<int>(dv) + int(dv) + static_cast<int>(a);
    long c = (long)a + long(a);
    const char* s = const_cast<const char*>(cp);
    void* v = reinterpret_cast<void*>(cp);
    int d = sizeof(int) + sizeof a + sizeof(a) + alignof(double);
    unsigned u = static_cast<unsigned>(c);
    char ch = 'a';
    double e = 1.5 + 2.0f + 1e3;
    bool t = true && false;
    int* np = nullptr;
    const char* str = "hello, " "world";
    return b + d + u + ch + e + t + (np == nullptr) + (s != nullptr) + (v != nullptr) + (str[0]);
}

int allocation(int n) {
    int* q = new int(5);
    int* r = new int[10];
    int* w = new int[n]{1, 2};
    P* ps = new P(1, 2);
    P* pb = new P{3, 4};
    int result = *q + r[0] + w[1] + ps->x + pb->y;
    delete q;
    delete[] r;
    delete[] w;
    delete ps;
    delete pb;
    return result;
}

int lambdas(int a) {
    auto l = [&](int z) { return z + a; };
    auto m = [=](int z) mutable { a += z; return a; };
    auto n = [a, &l]() { return a + l(1); };
    auto o = [k = a * 2](int z) -> int { return k + z; };
    auto p = [](auto x, auto y) { return x + y; };
    int r = l(2) + m(3) + n() + o(4) + p(1, 2) + twice(3) + [](int z) { return z; }(5);
    return r;
}

int increments(int a) {
    int b = a++ + ++a;
    int c = a-- - --a;
    int d = global++ + --global;
    return b + c + d;
}

void initializers() {
    int a[] = {1, 2, 3};
    int b[2][2] = {{1, 2}, {3, 4}};
    P p{1, 2};
    int c{4};
    int d(5);
    int e = {6};
    double f = double(c) + double{7};
}

int throwing(int a) {
    if (a > 100) throw a;
    if (a > 50) throw 1.5;
    return a;
}
