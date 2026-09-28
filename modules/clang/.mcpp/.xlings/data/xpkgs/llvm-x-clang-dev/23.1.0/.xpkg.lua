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
            ["23.1.0"] = {
                url    = "https://github.com/speak-agent/llvm-clang-dev/archive/refs/tags/23.1.0.tar.gz",
                sha256 = "11612b4a7785f7245ce2509ec32a63a9de602eeedfbfb49897fc2ecf82c3f946",
            },
        },
    },

    mcpp = "*/mcpp.toml",
}
