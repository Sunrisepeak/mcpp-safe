struct Base { virtual ~Base() = default; virtual int kind() const { return 0; } };
struct Derived : Base { int kind() const override { return 1; } };
Base* up(Derived* d) { return static_cast<Base*>(d); }
int kind_of(const Base& b) { return b.kind(); }
Derived* known(Base* b) { return static_cast<Derived*>(b); }
unsigned long size = sizeof(Derived);
const char* text = "typeid dynamic_cast";
