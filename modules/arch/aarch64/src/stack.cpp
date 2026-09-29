// mcxx_call_on_stack for aarch64 (mcxx.arch STACK_SWITCH): calls fn(arg) on the stack whose top is
// `top`, then returns to the caller's stack. The one piece of assembly libmc++ has, kept in the
// architecture's package so no other code needs a preprocessor test for the platform (plan P9).
//
// ELF only (Linux; openkal): there, every thread's stack is a fixed 256 KiB, too small for Clang,
// and libmc++ runs Clang on stacks of its own (modules/backend/clang :support). Elsewhere a thread
// takes the stack size it asks for, and the call is a plain call.
extern "C" void mcxx_call_on_stack(void* top, void (*fn)(void*), void* arg);

#if defined(__ELF__)
asm(R"(
    .text
    .globl mcxx_call_on_stack
    .type mcxx_call_on_stack,%function
mcxx_call_on_stack:
    stp x29, x30, [sp, #-16]!
    mov x29, sp
    mov sp, x0
    mov x0, x2
    blr x1
    mov sp, x29
    ldp x29, x30, [sp], #16
    ret
    .size mcxx_call_on_stack, .-mcxx_call_on_stack
)");
#else
extern "C" void mcxx_call_on_stack(void*, void (*fn)(void*), void* arg) { fn(arg); }
#endif
