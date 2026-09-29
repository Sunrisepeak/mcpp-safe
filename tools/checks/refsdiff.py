#!/usr/bin/env python3
"""MC++'s own name lookup against the Clang backend's, name by name (M2.1: A2.1.1).

    python3 tools/checks/refsdiff.py --corpus DIR --probe MCXX_PROBE --lexdump MCXX_LEXDUMP --resource DIR
                                     [--jobs N] [--reference-cache DIR] [--only SUBSTRING] [--report FILE]
                                     [--min-in-file PCT] [--max-other PCT]

For every source file of a built corpus (its compile database; its own files): the names the file writes
that name a declaration, with what each names -- the Clang backend's (`mcxx-probe --references`: its
occurrences that are references, not declarations, not implied) and MC++'s own front end's
(`mcxx-lexdump --references`, mcxx.frontend:lookup). A Clang reference counts when the name written there
is the name of what it names (a contextual `operator bool` at a variable's name, a constructor at a class's
name, is not what the name says). Printed, split by where the target is declared -- in the file, or
elsewhere (another module, a header: what M2.2's imported interfaces answer) -- how many of Clang's the
front end resolves to the same target (qualified name and kind; a local's also by where it is declared),
how many it resolves to another (wrong: the number that must stay near zero), and how many it leaves out.
`--reference-cache` keeps Clang's side per file (its path's digest and modification time).
`--min-in-file`: fail when fewer of the file's own names resolve the same; `--max-other`: fail when more
resolve to another target (either place) -- a wrong answer is worse than none.
"""
import collections, concurrent.futures, hashlib, json, os, pathlib, re, shlex, subprocess, sys, tempfile

def arg(name, default=None):
    return sys.argv[sys.argv.index(f"--{name}") + 1] if f"--{name}" in sys.argv else default

