// Templates and what depends on their parameters, on builtin types only: self-contained C++23 that Clang
// reads without errors; its AST is the reference for kinds and ranges (tools/checks/bodiesdiff.py).
template <class T> struct Box {
    T value;
    T get() const { return value; }
    template <class U> U convert() const { return static_cast<U>(value); }
    static constexpr int size = sizeof(T);
    Box() : value() {}
    explicit Box(T v) : value(v) {}
};

template <class... Ts> int count(Ts... ts) { return sizeof...(Ts); }
template <class... Ts> int sum(Ts... ts) { return (ts + ... + 0); }
template <class... Ts> int all(Ts... ts) { return (... && ts); }
template <class T, int N> int fill(T (&arr)[N], T v) {
    for (int i = 0; i < N; ++i) arr[i] = v;
    return N;
}
template <class T> T maxof(T a, T b) { return a > b ? a : b; }
template <class T> auto describe(T v) {
    if constexpr (sizeof(T) > 4) {
        return v + 1;
    } else {
        return v - 1;
    }
}
template <class F> int call_twice(F f) { return f(1) + f(2); }
template <class T> struct Holder {
    T v;
    int twice() { return v.get() + v.get(); }
};
template <class T> int use_member(T t) {
    int a = t.value + t.get() + T::size + t.template convert<int>();
    T copy(t);
    T made = T(1);
    T* p = new T(2);
    delete p;
    return a;
}
template <class T> T identity(T x) { return x; }
template <class T> int sizes() { return sizeof(T) + alignof(T) + sizeof(T*); }
template <class T> bool check(T a) {
    bool b = noexcept(a + a);
    int k = (int)a;
    long m = static_cast<long>(a);
    auto l = [a](T z) { return a + z; };
    return b && l(a) > a && (k + m) > 0;
}
template <class T, class U> auto add(T a, U b) -> decltype(a + b) { return a + b; }
template <class T> void each(T (&arr)[3]) {
    for (T v : arr) { T w = v; (void)w; }
    for (auto& v : arr) { v = identity(v); }
}

int instantiate() {
    Box<int> b(3);
    int total = b.get() + b.convert<int>() + Box<int>::size;
    total += count(1, 2.0, 'c') + sum(1, 2, 3) + all(true, false);
    int arr[3] = {1, 2, 3};
    total += fill(arr, 4) + maxof(1, 2) + describe(3) + describe(3.5);
    total += call_twice([](int x) { return x * 2; });
    Holder<Box<int>> h{b};
    total += h.twice() + use_member(b) + identity(5) + sizes<int>() + check(2) + add(1, 2.5);
    each(arr);
    return total;
}
