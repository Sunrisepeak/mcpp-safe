-- mcxx.mcpp-tools-safe — MC++'s feature gates as an mcpp build rule (E-PLG-1, E-IDX-5): a program built
-- with GCC, or with an LLVM that is not mcxx, runs `mcxx check` over its sources as a `role = "check"`
-- action, and the build fails on a gate's error. A host module: a consumer names it in
-- [build-dependencies] with `host-module = true`. Served by this repository's index (rule R2); the
-- package is plugins/mcpp-tools-safe of a tagged release of this repository.
package = {
    spec        = "1",
    namespace   = "mcxx",
    name        = "mcpp-tools-safe",
    description = "MC++'s feature gates (mcxx check) as an mcpp check rule, for builds with other compilers",
    licenses    = {"Apache-2.0"},
    repo        = "https://github.com/Sunrisepeak/mcpp-safe",
    type        = "package",

    xpm = {
        linux = {
            ["0.1.0"] = {
                url    = "https://github.com/Sunrisepeak/mcpp-safe/archive/refs/tags/0.1.0.tar.gz",
                sha256 = "87c6fd0eb8b15fd6efa9caa30cdd13b35866683e1a941bfb44c9951ee70f1cb2",
            },
        },
    },

    mcpp = "*/plugins/mcpp-tools-safe/mcpp.toml",
}
