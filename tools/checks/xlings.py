#!/usr/bin/env python3
"""E-XL-1: mcxx through xlings, from this repository's own xpkg -- installed, selected, run.

    python3 tools/checks/xlings.py --mcxx MCXX --headers DIR [--xlings-home DIR]

    MCXX       a built mcxx: the payload's program (there is no download yet, xpkgs/pkgs/m/mcxx.lua)
    --headers  Clang 23.1's builtin headers (llvm.clang-dev's llvm/clang/lib/Headers)
    --xlings-home  the xlings mcpp uses (default ~/.mcpp/registry), where the toolchain must land

`xlings config --add-xpkg xpkgs/pkgs/m/mcxx.lua`, `xlings install mcxx:mcxx@0.1.0`, `xlings use mcxx 0.1.0`;
then the `mcxx` xlings puts on its path is the one given (`version --json` says the same), xlings reports
it installed and active, and mcpp lists the toolchain `llvm 23.1.0-mcxx` it registers. Exits non-zero on
any failure.
"""
import json, os, pathlib, subprocess, sys

def arg(name, default=None):
    return sys.argv[sys.argv.index(f"--{name}") + 1] if f"--{name}" in sys.argv else default

repo = pathlib.Path(__file__).resolve().parents[2]
mcxx = str(pathlib.Path(arg("mcxx")).resolve())
headers = str(pathlib.Path(arg("headers")).resolve())
home = pathlib.Path(arg("xlings-home", pathlib.Path.home() / ".mcpp/registry")).resolve()
xlings = str(home / "bin/xlings")
env = dict(os.environ, XLINGS_HOME=str(home), MCXX_BINARY=mcxx, MCXX_CLANG_HEADERS=headers)
failures = []


def check(label, ok, detail=""):
    print(f"{'PASS' if ok else 'FAIL'}  {label}" + (f"\n      {detail}" if detail and not ok else ""))
    if not ok:
        failures.append(label)


def run(*args):
    return subprocess.run([xlings, "--agent", *args], capture_output=True, text=True, env=env)


added = run("config", "--add-xpkg", str(repo / "xpkgs/pkgs/m/mcxx.lua"))
check("xlings takes this repository's xpkg (config --add-xpkg)", added.returncode == 0, added.stdout[-300:] + added.stderr[-300:])
installed = run("install", "mcxx:mcxx@0.1.0", "-y")
check("xlings install mcxx:mcxx@0.1.0", installed.returncode == 0 and "mcxx:mcxx@0.1.0" in installed.stdout, installed.stdout[-500:] + installed.stderr[-300:])
used = run("use", "mcxx", "0.1.0")
check("xlings use mcxx 0.1.0", used.returncode == 0, used.stdout[-300:] + used.stderr[-300:])
info = run("info", "mcxx:mcxx")
check("xlings says it is installed and active", "selected installed: yes" in info.stdout and "(active)" in info.stdout, info.stdout[-400:])

shim = home / "subos/current/bin/mcxx"
if shim.exists():
    via = subprocess.run([str(shim), "version", "--json"], capture_output=True, text=True, env=env)
    direct = subprocess.run([mcxx, "version", "--json"], capture_output=True, text=True)
    same = via.returncode == 0 and json.loads(via.stdout) == json.loads(direct.stdout)
    check("the mcxx on xlings' path is the one installed: version --json the same", same, via.stdout[:200] + via.stderr[:200])
else:
    check("xlings put mcxx on its path", False, f"no {shim}")
toolchains = subprocess.run(["mcpp", "toolchain", "list"], capture_output=True, text=True)
check("mcpp lists the toolchain llvm 23.1.0-mcxx as installed",
      any("23.1.0-mcxx" in line and "installed" in line for line in toolchains.stdout.splitlines()), toolchains.stdout[-400:])

print(f"{'FAIL' if failures else 'PASS'}: mcxx through xlings ({len(failures)} failed)")
sys.exit(1 if failures else 0)
