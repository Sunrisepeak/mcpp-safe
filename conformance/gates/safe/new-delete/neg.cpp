struct S { int v; };
S value() { return S { 1 }; }
int local() { int x = 2; return x; }
void* operator_new_named(unsigned long n);   // a declaration named like it, not a new-expression
int arrays() { int a[2] = { 1, 2 }; return a[0]; }          // expect: c-array
struct Owner { S s; };
