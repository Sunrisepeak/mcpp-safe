// A more specific pattern (files."files/compat/raw_*.cpp") denies it again.
#include "shim.h"      // expect: include
int raw() { return shim_value(); }
