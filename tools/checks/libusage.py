#!/usr/bin/env python3
"""Which facilities of a library a corpus uses, and how much (MF.6, AF.6; formerly A3.5): every name a file writes
that names a declaration of the library, counted by facility -- the data the std / std2 decision record reads.

    python3 tools/checks/libusage.py --corpus NAME=DIR [--corpus NAME=DIR ...] --probe MCXX_PROBE --resource DIR
                                     [--library std] [--jobs N] [--reference-cache DIR] [--json FILE] [--top N] [--only SUBSTRING]

    DIR          a corpus built by mcpp (its compile database under target/); its own sources are counted, not its
                 packages' or its build's
    MCXX_PROBE   `--references`: the Clang backend's references of a file -- each name written that names a
                 declaration (not a declaration itself, not implied), with the qualified name of what it names
    --reference-cache  keeps each file's references (its path's digest and modification time), the same files as
                 tools/checks/refsdiff.py keeps: either check reuses the other's

A facility is what a program asks the library for by name (a namespace written as a qualifier is not one; the
implementation's reserved names, reached through an iterator's `->` and the like, count as `std::(implementation)`): `std::vector` for `std::vector<int>::push_back`,
`std::chrono::steady_clock` for `std::chrono::steady_clock::now`, `std::ranges::sort`, `std::views::transform` --
the library's namespace, its nested namespaces (ranges, views, chrono, filesystem, ...), and the first name in
them. Printed per corpus: the facilities by references, with how many files use each; --json writes everything.
"""
import collections, concurrent.futures, hashlib, json, os, pathlib, shutil, subprocess, sys, tempfile

def arg(name, default=None):
    return sys.argv[sys.argv.index(f"--{name}") + 1] if f"--{name}" in sys.argv else default

def args(name):
    return [sys.argv[i + 1] for i, a in enumerate(sys.argv[:-1]) if a == f"--{name}"]

probe = str(pathlib.Path(arg("probe")).resolve())
resource = arg("resource")
library = arg("library", "std")
jobs = int(arg("jobs", max(1, (os.cpu_count() or 4) // 4)))
cache_dir = pathlib.Path(arg("reference-cache")) if arg("reference-cache") else None
top = int(arg("top", "40"))
only = arg("only")
corpora = [c.split("=", 1) if "=" in c else (pathlib.Path(c).name, c) for c in args("corpus")]
if not corpora or not resource:
    sys.exit(__doc__)

# The library's nested namespaces a facility is named within: std::ranges::sort is one facility, not std::ranges.
NESTED = { "std": { "ranges", "views", "chrono", "filesystem", "pmr", "this_thread", "literals", "string_literals", "chrono_literals",
                    "string_view_literals", "complex_literals", "placeholders", "execution", "numbers", "regex_constants", "rel_ops",
                    "meta", "linalg", "experimental" } }

def plain(component):
    # A partial specialization's name carries its arguments: std::function<R (Args...)> is std::function.
    return component.split("<", 1)[0]

def facility(target):
    parts = [plain(p) for p in target.split("::")]
    if parts[0] != library or len(parts) < 2:
        return None
    out = [parts[0]]
    for p in parts[1:]:
        out.append(p)
        if p not in NESTED.get(library, set()):
            break
    return "::".join(out)

def references(database, unit, probe_cache):
    path = unit["file"]
    key = None
    if cache_dir:
        key = cache_dir / f"refs-{hashlib.sha1(path.encode()).hexdigest()}-{int(os.stat(path).st_mtime)}.json"
        if key.exists():
            return json.loads(key.read_text())
    run = subprocess.run([probe, "--db", str(database.parent), "--resource", resource, "--cache", probe_cache, "--references", path],
                         capture_output=True, text=True)
    refs = None
    for line in run.stdout.splitlines():
        if line.startswith('{"references"'):
            refs = json.loads(line)["references"]
    if refs is None:
        return None
    if key:
        key.parent.mkdir(parents=True, exist_ok=True)
        key.write_text(json.dumps(refs))
    return refs

report = {}
for name, directory in corpora:
    corpus = pathlib.Path(directory).resolve()
    database = sorted(corpus.glob("target/*/*/compile_commands.json"), key=lambda p: p.stat().st_mtime)[-1]
    units = [u for u in json.loads(database.read_text()) if str(u["file"]).startswith(str(corpus)) and "/target/" not in u["file"]
             and "/.mcpp/" not in u["file"]   # a checkout's own package cache: its dependencies' sources
             and u["file"].endswith((".cpp", ".cppm", ".cc", ".cxx", ".ixx"))]
    if only:
        units = [u for u in units if only in u["file"]]
    probe_cache = tempfile.mkdtemp(prefix="mcxx-libusage-")
    counts, files, kinds = collections.Counter(), collections.defaultdict(set), collections.defaultdict(collections.Counter)
    failed = []
    try:
        with concurrent.futures.ThreadPoolExecutor(jobs) as pool:
            for unit, refs in zip(units, pool.map(lambda u: references(database, u, probe_cache), units)):
                if refs is None:
                    failed.append(unit["file"])
                    continue
                for r in refs:
                    f = facility(r["target"])
                    if f is None or r["kind"] == "namespace":   # std::chrono in std::chrono::seconds is a qualifier
                        continue
                    if f.split("::")[-1].startswith("__"):     # the implementation's own names (std::__wrap_iter), reached
                        f = f"{library}::(implementation)"      # through what the file wrote (an iterator's ->)
                    counts[f] += 1
                    files[f].add(unit["file"])
                    kinds[f][r["kind"]] += 1
    finally:
        shutil.rmtree(probe_cache, True)
    total = sum(counts.values())
    report[name] = { "files": len(units), "not-parsed": failed, "references": total,
                     "facilities": [ { "facility": f, "references": n, "files": len(files[f]), "kinds": dict(kinds[f]) }
                                     for f, n in counts.most_common() ] }
    print(f"\n{name}: {len(units)} files ({len(failed)} not parsed), {total} references to {library}, {len(counts)} facilities")
    print(f"| facility | references | share | files |\n|---|---|---|---|")
    for f, n in counts.most_common(top):
        print(f"| `{f}` | {n} | {100.0 * n / max(1, total):.1f}% | {len(files[f])} |")

if arg("json"):
    pathlib.Path(arg("json")).write_text(json.dumps({ "library": library, "corpora": report }, indent=1) + "\n")
sys.exit(1 if any(r["not-parsed"] for r in report.values()) else 0)
