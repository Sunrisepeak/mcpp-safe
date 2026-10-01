struct Point { int x; int y; };
int a() { int x; x = 1; return x; }                     // expect: uninitialized
int b() { double d; return static_cast<int>(d); }       // expect: uninitialized uninitialized-read
int c() { Point p; p.x = 0; return p.x; }               // expect: uninitialized
int d() { int* p; return p != nullptr; }                // expect: uninitialized uninitialized-read
int e() { bool flag; return flag; }                     // expect: uninitialized uninitialized-read
