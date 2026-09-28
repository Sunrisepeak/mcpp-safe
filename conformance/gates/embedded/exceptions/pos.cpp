struct Error { int code; };
void fail() {
    throw Error { 1 };                                          // expect: exceptions
}
int guarded() {
    try {                                                       // expect: exceptions
        fail();
    } catch (...) {
        return 1;
    }
    return 0;
}
void rethrow() {
    try {                                                       // expect: exceptions
        fail();
    } catch (const Error&) {
        throw;                                                  // expect: exceptions
    }
}
int function_try() try { return 0; } catch (...) { return 1; }   // expect: exceptions
