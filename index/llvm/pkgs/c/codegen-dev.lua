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
