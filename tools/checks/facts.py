#!/usr/bin/env python3
"""T1 facts against Clang's AST (A0.4.3): for every source file of a built corpus, the declarations MSA
reports are, kind by kind, the declarations Clang's AST holds.

    python3 tools/checks/facts.py --corpus DIR --probe MCXX_PROBE --resource DIR [--jobs N] [--only SUBSTRING]

    DIR         a corpus built by mcpp (compile_commands.json under target/); its own sources are checked,
                not the packages it depends on
    MCXX_PROBE  tools/probe: `--facts` prints MSA's facts (MC3 JSON), `--census` the file's declarations
                counted straight off Clang's AST by a clang::RecursiveASTVisitor with Clang's defaults (no
                implicit code, no template instantiations), by Clang's own kind names
    --resource  the resource directory (the mcxx payload's lib/clang/23)

The two share only what makes a declaration the file's (MC3 §4.2): its location is in the file, it is not
implicit, a parameter belongs to a function. Everything else is independent: MSA's collector and its kind
mapping, the facts' JSON form, against Clang's traversal. Classes, structs and unions compare together
(Clang's kind is CXXRecord for all three). Exits non-zero when any file differs.
"""
import collections, concurrent.futures, json, os, pathlib, subprocess, sys, tempfile

def arg(name, default=None):
    return sys.argv[sys.argv.index(f"--{name}") + 1] if f"--{name}" in sys.argv else default

corpus = pathlib.Path(arg("corpus")).resolve()
probe = str(pathlib.Path(arg("probe")).resolve())
resource = arg("resource")
jobs = int(arg("jobs", max(1, (os.cpu_count() or 4) // 4)))
only = arg("only")
database = sorted(corpus.glob("target/*/*/compile_commands.json"), key=lambda p: p.stat().st_mtime)[-1]
units = [u for u in json.loads(database.read_text()) if str(u["file"]).startswith(str(corpus)) and u["file"].endswith((".cpp", ".cppm"))]
if only:
    units = [u for u in units if only in u["file"]]
cache = tempfile.mkdtemp(prefix="mcxx-facts-")

# Clang's kind names, as MSA names its kinds (MC3 §4.2); the other kinds are outside T1's declarations.
CLANG = {"Var": "variable", "Decomposition": "variable", "ParmVar": "parameter", "Field": "field", "Function": "function",
         "CXXMethod": "method", "CXXConstructor": "constructor", "CXXDestructor": "destructor", "CXXConversion": "conversion",
         "Typedef": "type-alias", "TypeAlias": "type-alias", "Enum": "enum", "Namespace": "namespace",
         "CXXRecord": "record", "Record": "record", "ClassTemplateSpecialization": "record"}
RECORDS = {"class", "struct", "union"}


def check(unit):
    run = subprocess.run([probe, "--db", str(database.parent), "--resource", resource, "--cache", cache, "--facts", "--census", unit["file"]],
                         capture_output=True, text=True)
    lines = [json.loads(l) for l in run.stdout.splitlines() if l.startswith("{")]
    facts = next((l for l in lines if "declarations" in l), None)
    census = next((l["census"] for l in lines if "census" in l), None)
    if facts is None or census is None:
        return unit["file"], None, None, run.stderr[-300:]
    msa = collections.Counter("record" if d["kind"] in RECORDS else d["kind"] for d in facts["declarations"])
    ast = collections.Counter()
    for kind, n in census.items():
        if kind in CLANG:
            ast[CLANG[kind]] += n
    return unit["file"], msa, ast, ""


with concurrent.futures.ThreadPoolExecutor(max_workers=jobs) as pool:
    results = list(pool.map(check, units))
bad = [r for r in results if r[1] is None or r[1] != r[2]]
total_m = sum((r[1] for r in results if r[1]), collections.Counter())
total_a = sum((r[2] for r in results if r[2]), collections.Counter())
print(f"{len(results)} files of {corpus.name}: {len(bad)} differ; declarations {sum(total_m.values())} by MSA, {sum(total_a.values())} in Clang's AST")
for kind in sorted(set(total_a) | set(total_m)):
    print(f"  {kind:<12} MSA {total_m[kind]:>6}  AST {total_a[kind]:>6}")
for f, m, a, why in bad[:15]:
    if m is None:
        print(f"  cannot compare {f}: {why}")
    else:
        print(f"  {f}: (MSA, AST) { {k: (m[k], a[k]) for k in set(a) | set(m) if a[k] != m[k]} }")
sys.exit(1 if bad else 0)
