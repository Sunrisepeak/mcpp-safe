#!/usr/bin/env python3
"""MC++'s preprocessor against Clang's (A1.6.2): for every source file of a corpus's compile database, the
tokens mcxx.frontend:preprocess gives are the main file's tokens in Clang's `-E` output for the same
command -- groups taken, the file's own macros expanded, module lines -- token for token.

    python3 tools/checks/ppdiff.py --lexdump MCXX_LEXDUMP --clang CLANGXX --db COMPILE_COMMANDS.json --root DIR [--header-macros] [--jobs N] [--show N]

    --db     a compile_commands.json; the files under --root are checked (a corpus's own, not its packages')
    --clang  Clang's side: the mcxx toolchain's clang++ (the payload's bin/clang++, xpkgs/README.md) with the
             command's -D, -U, -I, -isystem, --sysroot, -std and -nostdinc flags, and -E
    --lexdump  MC++'s side: `mcxx-lexdump --ppdiff` with the command's -D and -U, which compares

Each file is `equal`; or differs where MC++'s preprocessor said it was not certain (`uncertain`: the
compiler's answer is the one to use, plan P4); or differs at a name a header the file includes defines
as a macro (`header-macro`: MC++'s preprocessor reads no header -- a host that has those macros passes
them, PreprocessOptions::header_macros); or differs where it claimed to be certain (`certain`: a
defect). Exits non-zero when any file is `certain`, or Clang could not preprocess one.

With --header-macros the check plays such a host: Clang's `-dM -E` over the file's directives
(`mcxx-lexdump --directives`: its #defines and #undefs after the last #include aside, and #error and
#warning) gives the headers' macros,
less the names the file defines itself, and MC++'s preprocessor gets them -- `header-macro` should then
be none.
"""
import atexit, collections, concurrent.futures, json, os, pathlib, re, shutil, subprocess, sys, tempfile

def arg(name, default=None):
    return sys.argv[sys.argv.index(f"--{name}") + 1] if f"--{name}" in sys.argv else default

lexdump = str(pathlib.Path(arg("lexdump")).resolve())
clang = str(pathlib.Path(arg("clang")).resolve())
root = str(pathlib.Path(arg("root")).resolve())
jobs = int(arg("jobs", os.cpu_count() or 4))
show = int(arg("show", 10))
header_macros = "--header-macros" in sys.argv
units = [u for u in json.loads(pathlib.Path(arg("db")).read_text()) if u["file"].startswith(root + "/")
         and "/target/" not in u["file"] and "/.mcpp/" not in u["file"] and u["file"].endswith((".cpp", ".cppm", ".cc", ".cxx", ".ixx"))]
units = list({u["file"]: u for u in units}.values())
scratch = tempfile.mkdtemp(prefix="mcxx-ppdiff-")
atexit.register(shutil.rmtree, scratch, True)   # this run's only
KEEP_WITH_VALUE = {"-I", "-isystem", "-iquote", "-idirafter", "-D", "-U"}
KEEP = ("-D", "-U", "-I", "-isystem", "-iquote", "-idirafter", "-std=", "-nostdinc", "-nostdlibinc", "--no-default-config", "--target=", "-target",
        "--sysroot=")


def flags(arguments):
    out, i = [], 1
    while i < len(arguments):
        a = arguments[i]
        if a in KEEP_WITH_VALUE and i + 1 < len(arguments):
            out += [a, arguments[i + 1]]
            i += 2
            continue
        if a == "-target" and i + 1 < len(arguments):
            out += [a, arguments[i + 1]]
            i += 2
            continue
        if a.startswith(KEEP):
            out.append(a)
        i += 1
    return out


def headers_macros(unit, kept, language):
    """The macros the file's headers define, from Clang: -dM -E over the file's directive lines."""
    found = json.loads(subprocess.run([lexdump, "--directives", unit["file"]], capture_output=True, text=True, check=True).stdout)
    if not found["includes"]:
        return None
    source = os.path.join(scratch, f"{abs(hash(unit['file']))}.headers.cpp")
    pathlib.Path(source).write_text(found["text"])
    run = subprocess.run([clang, *kept, "-dM", "-E", "-x", "c++", source], capture_output=True, text=True,
                         cwd=unit["directory"] if os.path.isdir(unit.get("directory", "")) else root)
    os.unlink(source)
    if run.returncode != 0:
        return None
    own = set(found["own"])
    return "\n".join(l for l in run.stdout.splitlines() if (d := re.match(r"#define (\w+)", l)) and d.group(1) not in own) + "\n"


def check(unit):
    arguments = unit.get("arguments") or unit["command"].split()
    kept = flags(arguments)
    language = ["-x", "c++-module"] if unit["file"].endswith((".cppm", ".ixx")) else ["-x", "c++"]
    output = os.path.join(scratch, f"{abs(hash(unit['file']))}.i")
    run = subprocess.run([clang, *kept, "-E", *language, unit["file"], "-o", output], capture_output=True, text=True,
                         cwd=unit["directory"] if os.path.isdir(unit.get("directory", "")) else root)
    if run.returncode != 0:
        return {"file": unit["file"], "verdict": "clang-failed", "notes": [run.stderr[-400:]]}
    defines = []
    i = 0
    while i < len(kept):
        if kept[i] in ("-D", "-U"):
            defines.append(kept[i] + kept[i + 1])
            i += 2
            continue
        if kept[i].startswith(("-D", "-U")):
            defines.append(kept[i])
        i += 1
    extra = []
    if header_macros:
        macros = headers_macros(unit, kept, language)
        if macros is not None:
            extra = ["--header-macros", output + ".macros"]
            pathlib.Path(extra[1]).write_text(macros)
    ours = subprocess.run([lexdump, "--ppdiff", *defines, *extra, unit["file"], output], capture_output=True, text=True)
    os.unlink(output)
    if extra:
        os.unlink(extra[1])
    if ours.returncode != 0 or not ours.stdout.strip():
        return {"file": unit["file"], "verdict": "lexdump-failed", "notes": [ours.stderr[-400:]]}
    return json.loads(ours.stdout)


with concurrent.futures.ThreadPoolExecutor(max_workers=jobs) as pool:
    results = list(pool.map(check, units))

verdicts = collections.Counter(r["verdict"] for r in results)
for r in [r for r in results if r["verdict"] not in ("equal",)][:show]:
    print(f"{r['file']}: {r['verdict']}" + (f" at token {r['at']} (line {r['line']})\n    ours:  {r['ours']}\n    clang: {r['clang']}" if "at" in r else ""))
    for n in r.get("notes", [])[:3]:
        print(f"    note: {n.strip()[:300]}")
print(f"ppdiff: {len(results)} files: " + ", ".join(f"{n} {v}" for v, n in sorted(verdicts.items())))
sys.exit(1 if verdicts.get("certain") or verdicts.get("clang-failed") or verdicts.get("lexdump-failed") or not results else 0)
