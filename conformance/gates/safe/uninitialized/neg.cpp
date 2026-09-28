struct Point { int x; int y; };
struct Named { int id = 0; };
int global_count;
int a() { int x = 1; return x; }
int b() { int z {}; return z; }
int c() { Point p {}; return p.x; }
int d() { static int calls; return ++calls; }
int e() { Named n; return n.id; }
int f() { struct Empty {}; Empty e; return sizeof(e); }
