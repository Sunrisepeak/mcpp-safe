-- mcxx.mcxx-plugins-std — MC++'s plugins in plugins/std of this repository (E-IDX-5), for a package that
-- composes its own compiler with them (`mcxx compose`) or links them into a host (mcppls). They depend
-- on libmc++'s SDK by path inside the same archive. Served by this repository's index (rule R2).
package = {
    spec        = "1",
    namespace   = "mcxx",
    name        = "mcxx-plugins-std",
    description = "MC++'s std plugins, as static providers for a host or a composed mcxx",
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

    mcpp = "*/plugins/std/mcpp.toml",
}
