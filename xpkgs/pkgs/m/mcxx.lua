-- mcxx -- MC++'s compiler as an llvm-family toolchain payload (E-XIM-1, V0.6, MC5 section 7).
--
-- LOCAL: this repository's own xpkg (rule R2 of the milestone plan: nothing is filed with the
-- official indexes during the early stage). Registered with mcpp's xlings, then installed:
--
--   XLINGS_HOME=~/.mcpp/registry ~/.mcpp/registry/bin/xlings config --add-xpkg xpkgs/pkgs/m/mcxx.lua
--   MCXX_BINARY=<a built mcxx> MCXX_CLANG_HEADERS=<llvm.clang-dev's llvm/clang/lib/Headers> \
--       XLINGS_HOME=~/.mcpp/registry ~/.mcpp/registry/bin/xlings install mcxx:mcxx@0.1.0 -y
--   mcpp build --toolchain llvm@23.1.0-mcxx          # in any mcpp project; mcpp is not changed
--
-- The payload has xim:llvm's layout, so mcpp takes it as the llvm family: bin/clang++ and bin/clang
-- are mcxx (the driver selects its mode by the name it is started by), lib/clang/23/include holds
-- Clang 23.1's builtin headers, and everything else -- libc++ with its std module sources,
-- compiler-rt, lld, binutils, clang-scan-deps, the glibc and kernel-header configuration -- is the
-- xim:llvm 22.1.8 payload this package depends on. Installing also registers the payload where
-- mcpp looks up an llvm toolchain, as xim-x-llvm/23.1.0-mcxx (removed again on uninstall).
--
-- There is no download yet: the mcxx program comes from a build of this repository
-- (tools/payload/payload.py assembles the same layout without xlings). A release asset replaces
-- MCXX_BINARY once MC++ publishes one.
package = {
    spec = "2",

    name = "mcxx",
    namespace = "mcxx",
    description = "MC++'s compiler (clang 23.1 in process, with MC++'s feature gates and plugins) as an llvm-family toolchain",

    authors = {"speak-agent"},
    maintainers = {"speak-agent"},
    licenses = {"Apache-2.0 WITH LLVM-exception"},
    repo = "https://github.com/Sunrisepeak/mcpp-safe",

    type = "package",
    status = "dev",
    archs = {"x86_64"},
    categories = {"compiler", "toolchain", "llvm", "c++", "modules"},
    keywords = {"mcxx", "mc++", "clang", "modules", "feature-gates"},

    programs = {"mcxx"},

    xpm = {
        linux = {
            deps = {"xim:llvm@22.1.8"},
            ["latest"] = { ref = "0.1.0" },
            ["0.1.0"] = {},
        },
    },
}

import("xim.libxpkg.pkginfo")
import("xim.libxpkg.log")
import("xim.libxpkg.xvm")

local CLANG_MAJOR = "23"
local TOOLCHAIN_VERSION = "23.1.0-mcxx"
local TARGET = "x86_64-unknown-linux-gnu"

-- A shell command; false when it fails (os.exec raises).
local function sh(cmd)
    local ok = try { function() os.exec(cmd); return true end }
    return ok == true
end

local function q(p) return "'" .. p:gsub("'", "'\\''") .. "'" end

-- Where mcpp looks an llvm toolchain up: <store>/xim-x-llvm/<version>, beside this package's own
-- <store>/<namespace>-x-mcxx/<version>.
local function llvm_registration(install_dir)
    local store = path.directory(path.directory(install_dir))
    return path.join(store, "xim-x-llvm", TOOLCHAIN_VERSION)
end

function install()
    local dir = pkginfo.install_dir()
    local mcxx = os.getenv("MCXX_BINARY")
    local headers = os.getenv("MCXX_CLANG_HEADERS")
    if not mcxx or not os.isfile(mcxx) then
        raise("mcxx: set MCXX_BINARY to a built mcxx (mcpp build in Sunrisepeak/mcpp-safe)")
    end
    if not headers or not os.isfile(path.join(headers, "stddef.h")) then
        raise("mcxx: set MCXX_CLANG_HEADERS to llvm.clang-dev's llvm/clang/lib/Headers")
    end
    local llvm = pkginfo.dep_install_dir("xim:llvm")
    if not llvm or not os.isfile(path.join(llvm, "bin", "clang++")) then
        raise("mcxx: the xim:llvm 22.1.8 payload this package depends on is not installed")
    end

    os.tryrm(dir)
    os.mkdir(path.join(dir, "bin"))
    os.mkdir(path.join(dir, "lib", "clang", CLANG_MAJOR))

    -- Everything but the compilers and the resource directory is the llvm payload's, by link.
    local script = table.concat({
        "set -e",
        "L=" .. q(llvm), "D=" .. q(dir),
        "for top in \"$L\"/*; do n=$(basename \"$top\"); case $n in bin|lib) ;; *) ln -s \"$top\" \"$D/$n\";; esac; done",
        "for t in \"$L\"/bin/*; do n=$(basename \"$t\"); case $n in clang-scan-deps*) ln -s \"$t\" \"$D/bin/$n\";; clang*|*.cfg) ;; *) ln -s \"$t\" \"$D/bin/$n\";; esac; done",
        "for i in \"$L\"/lib/*; do n=$(basename \"$i\"); [ \"$n\" = clang ] || ln -s \"$i\" \"$D/lib/$n\"; done",
        "R=$(ls -d \"$L\"/lib/clang/* | head -1)",
        "cp -R " .. q(headers) .. " \"$D/lib/clang/" .. CLANG_MAJOR .. "/include\"",
        "[ -d \"$R/lib\" ] && ln -s \"$R/lib\" \"$D/lib/clang/" .. CLANG_MAJOR .. "/lib\"",
        -- mcxx under the names its driver selects the mode by; a hard link where the store allows
        "for n in clang++ clang; do ln " .. q(mcxx) .. " \"$D/bin/$n\" 2>/dev/null || cp " .. q(mcxx) .. " \"$D/bin/$n\"; done",
        "ln -s clang++ \"$D/bin/mcxx\"",
        -- The llvm payload's configuration, with the target stated.
        "for n in clang++ clang; do { echo --target=" .. TARGET .. "; cat \"$L/bin/$n.cfg\" 2>/dev/null; } > \"$D/bin/$n.cfg\"; done",
    }, "\n")
    if not sh("sh -c " .. q(script)) then
        raise("mcxx: assembling the payload failed")
    end

    -- mcpp takes a toolchain directory as installed when its marker is there, and would otherwise
    -- try to install xim:llvm@23.1.0-mcxx from an index that does not have it.
    io.writefile(path.join(dir, ".mcpp_ok"), "1\n")

    local registration = llvm_registration(dir)
    os.tryrm(registration)
    os.mkdir(path.directory(registration))
    if not sh("ln -s " .. q(dir) .. " " .. q(registration)) then
        raise("mcxx: cannot register the payload as " .. registration)
    end
    log.info("mcxx: payload at %s, registered for mcpp as llvm@%s", dir, TOOLCHAIN_VERSION)
    return true
end

function config()
    xvm.add("mcxx", { bindir = path.join(pkginfo.install_dir(), "bin") })
    return true
end

function uninstall()
    xvm.remove("mcxx")
    os.tryrm(llvm_registration(pkginfo.install_dir()))
    return true
end
