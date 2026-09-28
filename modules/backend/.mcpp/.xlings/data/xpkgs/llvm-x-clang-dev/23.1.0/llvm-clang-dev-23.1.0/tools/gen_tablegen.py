#!/usr/bin/env python3
"""Run every TableGen step the upstream CMake build declares, into llvm-generated/.

The declarations are upstream's: each `CMakeLists.txt` under llvm/llvm/{include,lib} and
llvm/clang/{include,lib} that calls `tablegen(...)` or `clang_tablegen(...)`. This script reads them
with the handful of CMake commands those files use (set, macro/endmacro, the two tablegen
functions), and runs the tools that tools/tblgen builds with mcpp:

    llvm-min-tblgen   project LLVM_HEADERS: everything declared under llvm/include/llvm
    llvm-tblgen       project LLVM elsewhere (clang's Options.inc)
    clang-tblgen      clang_tablegen(...)

Outputs go where CMake's binary directory would put them, rooted at llvm-generated/ instead:
    llvm/llvm/include/<p>   -> llvm-generated/include/<p>
    llvm/clang/include/<p>  -> llvm-generated/include/<p>
    llvm/clang/lib/<p>      -> llvm-generated/clang-lib/<p>     (included by that library's sources)

    tools/gen_tablegen.py              build the tools (mcpp) and regenerate everything
    tools/gen_tablegen.py --list       print the steps without running them
"""
import os
import pathlib
import re
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
UP = ROOT / "llvm"
GEN = ROOT / "llvm-generated"
TBLGEN = ROOT / "tools" / "tblgen"

SCAN = [UP / "llvm/include", UP / "llvm/lib", UP / "clang/include", UP / "clang/lib"]
SKIP = [UP / "clang/lib/Headers"]   # generates ARM/RISC-V intrinsic headers of the resource dir


def tokenize(text):
    """CMake commands as (name, [args]) with comments removed and quotes kept as one argument."""
    text = re.sub(r"#\[\[.*?\]\]", "", text, flags=re.S)
    out, i, n = [], 0, len(text)
    while i < n:
        m = re.compile(r"\s*([A-Za-z_][A-Za-z0-9_]*)\s*\(").match(text, i)
        if not m:   # a comment, a blank line or a continuation: go to the next line
            j = text.find("\n", i)
            i = n if j < 0 else j + 1
            continue
        name, i = m.group(1), m.end()
        args, cur, depth = [], "", 1
        while i < n and depth:
            c = text[i]
            if c == "#" and not cur:
                i = text.find("\n", i)
                i = n if i < 0 else i
                continue
            if c == '"':
                j = text.index('"', i + 1)
                cur += text[i + 1:j]
                i = j + 1
                continue
            if c == "(":
                depth += 1
                cur += c
            elif c == ")":
                depth -= 1
                if depth:
                    cur += c
            elif c.isspace():
                if cur:
                    args.append(cur)
                    cur = ""
            else:
                cur += c
            i += 1
        if cur:
            args.append(cur)
        out.append((name.lower(), args))
    return out


def expand(s, env):
    for _ in range(5):
        s2 = re.sub(r"\$\{(\w+)\}", lambda m: env.get(m.group(1), ""), s)
        if s2 == s:
            break
        s = s2
    return s


def out_dir(src_dir: pathlib.Path) -> pathlib.Path:
    rel = src_dir.relative_to(UP)
    parts = rel.parts
    if parts[1] == "include":
        return GEN / "include" / pathlib.Path(*parts[2:])
    return GEN / ("%s-lib" % parts[0]) / pathlib.Path(*parts[2:])


