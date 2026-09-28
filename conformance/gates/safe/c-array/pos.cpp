int table[4] = { 1, 2, 3, 4 };                             // expect: c-array
struct Packet { char bytes[16]; };                         // expect: c-array
int sum(int values[8]) { return values[0]; }               // expect: c-array raw-pointer-arithmetic -- the parameter is an int*
int local() { int buffer[2] = { 0, 1 }; return buffer[1]; } // expect: c-array
const char* const names[] = { "a", "b" };                  // expect: c-array
