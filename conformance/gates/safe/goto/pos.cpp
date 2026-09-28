int a(int n) { if (n) goto done; return 1; done: return 0; }   // expect: goto
int b(int n) {
    retry:
    if (--n > 0) goto retry;                                   // expect: goto
    return n;
}
int c() { goto out; out: return 2; }                           // expect: goto
int d(int n) { while (true) { if (n-- == 0) goto end; } end: return 3; } // expect: goto
int e() { goto skip; skip: return 4; }                         // expect: goto
