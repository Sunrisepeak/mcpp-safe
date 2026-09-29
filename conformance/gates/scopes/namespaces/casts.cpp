namespace app {
long address(int& p) { return reinterpret_cast<long>(&p); }                // expect: reinterpret-cast -- app is strict
}
namespace app::interop {
long address(int& p) { return reinterpret_cast<long>(&p); }                // the namespace allows it
namespace detail {
long nested(int& p) { return reinterpret_cast<long>(&p); }                 // and the namespaces inside it
}
}
namespace app::interop::checked {
long address(int& p) { return reinterpret_cast<long>(&p); }                // expect: reinterpret-cast -- the inner namespace denies again
}
