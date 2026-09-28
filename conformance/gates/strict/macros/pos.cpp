#define LIMIT 4                                            // expect: macros
#define SQUARE(x) ((x) * (x))                              // expect: macros
#define EMPTY                                              // expect: macros
#define NAME "n"                                           // expect: macros
#undef LIMIT
#define LIMIT 8                                            // expect: macros
int use() { return LIMIT + SQUARE(2); }
