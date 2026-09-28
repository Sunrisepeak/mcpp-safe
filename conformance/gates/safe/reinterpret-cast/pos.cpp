long address(int* p) { return reinterpret_cast<long>(p); }            // expect: reinterpret-cast
int* back(long v) { return reinterpret_cast<int*>(v); }               // expect: reinterpret-cast
float* pun(int* p) { return reinterpret_cast<float*>(p); }            // expect: reinterpret-cast
char* bytes(double* d) { return (char*)d; }                           // expect: reinterpret-cast c-style-cast
long c_style_address(int* p) { return (long)p; }                      // expect: reinterpret-cast c-style-cast
