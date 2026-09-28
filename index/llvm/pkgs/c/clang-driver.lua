-- llvm.clang-driver — clang itself in process: the driver (clang_main), cc1 and cc1as, and the
-- FrontendTool that dispatches cc1's actions (speak-agent/llvm-clang-dev, driver/). The mcxx driver
-- provides main(), registers the x86-64 and AArch64 back ends and calls clang_main.
package = {
    spec        = "1",
    namespace   = "llvm",
    name        = "clang-driver",
    description = "The clang 23.1 driver, cc1 and cc1as in process (clang_main), built by mcpp on openkal",
    licenses    = {"Apache-2.0 WITH LLVM-exception"},
    repo        = "https://github.com/speak-agent/llvm-clang-dev",
    type        = "package",

    xpm = {
        linux = {
            ["23.1.0.2"] = {
                url    = "https://github.com/speak-agent/llvm-clang-dev/archive/refs/tags/23.1.0.2.tar.gz",
                sha256 = "40c364282f5f3d3f65130b8eb000c8c480c9eb467870bc95e91b6d418a6b7f88",
            },
        },
    },

    mcpp = "*/driver/mcpp.toml",
}
