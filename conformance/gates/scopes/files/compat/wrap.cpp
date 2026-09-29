// The files that wrap a C library may include headers (files."files/compat/**"), the rest of the
// package may not (its profile, strict).
#include "shim.h"
int wrapped() { return shim_value(); }
