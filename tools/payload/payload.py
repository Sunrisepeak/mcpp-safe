#!/usr/bin/env python3
"""Assembles mcxx as an llvm-family toolchain payload (V0.6, MC5 section 7): the layout of xim:llvm,
with mcxx as its compiler.

    python3 tools/payload/payload.py --mcxx MCXX --llvm LLVM_PAYLOAD --headers CLANG_HEADERS --out DIR

    MCXX           the mcxx program (a build of this repository)
    LLVM_PAYLOAD   an installed xim:llvm payload, e.g. ~/.mcpp/registry/data/xpkgs/xim-x-llvm/22.1.8:
                   libc++ (headers, archives, std module sources), compiler-rt, lld, binutils and
                   clang-scan-deps come from it
    CLANG_HEADERS  Clang 23.1's builtin headers (llvm/clang/lib/Headers of llvm.clang-dev)
    DIR            the payload; to use it from mcpp as `llvm@23.1.0-mcxx`, DIR is
                   <mcpp registry>/data/xpkgs/xim-x-llvm/23.1.0-mcxx

In the payload:
  bin/clang++, bin/clang    mcxx (hard links, or copies across file systems): a compiler finds its
                            resource directory beside the path it was started by
  bin/mcxx                  a link to clang++, as the xpkg installs it
  bin/clang++.cfg, clang.cfg  the LLVM payload's, with the target stated (mcxx's own default is the
                            openkal host, x86_64-unknown-linux-musl) and its paths kept
  lib/clang/23/include      Clang 23's builtin headers; lib/clang/23/lib: the LLVM payload's compiler-rt
  lib/clang/<llvm's major>  a link to 23: the llvm payload's clang-scan-deps looks there for them
  everything else           symbolic links into the LLVM payload
  .mcxx-payload.json        what it was made of
"""
import json, os, pathlib, shutil, sys


def arg(name, required=True):
    if f"--{name}" in sys.argv:
        return sys.argv[sys.argv.index(f"--{name}") + 1]
    if required:
        sys.exit(f"payload.py: --{name} is required (see the top of this file)")
    return None


mcxx = pathlib.Path(arg("mcxx")).resolve()
llvm = pathlib.Path(arg("llvm")).resolve()
headers = pathlib.Path(arg("headers")).resolve()
out = pathlib.Path(arg("out"))
target = arg("target", required=False) or "x86_64-unknown-linux-gnu"
for p, what in ((mcxx, "mcxx"), (llvm / "bin" / "clang++", "the llvm payload"), (headers / "stddef.h", "the Clang headers")):
    if not p.exists():
        sys.exit(f"payload.py: {what} not found at {p}")
llvm_resource = next((llvm / "lib" / "clang").iterdir())   # lib/clang/<major>

if out.exists():
    shutil.rmtree(out)
(out / "bin").mkdir(parents=True)

# Everything of the llvm payload but its compilers and the resource directory, by link.
for top in llvm.iterdir():
    if top.name in ("bin", "lib"):
        continue
    (out / top.name).symlink_to(top)
for tool in (llvm / "bin").iterdir():
    if tool.name.startswith("clang") and not tool.name.startswith("clang-scan-deps") or tool.suffix == ".cfg":
        continue
    (out / "bin" / tool.name).symlink_to(tool)
(out / "lib").mkdir()
for item in (llvm / "lib").iterdir():
    if item.name == "clang":
        continue
    (out / "lib" / item.name).symlink_to(item)
resource = out / "lib" / "clang" / "23"
resource.mkdir(parents=True)
shutil.copytree(headers, resource / "include", symlinks=True)
if (llvm_resource / "lib").exists():
    (resource / "lib").symlink_to(llvm_resource / "lib")
# clang-scan-deps is the llvm payload's: it finds the builtin headers under lib/clang/<ITS major>
# beside the compiler a command names -- here, mcxx. Without this a module unit that includes a C
# header (<cstdio> in a global module fragment) is not scanned: 'stddef.h' file not found.
if llvm_resource.name != "23":
    (out / "lib" / "clang" / llvm_resource.name).symlink_to("23")

# The compiler: mcxx, under the names the driver selects its mode by (MC5-2-2).
for name in ("clang++", "clang"):
    dest = out / "bin" / name
    try:
        os.link(mcxx, dest)
    except OSError:
        shutil.copy2(mcxx, dest)
    cfg = llvm / "bin" / f"{name}.cfg"
    lines = cfg.read_text().splitlines() if cfg.exists() else []
    (out / "bin" / f"{name}.cfg").write_text("\n".join([f"--target={target}", *lines]) + "\n")

# And as itself, as the xpkg installs it (xpkgs/pkgs/m/mcxx.lua): `mcxx version`, `mcxx check`.
(out / "bin" / "mcxx").symlink_to("clang++")

(out / ".mcxx-payload.json").write_text(json.dumps({"mcxx": str(mcxx), "llvm": str(llvm), "clang-headers": str(headers), "target": target}, indent=2) + "\n")
print(out)