corpus = pathlib.Path(arg("corpus")).resolve()
probe = str(pathlib.Path(arg("probe")).resolve())
lexdump = str(pathlib.Path(arg("lexdump")).resolve())
resource = arg("resource")
jobs = int(arg("jobs", max(1, (os.cpu_count() or 4) // 4)))
only = arg("only")
cache_dir = pathlib.Path(arg("reference-cache")) if arg("reference-cache") else None
report = arg("report")
database = sorted(corpus.glob("target/*/*/compile_commands.json"), key=lambda p: p.stat().st_mtime)[-1]
units = [u for u in json.loads(database.read_text()) if str(u["file"]).startswith(str(corpus)) and "/target/" not in u["file"]
         and u["file"].endswith((".cpp", ".cppm"))]
if only:
    units = [u for u in units if only in u["file"]]
probe_cache = tempfile.mkdtemp(prefix="mcxx-refsdiff-")
INLINE = re.compile(r"::__\w*\d\w*(?=::|$)")   # libc++'s std::__1:: and its like: MC3 names leave inline namespaces out


def plain(name):
    return INLINE.sub("", name)


def clang_side(unit):
    key = None
    if cache_dir:
        key = cache_dir / f"refs-{hashlib.sha1(unit['file'].encode()).hexdigest()}-{int(os.stat(unit['file']).st_mtime)}.json"
        if key.exists():
            return json.loads(key.read_text())
    run = subprocess.run([probe, "--db", str(database.parent), "--resource", resource, "--cache", probe_cache, "--references", unit["file"]],
                         capture_output=True, text=True)
    refs = next((json.loads(l) for l in run.stdout.splitlines() if l.startswith("{") and '"references"' in l), None)
    if refs is not None and key is not None:
        key.parent.mkdir(parents=True, exist_ok=True)
        key.write_text(json.dumps(refs))
    return refs


def own_side(unit):
    args = unit.get("arguments") or shlex.split(unit["command"])
    flags = []
    for i, a in enumerate(args):
        if a.startswith(("-D", "-U")) and len(a) > 2:
            flags.append(a)
        elif a in ("-D", "-U") and i + 1 < len(args):
            flags.append(a + args[i + 1])
        elif a.startswith("--target="):
            flags += ["--target", a.split("=", 1)[1]]
        elif a.startswith(("-fmodule-file=", "-fprebuilt-module-path=")):
            flags.append(a)   # where the imports' BMIs, and so their MC2 interfaces, are (M2.2)
    run = subprocess.run([lexdump, "--references", *flags, unit["file"]], capture_output=True, text=True)
    return json.loads(run.stdout) if run.returncode == 0 and run.stdout.startswith("{") else None


def compare(unit):
    theirs, ours = clang_side(unit), own_side(unit)
    if theirs is None or ours is None:
        return unit["file"], None
    counts = collections.Counter()
    wrong = collections.defaultdict(list)
    mine = {tuple(r["range"][:2]): r for r in ours["references"]}
    lines = pathlib.Path(unit["file"]).read_bytes().split(b"\n")
    by_place = collections.defaultdict(list)
    for r in theirs["references"]:
        line, begin, end_line, end = r["range"]
        written = lines[line][begin:end].decode(errors="replace") if line == end_line and line < len(lines) else ""
        # What the name written there names: the target's own name is the name. And not a reference
        # at its target's own declaration (a condition variable's implied use, which Clang does not
        # mark implicit).
        if plain(r["target"]).rsplit("::", 1)[-1] != written:
            continue
        if r.get("declaration") is not None and r["declaration"] == r["range"][:2]:
            continue
        by_place[tuple(r["range"][:2])].append(r)
    for place, rs in by_place.items():
        where = "in-file" if any(r.get("declaration") is not None for r in rs) else "elsewhere"
        counts[f"{where}:clang"] += 1
        m = mine.get(place)
        if m is None:
            counts[f"{where}:left-out"] += 1
            wrong[f"{where}:left-out"].append(f"{place[0] + 1}:{place[1] + 1} {plain(rs[0]['target'])} ({rs[0]['kind']})")
            continue
        # The same entity: its qualified name and kind; a local (named by its name alone) also by where
        # it is declared. Which redeclaration a namespace's or a class's member is "declared" at is
        # not what lookup decides.
        same = any(plain(r["target"]) == m["target"] and r["kind"] == m["kind"] and
                   ("::" in m["target"] or r.get("declaration") is None or m.get("declaration") is None or r["declaration"] == m["declaration"])
                   for r in rs)
        if same:
            counts[f"{where}:same"] += 1
        else:
            counts[f"{where}:other"] += 1
            wrong[f"{where}:other"].append(f"{place[0] + 1}:{place[1] + 1} {m['name']}: Clang {plain(rs[0]['target'])} ({rs[0]['kind']}) "
                                           f"vs {m['target']} ({m['kind']})")
    for place, m in mine.items():
        if place not in by_place:
            counts["extra"] += 1
            wrong["extra"].append(f"{place[0] + 1}:{place[1] + 1} {m['name']} -> {m['target']} ({m['kind']})")
    return unit["file"], (counts, wrong)


with concurrent.futures.ThreadPoolExecutor(max_workers=jobs) as pool:
    results = list(pool.map(compare, units))
total = collections.Counter()
examples = collections.defaultdict(list)
failed = [f for f, r in results if r is None]
for f, r in results:
    if r is None:
        continue
    counts, wrong = r
    total.update(counts)
    for k, items in wrong.items():
        examples[k].extend(f"{pathlib.Path(f).name}:{x}" for x in items)
print(f"{len(results) - len(failed)} files of {corpus.name}")
summary = {"files": len(results) - len(failed), "counts": dict(total)}
for where in ("in-file", "elsewhere"):
    n = total[f"{where}:clang"]
    pct = lambda k: 100 * total[f"{where}:{k}"] / max(1, n)
    print(f"  {where:<9} Clang's references {n}: the same {total[f'{where}:same']} ({pct('same'):.2f}%), "
          f"another target {total[f'{where}:other']} ({pct('other'):.2f}%), left out {total[f'{where}:left-out']} ({pct('left-out'):.2f}%)")
    for k in ("other", "left-out"):
        for x in examples[f"{where}:{k}"][:6]:
            print(f"      {k}: {x}")
print(f"  names the front end resolves that Clang reports nothing at: {total['extra']}")
for x in examples["extra"][:6]:
    print(f"      {x}")
if failed:
    print(f"  not compared: {len(failed)} ({', '.join(pathlib.Path(f).name for f in failed[:5])})")
if report:
    pathlib.Path(report).write_text(json.dumps({**summary, "examples": {k: v[:300] for k, v in examples.items()}}, indent=1))
problems = []
if arg("min-in-file") and 100 * total["in-file:same"] / max(1, total["in-file:clang"]) < float(arg("min-in-file")):
    problems.append(f"the file's own names resolved the same below {arg('min-in-file')}%")
if arg("max-other"):
    other = 100 * (total["in-file:other"] + total["elsewhere:other"]) / max(1, total["in-file:clang"] + total["elsewhere:clang"])
    if other > float(arg("max-other")):
        problems.append(f"names resolved to another target above {arg('max-other')}% ({other:.3f}%)")
if failed:
    problems.append(f"{len(failed)} files not compared")
for p in problems:
    print(f"FAIL  {p}")
sys.exit(1 if problems else 0)
