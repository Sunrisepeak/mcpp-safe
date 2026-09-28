#!/usr/bin/env python3
"""mcxx as mcpp's toolchain (V0.6, MC5 section 7): mcpp, unchanged, builds and runs a modules program
with `llvm@23.1.0-mcxx`, the payload the xpkg xpkgs/pkgs/m/mcxx.lua installs.

    python3 tools/checks/toolchain.py [--toolchain llvm@23.1.0-mcxx] [--work DIR]

The payload must be installed (xpkgs/README.md). Checks: the build succeeds, the program prints what
it should, and its objects say clang 23.1 compiled them. The work directory is outside the
repository (default ~/.cache/mcxx-checks/toolchain). Exits non-zero on any failure.
"""
import pathlib, shutil, subprocess, sys

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
binaries = sorted(work.glob("target/*/*/bin/hello-modules"))
if binaries:
    run = subprocess.run([str(binaries[-1])], capture_output=True, text=True)
    check("the program runs and prints what it should", run.returncode == 0 and run.stdout.strip() == "hello, modules (answer=42)", run.stdout + run.stderr)
    comment = subprocess.run(["readelf", "-p", ".comment", str(binaries[-1])], capture_output=True, text=True).stdout
    check("its code was compiled by clang 23.1 (mcxx)", "clang version 23.1.0" in comment, comment[:300])
    ninja = next(iter(sorted(work.glob("target/*/*/build.ninja"))), None)
    check("the build's compiler is the payload's clang++", ninja is not None and f"xim-x-llvm/{toolchain.split('@')[1]}/bin/clang++" in ninja.read_text())
print(f"\n{failures} failed")
sys.exit(1 if failures else 0)
