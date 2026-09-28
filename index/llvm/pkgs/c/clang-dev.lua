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
