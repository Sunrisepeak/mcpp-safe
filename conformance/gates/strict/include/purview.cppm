module;
#include "gmf.h"
export module purview;
#include "five.h"     // expect: include -- in the module's purview: its declarations become the module's
export int value() { return gmf_value() + five_value(); }
