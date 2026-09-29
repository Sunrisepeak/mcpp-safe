#!/usr/bin/env python3
"""MC++'s own front end against the Clang backend, declaration by declaration (M2.1: A2.1.1, A2.1.2).

    python3 tools/checks/declsdiff.py --corpus DIR --probe MCXX_PROBE --lexdump MCXX_LEXDUMP --resource DIR
                                      [--jobs N] [--reference-cache DIR] [--only SUBSTRING] [--report FILE]
                                      [--min MEMBER=PCT]...

For every source file of a built corpus (its compile database; its own files, not its dependencies'):
the Clang backend's T1 facts (`mcxx-probe --facts`, every kind, declaration types too) and MC++'s own
front end's (`mcxx-lexdump --facts`, the command's -D/-U and target). Declarations are matched by where
their name is and their kind; for each matched pair every T1 member is compared -- qualified name,
container, exported, the flags (pointer, c-array, union, c-variadic, local), the type as text and the
templates it names. Printed: how many of Clang's declarations the front end has, and per member the share
that agrees, with examples of what does not. `--reference-cache` keeps Clang's side per file (its path's
digest and modification time), since it costs a parse. `--min matched=99.9 --min qualified-name=99.9`:
fail when a share falls below it (a regression gate).
"""
import atexit, collections, concurrent.futures, hashlib, json, os, pathlib, shlex, shutil, subprocess, sys, tempfile

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
probe_cache = tempfile.mkdtemp(prefix="mcxx-declsdiff-")
atexit.register(shutil.rmtree, probe_cache, True)   # the probe's cache of this run only

MEMBERS = ["qualified-name", "container", "exported", "pointer", "c-array", "union", "c-variadic", "local", "type", "templates"]


def clang_side(unit):
    key = None
    if cache_dir:
        key = cache_dir / f"facts-{hashlib.sha1(unit['file'].encode()).hexdigest()}-{int(os.stat(unit['file']).st_mtime)}.json"
        if key.exists():
            return json.loads(key.read_text())
    run = subprocess.run([probe, "--db", str(database.parent), "--resource", resource, "--cache", probe_cache, "--facts", unit["file"]],
                         capture_output=True, text=True)
    facts = next((json.loads(l) for l in run.stdout.splitlines() if l.startswith("{") and '"declarations"' in l), None)
    if facts is not None and key is not None:
        key.parent.mkdir(parents=True, exist_ok=True)
        key.write_text(json.dumps(facts))
    return facts


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
    run = subprocess.run([lexdump, "--facts", *flags, unit["file"]], capture_output=True, text=True)
    return json.loads(run.stdout) if run.returncode == 0 and run.stdout.startswith("{") else None


def key(d):
    return (d["name"]["begin"]["line"], d["name"]["begin"]["column"], d["kind"])


def compare(unit):
    theirs, ours = clang_side(unit), own_side(unit)
    if theirs is None or ours is None:
        return unit["file"], None
    t = {key(d): d for d in theirs["declarations"]}
    o = {key(d): d for d in ours["declarations"]}
    counts = collections.Counter()
    wrong = collections.defaultdict(list)
    counts["clang"] = len(t)
    counts["own"] = len(o)
    for k, d in t.items():
        mine = o.get(k)
        if mine is None:
            wrong["missing"].append(f"{d['kind']} {d['qualified-name']}")
            continue
        counts["matched"] += 1
        for m in MEMBERS:
            if mine.get(m) == d.get(m):
                counts[m] += 1
            else:
                wrong[m].append(f"{d['qualified-name']}: {d.get(m)!r} vs {mine.get(m)!r}")
    for k, d in o.items():
        if k not in t:
            wrong["extra"].append(f"{d['kind']} {d['qualified-name']}")
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
    for m, items in wrong.items():
        examples[m].extend(f"{pathlib.Path(f).name}: {x}" for x in items)
matched = total["matched"]
print(f"{len(results) - len(failed)} files of {corpus.name}; Clang's declarations {total['clang']}, the front end's {total['own']}, "
      f"matched {matched} ({100 * matched / max(1, total['clang']):.2f}% of Clang's)")
summary = {"files": len(results) - len(failed), "clang": total["clang"], "own": total["own"], "matched": matched, "members": {}}
for m in MEMBERS:
    share = 100 * total[m] / max(1, matched)
    summary["members"][m] = round(share, 3)
    print(f"  {m:<15} {share:7.3f}%  ({matched - total[m]} differ)")
    for x in examples[m][:3]:
        print(f"      {x}")
for m in ("missing", "extra"):
    print(f"  {m}: {len(examples[m])}")
    for x in examples[m][:5]:
        print(f"      {x}")
if failed:
    print(f"  not compared: {len(failed)} ({', '.join(pathlib.Path(f).name for f in failed[:5])})")
if report:
    pathlib.Path(report).write_text(json.dumps({**summary, "examples": {k: v[:200] for k, v in examples.items()}}, indent=1))
problems = []
for i, a in enumerate(sys.argv):
    if a != "--min" or i + 1 >= len(sys.argv):
        continue
    member, floor = sys.argv[i + 1].split("=")
    share = 100 * matched / max(1, total["clang"]) if member == "matched" else summary["members"].get(member, 0)
    if share < float(floor):
        problems.append(f"{member} {share:.3f}% below {floor}%")
if failed:
    problems.append(f"{len(failed)} files not compared")
for p in problems:
    print(f"FAIL  {p}")
sys.exit(1 if problems else 0)
