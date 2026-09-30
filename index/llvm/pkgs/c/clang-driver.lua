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
        },
    },

    mcpp = "*/driver/mcpp.toml",
}
