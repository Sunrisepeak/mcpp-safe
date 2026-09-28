namespace std { template <class T> struct vector { }; }
int* global_pointer = nullptr;                          // expect: raw-pointers
int* find(int key);                                      // expect: raw-pointers
void fill(char* out, int n);                             // expect: raw-pointers -- the parameter
struct Node { Node* next; };                             // expect: raw-pointers
using Callback = void (*)(int);                          // expect: raw-pointers
std::vector<const char*> names;                          // expect: raw-pointers
