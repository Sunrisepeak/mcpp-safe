constexpr int limit = 4;
constexpr int square(int x) { return x * x; }
inline int use() { return limit + square(2); }
#if 0
#define NEVER 1
#endif
// #define IN_A_COMMENT 1
const char* text = "#define IN_A_STRING 1";
