// The architecture of one target. Exactly one of the two arch packages is in the dependency graph
// of a build, chosen by the target's `cfg(arch = ...)`, so code branches on it with `if constexpr`
// instead of the preprocessor -- the same arrangement as the os packages, one axis over.
export module mcxx.arch;

import std;

export namespace mcxx::arch {

enum class Arch { x86_64, aarch64 };

inline constexpr Arch ARCH { Arch::x86_64 };

// Whether mcxx_call_on_stack runs its function on the given stack (src/stack.cpp): on ELF targets.
#if defined(__ELF__)
inline constexpr bool STACK_SWITCH { true };
#else
inline constexpr bool STACK_SWITCH { false };
#endif

} // namespace mcxx::arch

// Calls fn(arg) on the stack whose top is `top` (where STACK_SWITCH says so; a plain call elsewhere).
export extern "C" void mcxx_call_on_stack(void* top, void (*fn)(void*), void* arg);
