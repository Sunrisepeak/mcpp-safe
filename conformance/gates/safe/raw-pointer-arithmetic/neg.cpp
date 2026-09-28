struct Iter {
    int* at;
    Iter operator+(int n) const { return { at }; }
    int operator*() const { return 0; }
};
int class_iterator(Iter i) { return *(i + 1); }
int integers(int a, int b) { return a + b - 1; }
bool compare(int* p, int* q) { return p == q; }
int deref(int* p) { return *p; }
int by_ref(int& r) { return r + 1; }
