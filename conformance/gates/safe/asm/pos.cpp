void a() { asm("nop"); }                          // expect: asm
void b() { asm volatile("pause"); }              // expect: asm
void c() { __asm__("nop"); }                     // expect: asm
int d(int v) { asm("" : "+r"(v)); return v; }    // expect: asm
asm(".globl mcxx_marker");                       // expect: asm
