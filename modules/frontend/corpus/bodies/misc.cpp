// What the other fixtures leave out: overloaded operators, pointers to members, polymorphism, designated
// initializers, attributes, structured bindings in conditions, nested lambdas, GNU forms. Self-contained
// C++23 that Clang reads without errors (tools/checks/bodiesdiff.py).
namespace std {
template <class T> struct initializer_list {
    const T* first;
    unsigned long count;
};
}
struct Vec {
    int x, y;
    Vec operator+(const Vec& o) const { return Vec{x + o.x, y + o.y}; }
    Vec operator-() const { return Vec{-x, -y}; }
    Vec& operator+=(const Vec& o) { x += o.x; y += o.y; return *this; }
    bool operator==(const Vec& o) const { return x == o.x && y == o.y; }
    int operator[](int i) const { return i == 0 ? x : y; }
    int operator()(int k) const { return x * k; }
    Vec& operator++() { ++x; return *this; }
    Vec operator++(int) { Vec old = *this; ++x; return old; }
    Vec* operator->() { return this; }
    explicit operator bool() const { return x != 0; }
};
struct Base { virtual ~Base() {} virtual int id() const { return 1; } };
struct Derived : Base { int id() const override { return 2; } int extra = 3; };
struct Config { int width; int height; bool wide; };
struct Pair { int a; int b; };
Pair make_pair(int a, int b) { return Pair{a, b}; }

int operators(Vec u, Vec v) {
    Vec w = u + v;
    w += -u;
    ++w; w++;
    bool same = u == v;
    int k = w[0] + w[1] + w(2) + w->x + (bool)w;
    return k + same;
}

int member_function_pointers(Base* b, int (Base::* pf)() const, Base& r) {
    return (b->*pf)() + (r.*pf)();
}

int polymorphism(Base* b, Base& r) {
    Derived* d = dynamic_cast<Derived*>(b);
    Derived& dr = dynamic_cast<Derived&>(r);
    int n = d ? d->extra : dr.extra;
    return n + b->id() + r.id();
}

int pointers_to_members(Vec v, Vec* pv) {
    int Vec::* pm = &Vec::x;
    int a = v.*pm + pv->*pm;
    return a;
}

int designated() {
    Config c{.width = 3, .height = 4, .wide = false};
    Config d = {.width = 1};
    return c.width + d.height;
}

int attributes_and_labels(int n) {
    [[maybe_unused]] int unused = 0;
    if (n > 0) [[likely]] { n = 1; } else [[unlikely]] { n = 2; }
    switch (n) {
    case 1: [[fallthrough]];
    case 2: n = 3; break;
    }
    alignas(8) char buffer[16];
    static_assert(sizeof(int) >= 4);
    return n + buffer[0];
}

int bindings(Pair p, int (&arr)[2]) {
    auto [a, b] = p;
    auto& [c, d] = p;
    const auto [e, f] = arr;
    if (auto [g, h] = make_pair(1, 2); g < h) return g;
    return a + b + c + d + e + f;
}

int nested_lambdas(int n) {
    auto outer = [n](int a) {
        auto inner = [a, n](int b) { return a + b + n; };
        return inner(a) + [a] { return a; }();
    };
    return outer(1) + outer(2);
}

int strings_and_chars() {
    const char* a = "plain";
    const char8_t* b = u8"utf8";
    const wchar_t* c = L"wide";
    const char* d = R"(raw "string")";
    char e = '\n', f = '\'', g = L'x' ? 'y' : 'z';
    int h = 0x7fff + 0b101 + 077 + 1'000;
    unsigned long long i = 123ull;
    double j = 1e-3 + .5 + 5. + 0x1p3;
    return a[0] + b[0] + c[0] + d[0] + e + f + g + h + (int)i + (int)j;
}

int gnu_and_misc(int n, int* p) {
    int a = __builtin_expect(n, 1) + __builtin_popcount(n);
    int b = sizeof(int[3]) + sizeof(n) / sizeof(*p);
    int c = ({ int t = n; t * 2; });
    int d = n ? : 7;
    void* here = &&target;
target:
    asm volatile("" ::: "memory");
    goto *here;
    return a + b + c + d;
}

int conversions(double x, long y) {
    int a = (int)x + (int)y + int(x);
    float f = (float)x + float(y);
    char c = (char)(a + 1);
    unsigned u = (unsigned)-1;
    bool b = (bool)a || !!c;
    return a + (int)f + c + u + b;
}

int ternaries_and_commas(int a, int b, int c) {
    int d = a ? b : c ? a : b;
    int e = (a, b), f = (a ? (b, c) : (c, a));
    a = b = c;
    a += b -= c;
    int g = a < b ? a > c ? 1 : 2 : 3;
    return d + e + f + g;
}
