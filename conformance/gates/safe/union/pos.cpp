union Bits { int i; float f; };                            // expect: union
struct Tagged { int tag; union { int i; float f; } u; };   // expect: union
union Word { unsigned w; unsigned char b[4]; };            // expect: union c-array
template <class T> union Storage { T value; char raw; };   // expect: union
namespace n { union Inner { long l; double d; }; }         // expect: union
