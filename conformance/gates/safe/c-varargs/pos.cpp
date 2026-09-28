int trace(const char* format, ...);                                    // expect: c-varargs
void log(int level, ...);                                              // expect: c-varargs
struct Printer { int print(const char* f, ...); };                     // expect: c-varargs
int sum(int n, ...) {                                                  // expect: c-varargs
    __builtin_va_list args;                                            // expect: c-array uninitialized -- va_list: an array on x86-64, set by va_start
    __builtin_va_start(args, n);
    int total = __builtin_va_arg(args, int);                           // expect: c-varargs
    __builtin_va_end(args);
    return total;
}
