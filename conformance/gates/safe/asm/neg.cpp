int a(int v) { return v + 1; }
const char* text = "asm(\"nop\")";
// asm("nop") in a comment
struct assembly { int nop; };
int b() { return __builtin_popcount(7u); }
void c() { __atomic_thread_fence(__ATOMIC_SEQ_CST); }
