export module scopes.legacy;
export int walk(int n) {
    if (n == 0) goto done;                               // the module allows goto
    return n;
done:
    return 1;
}
export union Bits { int i; float f; };                   // expect: union -- the module changes goto only
