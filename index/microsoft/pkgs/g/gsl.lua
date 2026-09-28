-- microsoft.gsl — the C++ Core Guidelines Support Library (header-only), which the IFC SDK uses
-- for gsl::span and friends. Served by this repository's index (rule R2); upstream has no manifest.
package = {
    spec        = "1",
    namespace   = "microsoft",
    name        = "gsl",
    description = "Guidelines Support Library: header-only C++ types from the C++ Core Guidelines",
    licenses    = {"MIT"},
    repo        = "https://github.com/microsoft/GSL",
    type        = "package",

    xpm = {
        linux = {
            ["4.2.0"] = {
                url    = "https://github.com/microsoft/GSL/archive/refs/tags/v4.2.0.tar.gz",
                sha256 = "2c717545a073649126cb99ebd493fa2ae23120077968795d2c69cbab821e4ac6",
            },
        },
    },

    mcpp = {
        language     = "c++23",
        import_std   = false,
        include_dirs = { "*/include" },
        -- Header-only: a trivial anchor unit gives mcpp a lib target to build.
        generated_files = {
            ["mcpp_generated/gsl_anchor.cpp"] = [==[
int mcxx_microsoft_gsl_anchor(void) { return 0; }
]==],
        },
        sources      = { "mcpp_generated/gsl_anchor.cpp" },
        targets      = { ["gsl"] = { kind = "lib" } },
        deps         = { },
    },
}
