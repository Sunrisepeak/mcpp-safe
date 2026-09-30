-- llvm.codegen-dev — Clang 23.1 CodeGen and the LLVM optimizer, code generator and object writers for
-- x86-64 and AArch64, built by mcpp on openkal (speak-agent/llvm-clang-dev, codegen/). The mcxx driver
-- is its consumer (V0.4); a program that only reads C++ depends on llvm.clang-dev alone.
package = {
    spec        = "1",
    namespace   = "llvm",
    name        = "codegen-dev",
    description = "Clang 23.1 CodeGen and the LLVM optimizer, code generator and object writers for x86-64 and AArch64, built by mcpp on openkal",
    licenses    = {"Apache-2.0 WITH LLVM-exception"},
    repo        = "https://github.com/speak-agent/llvm-clang-dev",
    type        = "package",

    xpm = {
        linux = {
            -- 23.1.0.7: the code generator builds for openkal's macOS target (INT64_C and UINT64_C in the
            -- typedefs' type: MachineIRBuilder's SrcOp(INT64_C(0)) was ambiguous there; E-XIM-3).
            ["23.1.0.7"] = {
                url    = "https://github.com/speak-agent/llvm-clang-dev/archive/refs/tags/23.1.0.7.tar.gz",
                sha256 = "f9d416199560e4830b0df6e8c76fb848a39cf8fdae9ffd71de7b464fa998906f",
            },
            -- 23.1.0.6: upstream's fix for std::align_val_t declared in extern "C++" in a named module
            -- (llvm/llvm-project#219151; MSVC's std module was ambiguous).
            ["23.1.0.6"] = {
                url    = "https://github.com/speak-agent/llvm-clang-dev/archive/refs/tags/23.1.0.6.tar.gz",
                sha256 = "fd82f8b5765972462190938333015628a2b06451efb2235971d943da906362e8",
            },
            -- 23.1.0.5: the resource directory's generated intrinsics headers (llvm-generated/clang-lib/
            -- Headers: arm_neon.h and the other ARM, AArch64 and RISC-V ones).
            ["23.1.0.5"] = {
                url    = "https://github.com/speak-agent/llvm-clang-dev/archive/refs/tags/23.1.0.5.tar.gz",
                sha256 = "441cad8ea9b9825dc17601bb19d7c1877d4e38eaec4ffa4d19ecf55683b08df3",
            },
            -- 23.1.0.4: also openkal's Windows and macOS targets, cross-built from Linux (a path on
            -- Windows has a drive, read by Windows' rules).
            ["23.1.0.4"] = {
                url    = "https://github.com/speak-agent/llvm-clang-dev/archive/refs/tags/23.1.0.4.tar.gz",
                sha256 = "88ddd72f1b557be60b1f22a28a76e912ec327f6b2f7806bbfd2b2a2757202b74",
            },
            -- 23.1.0.3: the default target is x86_64-unknown-linux-gnu (the host stays openkal's musl), so
            -- the compiler serves as a glibc Linux's llvm toolchain (V0.6).
            ["23.1.0.3"] = {
                url    = "https://github.com/speak-agent/llvm-clang-dev/archive/refs/tags/23.1.0.3.tar.gz",
                sha256 = "42b661fcb837be116f4263c57ec5052774c3560f9099d7822bbe758011032d68",
            },
            ["23.1.0.2"] = {
                url    = "https://github.com/speak-agent/llvm-clang-dev/archive/refs/tags/23.1.0.2.tar.gz",
                sha256 = "40c364282f5f3d3f65130b8eb000c8c480c9eb467870bc95e91b6d418a6b7f88",
            },
            ["23.1.0.1"] = {
                url    = "https://github.com/speak-agent/llvm-clang-dev/archive/refs/tags/23.1.0.1.tar.gz",
                sha256 = "5fed04c4b8814305f9f58258d0a445daaaf577c1c0a51c52c6e9ed411e01c500",
            },
        },
    },

    mcpp = "*/codegen/mcpp.toml",
}
