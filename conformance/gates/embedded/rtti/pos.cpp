namespace std { class type_info { public: virtual ~type_info(); bool operator==(const type_info&) const noexcept; }; }
struct Base { virtual ~Base() = default; };
struct Derived : Base {};
Derived* down(Base* b) { return dynamic_cast<Derived*>(b); }                    // expect: rtti
bool same(Base& a, Base& b) { return typeid(a) == typeid(b); }                  // expect: rtti
const std::type_info& of_int() { return typeid(int); }                          // expect: rtti
Derived& down_ref(Base& b) { return dynamic_cast<Derived&>(b); }                // expect: rtti
bool is_derived(Base* b) { return dynamic_cast<Derived*>(b) != nullptr; }       // expect: rtti
