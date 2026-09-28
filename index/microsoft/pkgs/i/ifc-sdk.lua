-- microsoft.ifc-sdk — the IFC SDK (microsoft/ifc): the reader and DOM of the IFC binary format for C++
-- module interfaces, and ifc-printer. MC++ writes and reads .ifc files through it (MC2, plan E-IDX-2),
-- pinned to one release. Served by this repository's index (rule R2); upstream builds with CMake, so
-- the build is described here: CMakeLists' ifc-reader and ifc-dom (with the POSIX sha256), and the
-- printer's two units.
package = {
    spec        = "1",
    namespace   = "microsoft",
    name        = "ifc-sdk",
    description = "IFC SDK: reader and DOM for the IFC format of C++ module interfaces, and ifc-printer",
    licenses    = {"Apache-2.0 WITH LLVM-exception"},
    repo        = "https://github.com/microsoft/ifc",
    type        = "package",

    xpm = {
        linux = {
            ["0.43.5"] = {
                url    = "https://github.com/microsoft/ifc/archive/refs/tags/0.43.5.tar.gz",
                sha256 = "349ca534a6ccdf1bb9271c2e837a150bfecd1442f17d4ad8905f2682ff6e2784",
            },
        },
    },

    mcpp = {
        language     = "c++23",
        import_std   = false,
        include_dirs = { "*/include" },
        sources      = {
            "*/src/file.cxx", "*/src/sgraph.cxx", "*/src/sha256.cxx",
            "*/src/ifc-reader/operators.cxx", "*/src/ifc-reader/reader.cxx", "*/src/ifc-reader/util.cxx",
            "*/src/ifc-dom/charts.cxx", "*/src/ifc-dom/decls.cxx", "*/src/ifc-dom/exprs.cxx",
            "*/src/ifc-dom/literals.cxx", "*/src/ifc-dom/names.cxx", "*/src/ifc-dom/sentences.cxx",
            "*/src/ifc-dom/stmts.cxx", "*/src/ifc-dom/syntax.cxx", "*/src/ifc-dom/types.cxx",
            "*/src/ifc-printer/printer.cxx", "*/src/assert.cxx",
        },
        targets      = {
            ["ifc-sdk"]     = { kind = "lib" },
            ["ifc-printer"] = { kind = "bin", main = "ifc-0.43.5/src/ifc-printer/main.cxx" },
        },
        -- The C++ runtime (libc++ on openkal) is a dependency so that ifc-printer links on its own.
        deps         = { ["microsoft.gsl"] = "4.2.0", ["openkal-llvm-runtime"] = "0.15.2" },
    },
}
