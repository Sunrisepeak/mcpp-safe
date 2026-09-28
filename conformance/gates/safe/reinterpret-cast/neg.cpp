long widen(int v) { return static_cast<long>(v); }
void* erase(int* p) { return p; }
int* restore(void* p) { return static_cast<int*>(p); }
int* c_style_from_void(void* p) { return (int*)p; }
const int* add_const(int* p) { return const_cast<const int*>(p); }
double c_style_number(int v) { return (double)v; }
