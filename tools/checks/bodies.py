#!/usr/bin/env python3
"""MC++'s full parse over the corpora (A3.0.1, first half): every function body, initializer, default argument,
enumerator value and static_assert of a corpus's own sources, read into statements and expressions by
mcxx.frontend:bodies (M3.0) -- how many it read, how many it could not (a part that left a diagnostic), the error
nodes it made, who decided what the grammar leaves open (the parse's scopes, name lookup, the code's shape), and the time.

    python3 tools/checks/bodies.py --lexdump MCXX_LEXDUMP --corpus DIR [--corpus DIR ...]
                                   [--imports] [--header-macros --clang CLANGXX] [--cxx23] [--only SUBSTRING] [--show N] [--jobs N]
                                   [--max-failed N] [--report FILE]

    DIR          a corpus whose compile_commands.json is in it (or under target/*/*/): its own sources are read, not
                 its packages' or its build's; for each C-mcppls, C-mcpp, C-xlings: the checkout's root
    MCXX_LEXDUMP `--bodies`: the full parse, one JSON object per file
    --imports    give the parse what the units' imports bring in (-fmodule-file=, -fprebuilt-module-path= from their
                 commands), as the editor does: more names are known, fewer decided by the code's shape
    --header-macros  the headers' macros from Clang (--clang: the toolchain's clang++), as a host passes them
                 (PreprocessOptions::header_macros): without it, a macro a header defines (gtest's TEST) is left unexpanded
    --cxx23      without the C++26 syntax extension: the corpora are C++23, so it should read the same
    --show N     print the first N diagnostics of each corpus (default 10)
    --kinds      group the diagnostics by their message, most frequent first, with one place each
    --max-failed N  exit non-zero when more than N parts of a corpus (default 0) could not be read

Prints, per corpus: files, parts read, parts failed, error nodes, decisions by who made them. Exits non-zero when
a corpus has more failed parts than --max-failed, or a file did not parse at all.
"""
import atexit, collections, concurrent.futures, json, os, pathlib, re, shlex, shutil, subprocess, sys, tempfile


def args_of(name):
    out, i = [], 1
    while i < len(sys.argv):
        if sys.argv[i] == f"--{name}" and i + 1 < len(sys.argv):
            out.append(sys.argv[i + 1])
            i += 1
        i += 1
    return out


def arg(name, default=None):
    found = args_of(name)
    return found[-1] if found else default


lexdump = str(pathlib.Path(arg("lexdump")).resolve())
cache = tempfile.mkdtemp(prefix="mcxx-bodies-")
atexit.register(shutil.rmtree, cache, True)
corpora = [pathlib.Path(c).resolve() for c in args_of("corpus")]
jobs = int(arg("jobs", 2))
show = int(arg("show", 10))
only = arg("only")
max_failed = int(arg("max-failed", 0))
imports = "--imports" in sys.argv
clang = arg("clang")
header_macros = "--header-macros" in sys.argv
cxx23 = "--cxx23" in sys.argv
report = arg("report")


def database(corpus):
    direct = corpus / "compile_commands.json"
    if direct.exists():
        return direct
    found = sorted(corpus.glob("target/*/*/compile_commands.json"), key=lambda p: p.stat().st_mtime)
    return found[-1] if found else None


def units_of(corpus):
    db = database(corpus)
    if db is None:
        return []
    out = {}
    for u in json.loads(db.read_text()):
        f = str(u["file"])
        if not f.startswith(str(corpus) + "/") or not f.endswith((".cpp", ".cppm")):
            continue
        if any(part in f for part in ("/target/", "/.mcpp/", "/vendor/", "/.cache/", "/.payload-cache/", "/.test-scratch/")):
            continue
        if only and only not in f:
            continue
        out[f] = u
    return list(out.values())


def flags_of(unit):
    arguments = unit.get("arguments") or shlex.split(unit["command"])
    flags = []
    for i, a in enumerate(arguments):
        if a.startswith(("-D", "-U")) and len(a) > 2:
            flags.append(a)
        elif a in ("-D", "-U") and i + 1 < len(arguments):
            flags.append(a + arguments[i + 1])
        elif a.startswith("--target="):
            flags += ["--target", a.split("=", 1)[1]]
        elif imports and a.startswith(("-fmodule-file=", "-fprebuilt-module-path=")):
            flags.append(a)
    return flags


