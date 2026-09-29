#!/usr/bin/env python3
"""MC++'s own parser against Clang (A1.7.1, A1.7.2): for every source file of a built corpus, the outline
mcxx.frontend:syntax gives -- each declaration's kind, name, name range and whole range -- against the one
the Clang backend gives (Unit::symbols, what an editor's document outline shows), and whether the parser
had to skip anything (a parse failure).

    python3 tools/checks/syntaxdiff.py --corpus DIR --probe MCXX_PROBE --lexdump MCXX_LEXDUMP --resource DIR
                                       [--clang CLANGXX --header-macros] [--reference-cache DIR] [--report FILE] [--jobs N] [--show N] [--only SUBSTRING]

    DIR          a corpus built by mcpp (compile_commands.json under target/, or --db FILE); its own sources are checked,
                 not its packages' or its build's
    MCXX_PROBE   `--symbols`: Clang's outline, through the backend
    MCXX_LEXDUMP `--syntax`: MC++'s parser's, with the command's -D and -U
    --header-macros  the headers' macros from Clang (--clang: the toolchain's clang++), as a host passes
                 them (PreprocessOptions::header_macros); without it, a header's macro is left unexpanded
    --reference-cache  keeps Clang's outline of each file (by path and modification time) between runs;
                 with --cached-only, a file without one kept is not compared (not parsed with Clang again)

A symbol agrees when both give it with the same kind, name, name range and whole range. The agreement
rate is agreeing symbols over all symbols either gives. Exits non-zero when the rate is under 99.9% or a
file does not parse.
"""
import collections, concurrent.futures, hashlib, json, os, pathlib, re, subprocess, sys, tempfile

def arg(name, default=None):
    return sys.argv[sys.argv.index(f"--{name}") + 1] if f"--{name}" in sys.argv else default

