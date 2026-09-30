-- llvm.clang-dev — Clang and LLVM 23.1 frontend libraries built by mcpp on openkal
-- (speak-agent/llvm-clang-dev). MC++'s backend package (mcxx.clang) is the one consumer; E-IDX-1 of
-- the milestone plan. Served by this repository's index (R2), not by mcpp-index.
package = {
    spec        = "1",
    namespace   = "llvm",
    name        = "clang-dev",
    description = "Clang and LLVM 23.1 frontend libraries (AST, Sema, Lex, Parse, Frontend, Driver, Serialization, Index, Tooling), built by mcpp on openkal",
    licenses    = {"Apache-2.0 WITH LLVM-exception"},
    repo        = "https://github.com/speak-agent/llvm-clang-dev",
    type        = "package",

    xpm = {
        linux = {
            -- 23.1.0.9: mcxx as a toolchain on Windows and macOS (MSVC's triple as Windows' default; on
            -- macOS the kernel's process identifier for the executable's path, and Darwin's release).
            ["23.1.0.9"] = {
                url    = "https://github.com/speak-agent/llvm-clang-dev/archive/refs/tags/23.1.0.9.tar.gz",
                sha256 = "0f9dde453dd5994d4e11d339c485badaf80cef7d5960feb1083dd858f28d5552",
            },
            -- 23.1.0.8: a program on openkal's macOS target knows its own path (_NSGetExecutablePath took
            -- the kernel's 0 for a failure; mcxx found neither its resource directory nor its configuration).
            ["23.1.0.8"] = {
                url    = "https://github.com/speak-agent/llvm-clang-dev/archive/refs/tags/23.1.0.8.tar.gz",
                sha256 = "253076a0b0280ef510d6221bb5d9722579cc805deb29a922f9e69dca3ae4396f",
            },
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
            -- 23.1.0.1: the same upstream 23.1.0; the repository also carries llvm.codegen-dev, and
            -- the frontend's stand-ins for code-generation facilities are weak.
            ["23.1.0.1"] = {
                url    = "https://github.com/speak-agent/llvm-clang-dev/archive/refs/tags/23.1.0.1.tar.gz",
                sha256 = "5fed04c4b8814305f9f58258d0a445daaaf577c1c0a51c52c6e9ed411e01c500",
            },
            ["23.1.0"] = {
                url    = "https://github.com/speak-agent/llvm-clang-dev/archive/refs/tags/23.1.0.tar.gz",
                sha256 = "11612b4a7785f7245ce2509ec32a63a9de602eeedfbfb49897fc2ecf82c3f946",
            },
        },
    },

    mcpp = "*/mcpp.toml",
}
