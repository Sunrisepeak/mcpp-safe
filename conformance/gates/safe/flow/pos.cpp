[[noreturn]] void halt();
int pick(bool c) {
    int x;                                   // expect: uninitialized
    if (c) x = 1;
    return x;                                // expect: uninitialized-read -- c false: nothing wrote x
}
int total(int n) {
    int sum;                                 // expect: uninitialized
    for (int i = 0; i < n; ++i) sum += i;    // expect: uninitialized-read -- the first += reads it
    return sum;                              // expect: uninitialized-read -- n <= 0: the loop never ran
}
int sign(int v) {
    if (v > 0) return 1;
    if (v < 0) return -1;
}                                            // expect: missing-return -- v == 0
[[noreturn]] void stop(bool c) {
    if (c) halt();
}                                            // expect: noreturn-returns -- c false
[[noreturn]] void quit() {
    return;                                  // expect: noreturn-returns
}
