#include "one.h"      // expect: include
#include "two.h"      // expect: include
#include "three.h"    // expect: include
#if 1
#include "four.h"     // expect: include
#endif
int total() { return one_value() + two_value() + three_value() + four_value(); }
