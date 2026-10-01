// langdiff: c++2c
// P2843R3 where Clang 23.1 reads the program as the paper does: a new-line in a stringized raw string literal is
// \n (CWG1709), a string that would end in a lone backslash loses it, defining what the paper keeps from #define
// is read as it always was. Token for token (tools/checks/langdiff.py).
#define S(x) #x
const char* raw = S(R"(a
b)");
const char* raw_two = S(R"x(first
second
third)x" u8R"(utf8
text)");
#define TO_TEXT(a) #a
#define TEXT(a) TO_TEXT(a)
#define BACKSLASH \\

const char* lone = TEXT(BACKSLASH);
const char* pair = S(\\);
const char* quoted = S("\\");
const char* escaped = S('\n' "a\"b");
#define DO_CONCAT(a, b) a##b
#define CONCAT(a, b) DO_CONCAT(a, b)
int concatenated = CONCAT(1, 2) + CONCAT(x, 3);
#define public private
#undef public
#define unlikely(x) x
#define likely(x) x
int after = unlikely(1) + likely(2);
#define __cpp_lib_user 1
