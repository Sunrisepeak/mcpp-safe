#define LIMIT 4                                          // strict denies macros; the package allows them
int risky(int n) {
    if (n > LIMIT) throw n;                              // expect: exceptions -- denied by the package, in no profile
    if (n < 0) goto out;                                 // expect: goto -- strict's, not the package's to change here
    return n;
out:
    return 0;
}
