export module scopes.legacy:part;
export int step(int n) {
    if (n == 0) goto done;                               // a partition takes its module's levels
    return n;
done:
    return 2;
}