def macros_of(unit):
    """The macros the headers a unit includes define, as `clang -dM -E` prints them (what a host passes the preprocessor)."""
    found = json.loads(subprocess.run([lexdump, "--directives", unit["file"]], capture_output=True, text=True, check=True).stdout)
    if not found["includes"]:
        return None
    arguments = unit.get("arguments") or shlex.split(unit["command"])
    keep = [a for i, a in enumerate(arguments[1:], 1) if a.startswith(("-I", "-D", "-U", "-std", "-isystem", "--no-default-config", "-nostd"))
            or (i > 1 and arguments[i - 1] in ("-I", "-isystem"))]
    source = os.path.join(cache, f"{abs(hash(unit['file']))}.headers.cpp")
    pathlib.Path(source).write_text(found["text"])
    run = subprocess.run([clang, *keep, "-dM", "-E", "-x", "c++", source], capture_output=True, text=True,
                         cwd=unit["directory"] if os.path.isdir(unit.get("directory", "")) else None)
    os.unlink(source)
    if run.returncode != 0:
        return None
    own = set(found["own"])
    path = source + ".macros"
    pathlib.Path(path).write_text("\n".join(l for l in run.stdout.splitlines() if (d := re.match(r"#define (\w+)", l)) and d.group(1) not in own) + "\n")
    return path


def read(unit):
    extra = ["--cxx23"] if cxx23 else []
    macros = macros_of(unit) if header_macros and clang else None
    if macros:
        extra += ["--header-macros", macros]
    run = subprocess.run([lexdump, "--bodies", *extra, *flags_of(unit), unit["file"]], capture_output=True, text=True)
    if macros:
        os.unlink(macros)
    if run.returncode != 0 or not run.stdout.startswith("{"):
        return {"file": unit["file"], "crashed": f"exit {run.returncode}: {(run.stderr.strip().splitlines() or [''])[-1][:200]}"}
    result = json.loads(run.stdout)
    result["file"] = unit["file"]
    return result


failed_corpora = 0
summary = {}
for corpus in corpora:
    units = units_of(corpus)
    with concurrent.futures.ThreadPoolExecutor(max_workers=jobs) as pool:
        results = list(pool.map(read, units))
    crashed = [r for r in results if "crashed" in r]
    done = [r for r in results if "crashed" not in r]
    parts = sum(r["parts"] for r in done)
    failed = sum(r["failed"] for r in done)
    errors = sum(r["errors"] for r in done)
    invalid = [(r["file"], v) for r in done for v in r.get("invalid", [])]
    decisions = collections.Counter()
    for r in done:
        for key, value in r["decisions"].items():
            decisions[key] += value
    seconds = sum(r["seconds"] for r in done)
    summary[str(corpus)] = {"files": len(done), "crashed": len(crashed), "parts": parts, "failed": failed, "errors": errors, "invalid": len(invalid),
                            "decisions": dict(decisions), "seconds": seconds,
                            "speculations": sum(r["speculations"] for r in done), "rewinds": sum(r["rewinds"] for r in done)}
    print(f"{corpus.name}: {len(done)} files ({len(crashed)} crashed), {parts} parts, {failed} failed ({100 * failed / parts if parts else 0:.4f}%), "
          f"{errors} error nodes, {len(invalid)} structure problems; decided by scope {decisions['scope']}, lookup {decisions['lookup']}, feedback {decisions['feedback']}, "
          f"shape {decisions['shape']}, syntax {decisions['syntax']}; {seconds:.2f} s")
    shown = 0
    kinds = collections.Counter()
    first_of = {}
    for r in done:
        for d in r["diagnostics"]:
            message = d.split(": ", 1)[1] if ": " in d else d
            kinds[message] += 1
            first_of.setdefault(message, f"{r['file']}:{d.split(': ', 1)[0]}")
    if "--kinds" in sys.argv:
        for message, count in kinds.most_common(25):
            print(f"  {count:6d} x {message}   e.g. {first_of[message]}")
        shown = show
    for f, v in invalid[:show]:
        print(f"  {f}: structure: {v}")
    for r in crashed:
        print(f"  {r['file']}: {r['crashed']}")
    for r in done:
        for d in r["diagnostics"]:
            if shown >= show:
                break
            shown += 1
            print(f"  {r['file']}:{d}")
    if failed > max_failed or crashed or invalid:
        failed_corpora += 1
if report:
    pathlib.Path(report).write_text(json.dumps(summary, indent=1))
sys.exit(1 if failed_corpora else 0)
