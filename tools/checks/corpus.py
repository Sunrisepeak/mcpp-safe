#!/usr/bin/env python3
"""Corpus check (A0.5.1): every translation unit of a built corpus, checked by mcxx and by plain clang of the
same release -- zero errors, and the compiler's diagnostics the same; MC++'s own (its gates) apart.

    python3 tools/checks/corpus.py --corpus DIR --mcxx MCXX --clang CLANG [--jobs N] [--json FILE]

    DIR     a corpus built by mcpp (its compile_commands.json under target/, BMIs beside it); C-mcppls
            as tools/checks/selfhost.py leaves it
    MCXX    the mcxx the corpus was built with (the payload's bin/clang++)
    CLANG   clang 23.1 without MC++: speak-agent/llvm-clang-dev's tools/driver-smoke

Each unit's command runs as it is, with -fsyntax-only instead of its output, and with the same target and
resource directory given to both (the reference clang's defaults are its build's, not a toolchain's). A
diagnostic of MC++'s is one whose code is a feature an MC++ provider declares, or mcxx-*: it is counted,
not compared. Exits non-zero when mcxx reports an error, or the compiler diagnostics differ.
"""
import concurrent.futures, json, os, pathlib, re, subprocess, sys

def arg(name, default=None):
    return sys.argv[sys.argv.index(f"--{name}") + 1] if f"--{name}" in sys.argv else default

corpus = pathlib.Path(arg("corpus")).resolve()
mcxx = str(pathlib.Path(arg("mcxx")).absolute())   # links kept: mcxx selects its mode by the name it is started by
clang = str(pathlib.Path(arg("clang")).resolve())
jobs = int(arg("jobs", os.cpu_count() or 4))
report_file = arg("json")
target = arg("target", "x86_64-unknown-linux-gnu")

database = sorted(corpus.glob("target/*/*/compile_commands.json"), key=lambda p: p.stat().st_mtime)[-1]
units = json.loads(database.read_text())
resource = subprocess.run([mcxx, "-print-resource-dir"], capture_output=True, text=True).stdout.strip()
catalog = json.loads(subprocess.run([mcxx.replace("/clang++", "/mcxx") if os.path.exists(mcxx.replace("/clang++", "/mcxx")) else mcxx, "features", "--json"],
                                    capture_output=True, text=True).stdout or '{"features":[]}')
mcxx_codes = {f["id"] for f in catalog.get("features", [])}
line_re = re.compile(r"^(.+?):(\d+):(\d+): (error|warning|note|remark|fatal error): (.*)$")


def command(entry, compiler):
    args = list(entry.get("arguments") or entry["command"].split())
    cxx = os.path.basename(args[0]).endswith(("++", "++.exe"))
    # mcxx: its payload's driver of the same kind (the mode is the name); the reference: told the mode
    if compiler == mcxx:
        out = [mcxx if cxx else str(pathlib.Path(mcxx).with_name("clang"))]
    else:
        out = [compiler, "--driver-mode=g++" if cxx else "--driver-mode=gcc"]
    skip = False
    for a in args[1:]:
        if skip:
            skip = False
            continue
        if a in ("-o", "-MF", "-MT", "-MQ"):
            skip = True
            continue
        if a in ("-c", "-MD", "-MMD") or a.startswith("-fmodule-output") or (a.startswith("-o") and len(a) > 2 and not a.startswith("-objc")):
            continue
        out.append(a)
    return out + ["-fsyntax-only", f"--target={target}", f"-resource-dir={resource}"]


def mine(message):
    # MC++'s diagnostics name their feature (or mcxx-*) in brackets, anywhere in the message
    return any(c in mcxx_codes or c.startswith("mcxx-") for c in re.findall(r"\[([A-Za-z0-9:._-]+)\]", message))


def diagnostics(entry, compiler):
    run = subprocess.run(command(entry, compiler), cwd=entry["directory"], capture_output=True, text=True)
    found = []
    for line in run.stderr.splitlines():
        m = line_re.match(line)
        if m:
            found.append((m.group(1), int(m.group(2)), int(m.group(3)), m.group(4), m.group(5)))
        elif ": error: " in line:   # the driver's own, with no place: "clang++: error: ..."
            found.append(("", 0, 0, "error", line.split(": error: ", 1)[1]))
    return run.returncode, found


def check(entry):
    rc_m, by_mcxx = diagnostics(entry, mcxx)
    rc_c, by_clang = diagnostics(entry, clang)
    ours = [d for d in by_mcxx if mine(d[4])]
    compiler_m = sorted(d for d in by_mcxx if not mine(d[4]))
    return {"file": entry["file"], "mcxx-exit": rc_m, "clang-exit": rc_c, "mcxx": compiler_m, "clang": sorted(by_clang),
            "gates": ours, "errors": [d for d in by_mcxx if d[3] in ("error", "fatal error") and not mine(d[4])]}


with concurrent.futures.ThreadPoolExecutor(max_workers=jobs) as pool:
    results = list(pool.map(check, units))

different = [r for r in results if r["mcxx"] != r["clang"]]
errors = [r for r in results if r["errors"] or (r["mcxx-exit"] != 0 and not r["gates"])]
gates = sum(len(r["gates"]) for r in results)
print(f"{len(results)} units of {corpus.name}: {len(errors)} with errors under mcxx, {len(different)} whose compiler diagnostics differ "
      f"from clang's, {sum(len(r['mcxx']) for r in results)} compiler diagnostics, {gates} of MC++'s own")
for r in errors[:10]:
    print(f"  error: {r['file']}: {r['errors'][:2]}")
for r in different[:10]:
    print(f"  differs: {r['file']}\n    mcxx:  {r['mcxx'][:3]}\n    clang: {r['clang'][:3]}")
for r in results:
    for d in r["gates"]:
        print(f"  MC++: {d[0]}:{d[1]}:{d[2]}: {d[3]}: {d[4][:140]}")
if report_file:
    pathlib.Path(report_file).write_text(json.dumps({"units": len(results), "errors": len(errors), "different": len(different), "mcxx-diagnostics": gates,
                                                     "results": results}, indent=1))
sys.exit(1 if errors or different else 0)
