module;
#include "one.h"
#include "two.h"
export module clean;
#if 0
#include "never.h"
#endif
// #include "comment.h"
export const char* text = "#include \"string.h\"";
export int value() { return one_value() + two_value(); }