def collect():
    steps = []
    for base in SCAN:
        for cm in sorted(base.rglob("CMakeLists.txt")):
            d = cm.parent
            if any(str(d).startswith(str(s)) for s in SKIP):
                continue
            project_root = UP / d.relative_to(UP).parts[0]
            under_llvm_include = str(d).startswith(str(UP / "llvm/include/llvm"))
            env = {"CMAKE_CURRENT_SOURCE_DIR": str(d), "PROJECT_SOURCE_DIR": str(project_root),
                   "LLVM_MAIN_SRC_DIR": str(UP / "llvm"), "CLANG_SOURCE_DIR": str(UP / "clang"),
                   "CMAKE_CURRENT_BINARY_DIR": str(out_dir(d))}
            macros = {}
            cmds = tokenize(cm.read_text())
            k = 0

            def run(cmds, env):
                i = 0
                while i < len(cmds):
                    name, args = cmds[i]
                    if name == "macro":
                        j = i + 1
                        while cmds[j][0] != "endmacro":
                            j += 1
                        macros[args[0].lower()] = (args[1:], cmds[i + 1:j])
                        i = j + 1
                        continue
                    if name in macros:
                        params, body = macros[name]
                        e = dict(env)
                        e.update({p: expand(a, env) for p, a in zip(params, args)})
                        run(body, e)
                    elif name == "set" and args:
                        env[args[0]] = " ".join(expand(a, env) for a in args[1:])
                    elif name == "tablegen":
                        a = [expand(x, env) for x in args]
                        project, ofn, rest = a[0], a[1], a[2:]
                        extra, flags, mode = [], [], None
                        for x in rest:
                            if x in ("EXTRA_INCLUDES", "DEPENDS"):
                                mode = x
                            elif mode == "EXTRA_INCLUDES":
                                extra.append(x)
                            elif mode == "DEPENDS":
                                pass
                            else:
                                flags.append(x)
                        tool = "llvm-min-tblgen" if under_llvm_include else "llvm-tblgen"
                        steps.append(dict(tool=tool, td=env["LLVM_TARGET_DEFINITIONS"], dir=d, out=out_dir(d) / ofn,
                                          flags=flags, extra=extra))
                    elif name == "clang_tablegen":
                        a = [expand(x, env) for x in args]
                        ofn, rest = a[0], a[1:]
                        flags, extra, src, mode = [], [], None, None
                        for x in rest:
                            if x in ("SOURCE", "TARGET", "DEPENDS", "EXTRA_INCLUDES"):
                                mode = x
                            elif mode == "SOURCE":
                                src, mode = x, None
                            elif mode == "EXTRA_INCLUDES":
                                extra.append(x)
                            elif mode in ("TARGET", "DEPENDS"):
                                pass
                            else:
                                flags.append(x)
                        steps.append(dict(tool="clang-tblgen", td=src, dir=d, out=out_dir(d) / ofn, flags=flags, extra=extra))
                    i += 1

            run(cmds, env)
    return steps


def tool_path(name):
    found = sorted(TBLGEN.glob("*/target/*/*/bin/" + name), key=lambda p: p.stat().st_mtime)
    if not found:
        sys.exit("no %s under tools/tblgen/*/target: build it with mcpp first" % name)
    return str(found[-1])


def build_tools(members):
    for m in members:
        subprocess.run(["mcpp", "build", "-p", m], cwd=TBLGEN, check=True, stdout=subprocess.DEVNULL)


def run_step(s):
    td = pathlib.Path(s["td"])
    if not td.is_absolute():
        td = s["dir"] / td
    if not td.exists():
        return "missing td " + str(td)
    includes = [s["dir"], *map(pathlib.Path, s["extra"]), UP / "llvm/include", UP / "clang/include", GEN / "include", td.parent]
    cmd = [tool_path(s["tool"])] + sum((["-I", str(p)] for p in includes), []) + [str(td), "-o", str(s["out"]) + ".tmp"] + s["flags"]
    s["out"].parent.mkdir(parents=True, exist_ok=True)
    r = subprocess.run(cmd, capture_output=True, text=True)
    if r.returncode:
        return "failed: " + " ".join(cmd) + "\n" + r.stderr[-2000:]
    tmp = pathlib.Path(str(s["out"]) + ".tmp")
    if s["out"].exists() and s["out"].read_bytes() == tmp.read_bytes():
        tmp.unlink()
    else:
        tmp.replace(s["out"])
    return None


def main():
    steps = collect()
    if "--list" in sys.argv:
        for s in steps:
            print(s["tool"], s["out"].relative_to(ROOT), s["td"], " ".join(s["flags"]))
        print(len(steps), "steps")
        return
    failures = []
    for phase, tools, build in [(1, {"llvm-min-tblgen"}, ["min"]), (2, {"llvm-tblgen", "clang-tblgen"}, ["llvm", "clang"])]:
        build_tools(build)
        for s in [s for s in steps if s["tool"] in tools]:
            err = run_step(s)
            if err:
                failures.append("%s: %s" % (s["out"].relative_to(ROOT), err))
        print("phase %d: %d steps" % (phase, sum(1 for s in steps if s["tool"] in tools)))
    for f in failures:
        print("FAIL", f)
    sys.exit(1 if failures else 0)


if __name__ == "__main__":
    main()
