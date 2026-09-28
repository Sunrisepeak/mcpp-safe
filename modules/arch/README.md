# arch

Two packages, `x86_64` and `aarch64`, each exporting the same module, `mcxx.arch`: `Arch` and
`ARCH`. Only one is ever in a build's dependency graph — the os packages pick it with
`[target.'cfg(arch = "...")'.dependencies]` — so, like `mcxx.os`, it is a compile-time constant
read with `if constexpr`, never a macro. Its one consumer is `mcxx.os`'s `PLATFORM`, the name a
payload, a VSIX and the lock use for "this operating system on this architecture".
