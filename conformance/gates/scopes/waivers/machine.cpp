[[mcpp::allow("goto", "a state machine reads best as jumps")]]
int machine(int state) {
    if (state == 0) goto idle;                           // expect-waived: goto
    if (state == 1) goto busy;                           // expect-waived: goto
    return -1;
idle:
    return 0;
busy:
    return 1;
}
int plain(int n) {
    if (n == 0) goto end;                                // expect: goto -- no waiver here
    return n;
end:
    return 0;
}
struct [[mcpp::allow("union")]] Holder {
    union Raw { int i; float f; };                       // expect-waived: union -- the waiver covers what it declares
};
