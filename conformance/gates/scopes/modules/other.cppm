export module scopes.other;
export int jump(int n) {
    if (n == 0) goto done;                               // expect: goto -- another module keeps strict's level
    return n;
done:
    return 3;
}
