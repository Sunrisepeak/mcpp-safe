int* unconst(const int* p) { return const_cast<int*>(p); }             // expect: const-cast
int& unconst_ref(const int& r) { return const_cast<int&>(r); }         // expect: const-cast
void write(const int* p) { *const_cast<int*>(p) = 1; }                 // expect: const-cast
const int* add(int* p) { return const_cast<const int*>(p); }           // expect: const-cast -- adding const is one too
volatile int* vol(int* p) { return const_cast<volatile int*>(p); }     // expect: const-cast