corpus = pathlib.Path(arg("corpus")).resolve()
probe = str(pathlib.Path(arg("probe")).resolve())
lexdump = str(pathlib.Path(arg("lexdump")).resolve())
resource = arg("resource")
clang = arg("clang")
header_macros = "--header-macros" in sys.argv
jobs = int(arg("jobs", max(1, (os.cpu_count() or 4) // 4)))
show = int(arg("show", 20))
only = arg("only")
reference_cache = arg("reference-cache")
report = arg("report")
cached_only = "--cached-only" in sys.argv
database = pathlib.Path(arg("db")).resolve() if arg("db") else sorted(corpus.glob("target/*/*/compile_commands.json"), key=lambda p: p.stat().st_mtime)[-1]
units = [u for u in json.loads(database.read_text()) if str(u["file"]).startswith(str(corpus) + "/") and u["file"].endswith((".cpp", ".cppm"))
         and "/target/" not in u["file"] and "/.mcpp/" not in u["file"]]
units = list({u["file"]: u for u in units}.values())
if only:
    units = [u for u in units if only in u["file"]]
cache = tempfile.mkdtemp(prefix="mcxx-syntaxdiff-")


def defines(arguments):
    out, i = [], 0
    while i < len(arguments):
        a = arguments[i]
        if a in ("-D", "-U") and i + 1 < len(arguments):
            out.append(a + arguments[i + 1])
            i += 2
            continue
        if a.startswith(("-D", "-U")):
            out.append(a)
        i += 1
    return out


def macros_of(unit, arguments):
    found = json.loads(subprocess.run([lexdump, "--directives", unit["file"]], capture_output=True, text=True, check=True).stdout)
    if not found["includes"]:
        return None
    keep = [a for i, a in enumerate(arguments[1:], 1) if a.startswith(("-I", "-D", "-U", "-std", "-isystem", "--no-default-config", "-nostd"))
            or (i > 1 and arguments[i - 1] in ("-I", "-isystem"))]
    source = os.path.join(cache, f"{abs(hash(unit['file']))}.headers.cpp")
    pathlib.Path(source).write_text(found["text"])
    run = subprocess.run([clang, *keep, "-dM", "-E", "-x", "c++", source], capture_output=True, text=True,
                         cwd=unit["directory"] if os.path.isdir(unit.get("directory", "")) else str(corpus))
    os.unlink(source)
    if run.returncode != 0:
        return None
    own = set(found["own"])
    path = source + ".macros"
    pathlib.Path(path).write_text("\n".join(l for l in run.stdout.splitlines() if (d := re.match(r"#define (\w+)", l)) and d.group(1) not in own) + "\n")
    return path


def key(s):
    return (s["kind"], s["name"].replace(" ", ""), tuple(s["selection"]))


def check(unit):
    arguments = unit.get("arguments") or unit["command"].split()
    kept = None
    if reference_cache:
        stamp = f"{hashlib.sha1(unit['file'].encode()).hexdigest()}-{int(os.stat(unit['file']).st_mtime)}.json"
        kept = pathlib.Path(reference_cache) / stamp
    if kept and kept.exists():
        clang_symbols, theirs = json.loads(kept.read_text()), None
    elif cached_only:
        return {"file": unit["file"], "failed": "no outline of Clang's kept (--cached-only)"}
    else:
        theirs = subprocess.run([probe, "--db", str(database.parent), "--resource", resource, "--cache", cache, "--symbols", unit["file"]],
                                capture_output=True, text=True)
        clang_symbols = next((json.loads(l)["symbols"] for l in theirs.stdout.splitlines() if l.startswith('{"symbols"')), None)
        if kept and clang_symbols is not None:
            kept.parent.mkdir(parents=True, exist_ok=True)
            kept.write_text(json.dumps(clang_symbols))
    extra = []
    if header_macros and clang:
        path = macros_of(unit, arguments)
        if path:
            extra = ["--header-macros", path]
    ours = subprocess.run([lexdump, "--syntax", *defines(arguments), *extra, unit["file"]], capture_output=True, text=True)
    if extra:
        os.unlink(extra[1])
    if clang_symbols is None or ours.returncode != 0:
        return {"file": unit["file"], "failed": ((theirs.stderr if theirs else "") + ours.stderr)[-400:]}
    mine = json.loads(ours.stdout)
    a = {key(s): s for s in mine["symbols"]}
    b = {key(s): s for s in clang_symbols}
    agree = [k for k in b if k in a and a[k]["range"] == b[k]["range"]]
    return {"file": unit["file"], "clang": len(b), "ours": len(a), "agree": len(agree), "diagnostics": mine["diagnostics"],
            "seconds": mine["seconds"],
            "missing": [b[k] for k in b if k not in a],
            "extra": [a[k] for k in a if k not in b],
            "range": [(a[k], b[k]) for k in b if k in a and a[k]["range"] != b[k]["range"]]}


with concurrent.futures.ThreadPoolExecutor(max_workers=jobs) as pool:
    results = list(pool.map(check, units))

failed = [r for r in results if "failed" in r]
done = [r for r in results if "failed" not in r]
union = sum(r["clang"] + len(r["extra"]) for r in done)
agree = sum(r["agree"] for r in done)
named = sum(r["clang"] - len(r["missing"]) for r in done)
unparsed = [r for r in done if r["diagnostics"]]
shown = 0
for r in done:
    for what, items in (("missing", r["missing"]), ("extra", r["extra"]), ("range", r["range"])):
        for item in items:
            if shown >= show:
                break
            shown += 1
            if what == "range":
                print(f"{r['file']}: {item[1]['kind']} {item[1]['name']} whole range ours {item[0]['range']} clang {item[1]['range']}")
            else:
                print(f"{r['file']}: {what} {item['kind']} {item['name']} at {item['selection']} range {item['range']}")
for r in unparsed[:show]:
    print(f"{r['file']}: parse diagnostics: {r['diagnostics'][:3]}")
for r in failed[:show]:
    print(f"{r['file']}: not compared: {r['failed']}")
rate = agree / union if union else 0.0
if report:
    pathlib.Path(report).write_text(json.dumps(results))
print(f"syntaxdiff: {len(done)} files ({len(failed)} not compared, {len(unparsed)} with parse diagnostics); "
      f"{union} symbols, {agree} agree ({100 * rate:.3f}%), {named} of Clang's {sum(r['clang'] for r in done)} with the same name range; "
      f"parse time {sum(r['seconds'] for r in done):.3f} s")
sys.exit(0 if rate >= 0.999 and not unparsed and not failed and done else 1)
