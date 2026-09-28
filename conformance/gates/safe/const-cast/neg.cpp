struct Counter { mutable int hits = 0; void touch() const { ++hits; } };
const int* view(int* p) { return p; }
int copy(const int* p) { return *p; }
int* same(int* p) { return static_cast<int*>(p); }
const int& ref(const int& r) { return r; }
int read(const Counter& c) { c.touch(); return c.hits; }
