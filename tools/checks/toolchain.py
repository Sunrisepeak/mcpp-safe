#!/usr/bin/env python3
"""mcxx as mcpp's toolchain (V0.6, MC5 section 7): mcpp, unchanged, builds and runs a modules program
with `llvm@23.1.0-mcxx`, the payload the xpkg xpkgs/pkgs/m/mcxx.lua installs.

    python3 tools/checks/toolchain.py [--toolchain llvm@23.1.0-mcxx] [--work DIR]

The payload must be installed (xpkgs/README.md). Checks: the build succeeds, the program prints what
it should, and its objects say clang 23.1 compiled them. The work directory is outside the
repository (default ~/.cache/mcxx-checks/toolchain). Exits non-zero on any failure.
"""
import pathlib, re, shutil, subprocess, sys

windows = sys.platform == "win32"

toolchain = sys.argv[sys.argv.index("--toolchain") + 1] if "--toolchain" in sys.argv else "llvm@23.1.0-mcxx"
work = pathlib.Path(sys.argv[sys.argv.index("--work") + 1] if "--work" in sys.argv else pathlib.Path.home() / ".cache/mcxx-checks/toolchain")
failures = 0


def check(label, ok, detail=""):
    global failures
    print(f"{'PASS' if ok else 'FAIL'}  {label}{('  ' + detail) if detail and not ok else ''}")
    failures += 0 if ok else 1


shutil.rmtree(work, ignore_errors=True)
(work / "src").mkdir(parents=True)
(work / "mcpp.toml").write_text('[package]\nname = "hello-modules"\nversion = "0.1.0"\n\n[targets.hello-modules]\nkind = "bin"\nmain = "src/main.cpp"\n')
(work / "src/greet.cppm").write_text('export module greet;\n\nimport std;\n\nexport std::string greeting(std::string_view who) { return std::format("hello, {}", who); }\n'
                                     'export constexpr int answer() { return 42; }\n')
(work / "src/main.cpp").write_text('import std;\nimport greet;\n\nint main() {\n    std::println("{} (answer={})", greeting("modules"), answer());\n    return 0;\n}\n')

build = subprocess.run(["mcpp", "build", "--toolchain", toolchain], cwd=work, capture_output=True, text=True)
check(f"mcpp build --toolchain {toolchain}", build.returncode == 0, (build.stdout + build.stderr)[-800:])
binaries = sorted(work.glob("target/*/*/bin/hello-modules" + (".exe" if windows else "")))
if binaries:
    run = subprocess.run([str(binaries[-1])], capture_output=True, text=True)
    check("the program runs and prints what it should", run.returncode == 0 and run.stdout.strip() == "hello, modules (answer=42)", run.stdout + run.stderr)
    ninja = next(iter(sorted(work.glob("target/*/*/build.ninja"))), None)
    # The payload's clang++ (clang++.exe), whichever separator the build's paths use.
    compiler = re.search(r"(\S*xim-x-llvm[/\\]" + re.escape(toolchain.split("@")[1]) + r"[/\\]bin[/\\]clang\+\+(?:\.exe)?)",
                         ninja.read_text()) if ninja else None
    check("the build's compiler is the payload's clang++", compiler is not None)
    if sys.platform.startswith("linux"):
        comment = subprocess.run(["readelf", "-p", ".comment", str(binaries[-1])], capture_output=True, text=True).stdout
        check("its code was compiled by clang 23.1 (mcxx)", "clang version 23.1.0" in comment, comment[:300])
    elif compiler:
        # PE and Mach-O objects keep no .comment: the compiler the build ran (the payload's, checked
        # above) says what it is -- run from mcpp's store, not from build.ninja's spelling of it
        # (ninja escapes a drive's colon).
        payload = pathlib.Path.home() / ".mcpp/registry/data/xpkgs/xim-x-llvm" / toolchain.split("@")[1] / "bin" / ("clang++.exe" if windows else "clang++")
        version = subprocess.run([str(payload), "--version"], capture_output=True, text=True).stdout
        check("its code was compiled by clang 23.1 (mcxx)", "clang version 23.1.0" in version, version[:300])
print(f"\n{failures} failed")
sys.exit(1 if failures else 0)
