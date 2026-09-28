int* one() { return new int(1); }                          // expect: new-delete
int* many() { return new int[4]; }                         // expect: new-delete
void drop(int* p) { delete p; }                            // expect: new-delete
void drop_many(int* p) { delete[] p; }                     // expect: new-delete
struct S { int v; };
S* object() { return new S { 3 }; }                        // expect: new-delete
