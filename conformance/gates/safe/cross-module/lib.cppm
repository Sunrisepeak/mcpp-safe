export module shapes;
export int* make() { static int storage[4] = {}; return storage; }   // expect: c-array
export using Buf = int[16];                                           // expect: c-array
export struct Node { Node* next; };
