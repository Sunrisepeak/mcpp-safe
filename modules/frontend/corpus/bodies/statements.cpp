// Statements on builtin types only: self-contained C++23 that Clang reads without errors, the reference for
// statement kinds and ranges (tools/checks/bodiesdiff.py).
namespace std {
template <class T> struct initializer_list {   // what a braced list in a range-based for needs, without a header
    const T* first;
    unsigned long count;
    const T* begin() const { return first; }
    const T* end() const { return first + count; }
};
}
struct Pair {
    int first;
    int second;
};
int sink(int v) { return v; }

int branches(int a, int b) {
    if (a > b) sink(1);
    if (a > b) { sink(2); } else { sink(3); }
    if (a > 0) sink(4); else if (b > 0) sink(5); else sink(6);
    if (int c = a + b) sink(c);
    if (int d = a; d > b) sink(d);
    if constexpr (sizeof(int) == 4) sink(7);
    switch (a) {
    case 1: sink(8); break;
    case 2:
    case 3: sink(9); break;
    default: sink(10);
    }
    switch (int s = b; s) { case 1: break; default: break; }
    return a;
}

int loops(int n, int (&arr)[4]) {
    int total = 0;
    for (int i = 0; i < n; ++i) total += i;
    for (int i = 0, j = n; i < j; ++i, --j) { total += i * j; }
    for (;;) break;
    for (int v : arr) total += v;
    for (auto& v : arr) { total -= v; }
    for (int v : {1, 2, 3}) total += v;
    int k = 0;
    while (k < n) ++k;
    while (k > 0) { --k; if (k == 3) continue; if (k == 1) break; }
    do { ++k; } while (k < 10);
    do ++k; while (k < 20);
    return total;
}

int jumps(int a) {
    int r = 0;
again:
    r += a;
    if (r < 10) goto again;
    goto out;
out:
    return r;
}

int exceptions(int a) {
    try {
        if (a > 1) throw a;
        sink(1);
    } catch (int e) {
        sink(e);
    } catch (const char*) {
        sink(2);
    } catch (...) {
        sink(3);
    }
    try { sink(4); } catch (...) { }
    return a;
}

int declarations(int a) {
    int x = 1, y = 2;
    const int z = 3;
    static int counter = 0;
    int arr[3] = {1, 2, 3};
    int* p = arr;
    int& r = x;
    auto q = x + y;
    auto [f, s] = Pair{1, 2};
    typedef int Int;
    using Long = long;
    struct Local { int m; int get() const { return m; } };
    enum Color { red, green = 3 };
    Local l{1};
    Int ii = 4;
    Long ll = 5;
    return x + y + z + counter + arr[0] + *p + r + q + f + s + l.get() + ii + ll + green;
}

void blocks() {
    ;
    {
        int inner = 1;
        sink(inner);
    }
    { }
}

int nested(int a) {
    for (int i = 0; i < a; ++i) {
        for (int j = 0; j < i; ++j) {
            if (i == j) continue;
            else if (j > 5) break;
            switch (j) { case 0: sink(i); default: break; }
        }
    }
    return a;
}
