struct Error { int code; };
struct Result { int value; Error error; };
Result compute(int v) { return { v, { 0 } }; }
int quiet() noexcept { return 0; }
int thrown = 0;
const char* text = "throw try catch";
static_assert(noexcept(quiet()));
