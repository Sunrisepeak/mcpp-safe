-- mcxx -- MC++'s compiler as an llvm-family toolchain payload (E-XIM-1, E-XIM-3, V0.6, MC5 section 7).
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
-- compiler-rt, lld, binutils, clang-scan-deps, the platform's configuration -- is the xim:llvm
-- 22.1.8 payload this package depends on. Installing also registers the payload where mcpp looks
-- up an llvm toolchain, as xim-x-llvm/23.1.0-mcxx (removed again on uninstall).
--
-- Linux, macOS (arm64) and Windows (x86_64): the mcxx program is the one built for that platform
-- (mcpp build --target aarch64-macos / x86_64-windows-gnu, cross-built from Linux). Its default
-- target is the one the xim:llvm payload's own clang++ reports, stated in the configuration files
-- beside it: on Windows that is MSVC's, which mcpp builds for there, not the MinGW triple mcxx
-- itself was built for. On Windows the payload's directories are junctions and its files hard
-- links (a copy where a link cannot be made): neither needs privileges.
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
    archs = {"x86_64", "arm64"},
    categories = {"compiler", "toolchain", "llvm", "c++", "modules"},
    keywords = {"mcxx", "mc++", "clang", "modules", "feature-gates"},

    programs = {"mcxx"},

    xpm = {
        linux = {
            deps = {"xim:llvm@22.1.8"},
            ["latest"] = { ref = "0.1.0" },
            ["0.1.0"] = {},
        },
        macosx = {
            deps = {"xim:llvm@22.1.8"},
            ["latest"] = { ref = "0.1.0" },
            ["0.1.0"] = {},
        },
        windows = {
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
local LINUX_TARGET = "x86_64-unknown-linux-gnu"

-- A shell command; false when it fails (os.exec raises).
local function sh(cmd)
    local ok = try { function() os.exec(cmd); return true end }
    return ok == true
end

local function q(p) return "'" .. p:gsub("'", "'\\''") .. "'" end

-- A PowerShell string literal.
local function pq(p) return "'" .. p:gsub("'", "''") .. "'" end

-- A PowerShell script, run from a file (a script this long does not survive cmd's quoting), written
-- beside the payload: the package's own directory in the store.
local scripts = 0
local function ps(script)
    scripts = scripts + 1
    local file = path.join(path.directory(pkginfo.install_dir()), "mcxx-xpkg-" .. scripts .. ".ps1")
    io.writefile(file, "$ErrorActionPreference = 'Stop'\n" .. script .. "\n")
    local ok = sh('powershell -NoProfile -NonInteractive -ExecutionPolicy Bypass -File "' .. file .. '"')
    os.tryrm(file)
    return ok
end

-- The junctions under a Windows payload, unlinked before anything under it is removed: removing a
-- junction's contents would remove the llvm payload's. Only the levels this package links at.
local function ps_unlink(dir)
    return table.concat({
        "$D = " .. pq(dir),
        "foreach ($level in @($D, \"$D/bin\", \"$D/lib\", \"$D/lib/clang\", \"$D/lib/clang/" .. CLANG_MAJOR .. "\")) {",
        "  if (Test-Path -LiteralPath $level) {",
        "    Get-ChildItem -LiteralPath $level -Force | Where-Object { $_.Attributes -band [IO.FileAttributes]::ReparsePoint } |",
        "      ForEach-Object { [IO.Directory]::Delete($_.FullName, $false) }",
        "  }",
        "}",
    }, "\n")
end

-- The target the dependency's clang++ builds for, which mcpp configured it for on this host.
local function dumpmachine(llvm)
    local exe = path.join(llvm, "bin", os.host() == "windows" and "clang++.exe" or "clang++")
    local f = io.popen('"' .. exe .. '" -dumpmachine')
    if not f then return nil end
    local out = f:read("*a") or ""
    f:close()
    out = out:gsub("%s+$", "")
    return out ~= "" and out or nil
end

-- Where mcpp looks an llvm toolchain up: <store>/xim-x-llvm/<version>, beside this package's own
-- <store>/<namespace>-x-mcxx/<version>.
local function llvm_registration(install_dir)
    local store = path.directory(path.directory(install_dir))
    return path.join(store, "xim-x-llvm", TOOLCHAIN_VERSION)
end

local function assemble_posix(dir, llvm, mcxx, headers, target, macos)
    local script = table.concat({
        "set -e",
        "L=" .. q(llvm), "D=" .. q(dir),
        "for top in \"$L\"/*; do n=$(basename \"$top\"); case $n in bin|lib) ;; *) ln -s \"$top\" \"$D/$n\";; esac; done",
        "for t in \"$L\"/bin/*; do n=$(basename \"$t\"); case $n in clang-scan-deps*) ln -s \"$t\" \"$D/bin/$n\";; clang*|*.cfg) ;; *) ln -s \"$t\" \"$D/bin/$n\";; esac; done",
        "for i in \"$L\"/lib/*; do n=$(basename \"$i\"); [ \"$n\" = clang ] || ln -s \"$i\" \"$D/lib/$n\"; done",
        "R=$(ls -d \"$L\"/lib/clang/* | head -1)",
        "cp -R " .. q(headers) .. " \"$D/lib/clang/" .. CLANG_MAJOR .. "/include\"",
        -- The headers Clang's build generates (arm_neon.h, the other ARM, AArch64 and RISC-V
        -- intrinsics), which llvm.clang-dev carries beside clang/lib/Headers since 23.1.0.5.
        "G=" .. q(headers) .. "/../../../../llvm-generated/clang-lib/Headers; if [ -d \"$G\" ]; then cp \"$G\"/* \"$D/lib/clang/" .. CLANG_MAJOR .. "/include/\"; fi",
        "[ -d \"$R/lib\" ] && ln -s \"$R/lib\" \"$D/lib/clang/" .. CLANG_MAJOR .. "/lib\"",
        -- the llvm payload's clang-scan-deps looks under lib/clang/<its major> for the builtin headers
        "r=$(basename \"$R\"); [ \"$r\" = " .. CLANG_MAJOR .. " ] || ln -s " .. CLANG_MAJOR .. " \"$D/lib/clang/$r\"",
        -- mcxx under the names its driver selects the mode by. macOS: a copy, signed ad hoc (a program
        -- built elsewhere does not start on arm64 unsigned), and the other names linked to it.
        macos and ("cp " .. q(mcxx) .. " \"$D/bin/clang++\" && chmod 755 \"$D/bin/clang++\" && codesign -s - -f \"$D/bin/clang++\" && ln \"$D/bin/clang++\" \"$D/bin/clang\"")
              or ("for n in clang++ clang; do ln " .. q(mcxx) .. " \"$D/bin/$n\" 2>/dev/null || cp " .. q(mcxx) .. " \"$D/bin/$n\"; done"),
        "ln -s clang++ \"$D/bin/mcxx\"",
        -- The llvm payload's configuration, with the target stated.
        "for n in clang++ clang; do { echo --target=" .. target .. "; cat \"$L/bin/$n.cfg\" 2>/dev/null; } > \"$D/bin/$n.cfg\"; done",
    }, "\n")
    return sh("sh -c " .. q(script))
end

local function assemble_windows(dir, llvm, mcxx, headers, target)
    return ps(table.concat({
        "$L = " .. pq(llvm), "$D = " .. pq(dir), "$H = " .. pq(headers), "$M = " .. pq(mcxx), "$T = " .. pq(target),
        "$V = '" .. CLANG_MAJOR .. "'",
        "New-Item -ItemType Directory -Force -Path \"$D/bin\", \"$D/lib/clang/$V\" | Out-Null",
        "function Link($src, $dst) {",
        "  if (Test-Path -LiteralPath $src -PathType Container) { New-Item -ItemType Junction -Path $dst -Target $src | Out-Null }",
        "  else { try { New-Item -ItemType HardLink -Path $dst -Target $src -ErrorAction Stop | Out-Null } catch { Copy-Item -LiteralPath $src -Destination $dst } }",
        "}",
        "Get-ChildItem -LiteralPath $L | Where-Object { $_.Name -ne 'bin' -and $_.Name -ne 'lib' } | ForEach-Object { Link $_.FullName (Join-Path $D $_.Name) }",
        "Get-ChildItem -LiteralPath \"$L/bin\" | Where-Object { $_.Name -like 'clang-scan-deps*' -or -not ($_.Name -like 'clang*' -or $_.Name -like '*.cfg') } |",
        "  ForEach-Object { Link $_.FullName (Join-Path \"$D/bin\" $_.Name) }",
        "Get-ChildItem -LiteralPath \"$L/lib\" | Where-Object { $_.Name -ne 'clang' } | ForEach-Object { Link $_.FullName (Join-Path \"$D/lib\" $_.Name) }",
        -- The tools clang's driver starts (lld-link for -fuse-ld=lld) also by their names without .exe:
        -- LLVM, built as for Linux on openkal's Windows target, looks a program up by the name alone and
        -- adds no extension, so it found no lld-link beside itself and could not start the bare name.
        -- Found, it is started by that full path, which CreateProcessW runs as the image it is.
        "Get-ChildItem -LiteralPath \"$L/bin\" -Filter *.exe | Where-Object { $_.Name -like 'clang-scan-deps*' -or -not ($_.Name -like 'clang*') } |",
        "  ForEach-Object { Link $_.FullName (Join-Path \"$D/bin\" $_.BaseName) }",
        "Copy-Item -Recurse -LiteralPath $H -Destination \"$D/lib/clang/$V/include\"",
        "$G = Join-Path $H '../../../../llvm-generated/clang-lib/Headers'",
        "if (Test-Path -LiteralPath $G) { Copy-Item -Path \"$G/*\" -Destination \"$D/lib/clang/$V/include/\" }",
        "$R = Get-ChildItem -LiteralPath \"$L/lib/clang\" -Directory -ErrorAction SilentlyContinue | Select-Object -First 1",
        "if ($R) {",
        "  if (Test-Path -LiteralPath (Join-Path $R.FullName 'lib')) { Link (Join-Path $R.FullName 'lib') \"$D/lib/clang/$V/lib\" }",
        "  if ($R.Name -ne $V) { New-Item -ItemType Junction -Path \"$D/lib/clang/$($R.Name)\" -Target \"$D/lib/clang/$V\" | Out-Null }",
        "}",
        "foreach ($n in 'clang++.exe', 'clang.exe', 'mcxx.exe') { Link $M \"$D/bin/$n\" }",
        "foreach ($n in 'clang++', 'clang') {",
        "  $cfg = \"--target=$T`n\"",
        "  if (Test-Path -LiteralPath \"$L/bin/$n.cfg\") { $cfg += Get-Content -Raw -LiteralPath \"$L/bin/$n.cfg\" }",
        "  [IO.File]::WriteAllText(\"$D/bin/$n.cfg\", $cfg)",
        "}",
    }, "\n"))
end

-- Removes the payload (and, on Windows, its junctions first, never what they point at).
local function remove_payload(dir)
    if os.host() == "windows" and os.isdir(dir) then
        ps(ps_unlink(dir))
    end
    os.tryrm(dir)
end

local function register(dir, registration)
    if os.host() == "windows" then
        return ps("$P = " .. pq(registration) .. "\n" ..
                  "if (Test-Path -LiteralPath $P) { [IO.Directory]::Delete($P, $false) }\n" ..
                  "New-Item -ItemType Directory -Force -Path (Split-Path -Parent $P) | Out-Null\n" ..
                  "New-Item -ItemType Junction -Path $P -Target " .. pq(dir) .. " | Out-Null")
    end
    os.tryrm(registration)
    os.mkdir(path.directory(registration))
    return sh("ln -s " .. q(dir) .. " " .. q(registration))
end

local function unregister(registration)
    if os.host() == "windows" then
        ps("$P = " .. pq(registration) .. "\nif (Test-Path -LiteralPath $P) { [IO.Directory]::Delete($P, $false) }")
        return
    end
    os.tryrm(registration)
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
    local clangxx = path.join(llvm or "", "bin", os.host() == "windows" and "clang++.exe" or "clang++")
    if not llvm or not os.isfile(clangxx) then
        raise("mcxx: the xim:llvm 22.1.8 payload this package depends on is not installed")
    end
    local target = LINUX_TARGET
    if os.host() ~= "linux" then
        target = dumpmachine(llvm)
        if not target then raise("mcxx: " .. clangxx .. " -dumpmachine said nothing") end
    end

    remove_payload(dir)
    os.mkdir(path.join(dir, "bin"))
    os.mkdir(path.join(dir, "lib", "clang", CLANG_MAJOR))

    -- Everything but the compilers and the resource directory is the llvm payload's, by link.
    local ok
    if os.host() == "windows" then
        ok = assemble_windows(dir, llvm, mcxx, headers, target)
    else
        ok = assemble_posix(dir, llvm, mcxx, headers, target, os.host() == "macosx")
    end
    if not ok then
        raise("mcxx: assembling the payload failed")
    end

    -- mcpp takes a toolchain directory as installed when its marker is there, and would otherwise
    -- try to install xim:llvm@23.1.0-mcxx from an index that does not have it.
    io.writefile(path.join(dir, ".mcpp_ok"), "1\n")

    local registration = llvm_registration(dir)
    if not register(dir, registration) then
        raise("mcxx: cannot register the payload as " .. registration)
    end
    log.info("mcxx: payload at %s for %s, registered for mcpp as llvm@%s", dir, target, TOOLCHAIN_VERSION)
    return true
end

function config()
    xvm.add("mcxx", { bindir = path.join(pkginfo.install_dir(), "bin") })
    return true
end

function uninstall()
    xvm.remove("mcxx")
    unregister(llvm_registration(pkginfo.install_dir()))
    remove_payload(pkginfo.install_dir())
    return true
end
