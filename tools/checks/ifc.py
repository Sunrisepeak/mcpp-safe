#!/usr/bin/env python3
"""MC2 v1 (M1.1): the .ifc MC++ writes beside every BMI, checked three ways.

    python3 tools/checks/ifc.py --mcxx MCXX --probe MCXX_PROBE [--corpus DIR --resource DIR] [--jobs N] [--work DIR]

    MCXX        the mcxx driver: it compiles conformance/ifc/dialect (A1.1.4) and lists its catalog
    MCXX_PROBE  tools/probe: `--read-ifc` prints an .ifc as MC2 reads it; `--ifc X.ifc FILE` compares a parse
                of FILE with it, item by item
    --corpus    a corpus built by mcpp with mcxx as its toolchain (tools/checks/selfhost.py): its build's
                every BMI has its .ifc (A1.1.1); ifc-printer reads every one of them without an error
                (A1.1.2); the declarations read back from each are those a parse of its source says are not
                local, field by field (A1.1.3)
    --resource  the resource directory (the mcxx payload's lib/clang/23), for the probe's parses
    --work      where ifc-printer is built and the fixture compiled (default ~/.cache/mcxx-checks/ifc)

ifc-printer is the IFC SDK's own reader (microsoft/ifc 0.43.5, from this repository's index/microsoft),
built here from its sources: it validates the file (signature, format version, content hash) and loads
every declaration reachable from the global scope, as a consumer of IFC would. Exits non-zero when any
check fails.
"""
import concurrent.futures, json, os, pathlib, shutil, subprocess, sys, tempfile

def arg(name, default=None):
    return sys.argv[sys.argv.index(f"--{name}") + 1] if f"--{name}" in sys.argv else default

repo = pathlib.Path(__file__).resolve().parents[2]
mcxx = str(pathlib.Path(arg("mcxx")).resolve())
probe = str(pathlib.Path(arg("probe")).resolve())
corpus = pathlib.Path(arg("corpus")).resolve() if arg("corpus") else None
resource = arg("resource")
jobs = int(arg("jobs", max(1, (os.cpu_count() or 4) // 4)))
work = pathlib.Path(arg("work", pathlib.Path.home() / ".cache/mcxx-checks/ifc")).resolve()
failures = []


def check(label, ok, detail=""):
    print(f"{'PASS' if ok else 'FAIL'}  {label}" + (f"\n      {detail}" if detail and not ok else ""))
    if not ok:
        failures.append(label)


def build_printer():
    """ifc-printer from the SDK's own sources: a package that depends on microsoft.ifc-sdk and whose main is
    the SDK's (its printer is in the library)."""
    host = work / "ifc-printer"
    (host / "src").mkdir(parents=True, exist_ok=True)
    (host / "mcpp.toml").write_text(
        '[package]\nname = "ifc-printer-host"\nversion = "0.1.0"\n\n'
        '[targets.ifc-printer]\nkind = "bin"\nmain = "src/main.cpp"\n\n'
        '[dependencies]\nopenkal-llvm-runtime = "0.15.2"\n\n'
        '[dependencies.microsoft]\nifc-sdk = "0.43.5"\n\n'
        f'[indices]\nmicrosoft = {{ path = "{repo / "index/microsoft"}" }}\n')
    # The SDK's sources are where mcpp unpacks a package's dependencies, beside its manifest.
    (host / "src/main.cpp").write_text(
        '#include "../.mcpp/.xlings/data/xpkgs/microsoft-x-ifc-sdk/0.43.5/ifc-0.43.5/src/ifc-printer/main.cxx"\n')
    run = subprocess.run(["mcpp", "build"], cwd=host, capture_output=True, text=True)
    found = sorted(host.glob("target/*/*/bin/ifc-printer"), key=lambda p: p.stat().st_mtime)
    if run.returncode != 0 or not found:
        sys.exit(f"cannot build ifc-printer:\n{(run.stdout + run.stderr)[-2000:]}")
    return str(found[-1])


def printer_reads(printer, ifc):
    run = subprocess.run([printer, str(ifc)], capture_output=True, text=True)
    return run.returncode == 0 and not run.stderr.strip(), (run.stderr or run.stdout)[-300:]


def read_ifc(path):
    run = subprocess.run([probe, "--read-ifc", str(path)], capture_output=True, text=True)
    return json.loads(run.stdout) if run.returncode == 0 else None


printer = build_printer()

# A1.1.4: the dialect fixture, compiled as a build would (a partition before its primary interface).
fixture = work / "dialect"
shutil.rmtree(fixture, ignore_errors=True)
shutil.copytree(repo / "conformance/ifc/dialect", fixture)
src = fixture / "src"
compiles = [
    ["--precompile", "dialect-part.cppm", "-o", "dialect-part.pcm"],
    ["-fmodule-file=dialect:part=dialect-part.pcm", "--precompile", "dialect.cppm", "-o", "dialect.pcm"],
    ["-fmodule-output=dialect.legacy.pcm", "-c", "legacy.cppm", "-o", "legacy.o"],
]
for args in compiles:
    run = subprocess.run([mcxx, "c++", "-std=c++23", *args], cwd=src, capture_output=True, text=True)
    check(f"mcxx compiles {args[-3] if args[-2] == '-o' else args[0]} of the dialect fixture", run.returncode == 0, run.stderr[-500:])
catalog = json.loads(subprocess.run([mcxx, "features", "--json"], capture_output=True, text=True, check=True).stdout)
feature_ids = {f["id"] for f in catalog.get("features", [])}
expected = json.loads((fixture / "expected.json").read_text())
for ifc, module in (("dialect.ifc", "dialect"), ("dialect-part.ifc", "dialect:part"), ("dialect.legacy.ifc", "dialect.legacy")):
    path = src / ifc
    read = read_ifc(path) if path.exists() else None
    want = expected[module]
    got = read["dialect"] if read else {}
    problems = []
    if not read:
        problems.append(f"{ifc} missing or unreadable")
    else:
        if read["module"] != module:
            problems.append(f"module {read['module']}")
        if got["profiles"] != want["profiles"]:
            problems.append(f"profiles {got['profiles']}")
        problems += [f"{k} = {got['features'].get(k)}" for k, v in want["features"].items() if got["features"].get(k) != v]
        if set(got["features"]) != feature_ids:
            problems.append(f"features {sorted(set(got['features']) ^ feature_ids)} not the catalog's")
        if got["namespaces"] != want["namespaces"]:
            problems.append(f"namespaces {got['namespaces']}")
    check(f"A1.1.4 {module}: its .ifc says the profile, every feature's level for the module and the namespaces' levels "
          f"({len(got.get('features', {}))} features)", not problems, "; ".join(problems))
    if path.exists():
        ok, why = printer_reads(printer, path)
        check(f"ifc-printer reads the fixture's {ifc}", ok, why)

# A1.1.1 - A1.1.3 over a corpus built with mcxx.
if corpus:
    database = sorted(corpus.glob("target/*/*/compile_commands.json"), key=lambda p: p.stat().st_mtime)[-1]
    # The module units and their BMIs, from the build's own edges (the compile database leaves out
    # -fmodule-output): `build ... : cxx_module SOURCE ...` with its `bmi_out`.
    units = []
    source = None
    for line in (database.parent / "build.ninja").read_text().splitlines():
        if line.startswith("build "):
            rule = line.split(" : ", 1)[1].split() if " : " in line else []
            source = rule[1] if len(rule) > 1 and rule[0] == "cxx_module" else None
        elif source and line.strip().startswith("bmi_out = "):
            bmi = database.parent / line.split("=", 1)[1].strip()
            units.append((source, bmi, bmi.with_suffix(".ifc")))
            source = None
    own = [u for u in units if u[0].startswith(str(corpus)) and "/target/" not in u[0]]
    missing = [f for f, _, ifc in own if not ifc.exists()]
    check(f"A1.1.1 every interface unit of {corpus.name} has its .ifc beside its BMI: {len(own) - len(missing)}/{len(own)}",
          not missing and bool(own), f"missing: {missing[:5]}")
    # A dependency's BMI that mcpp restores from its build cache was not compiled in this build, and the
    # cache keeps the BMI, not what is beside it: counted, not required.
    bmis = sorted(database.parent.glob("pcm.cache/*.pcm"))
    print(f"      every BMI in the build: {sum(1 for b in bmis if b.with_suffix('.ifc').exists())}/{len(bmis)} have an .ifc "
          f"(without: {', '.join(b.stem for b in bmis if not b.with_suffix('.ifc').exists())[:300] or 'none'})")

    all_ifc = sorted({ifc for _, _, ifc in units if ifc.exists()} | set(database.parent.glob("pcm.cache/*.ifc")))
    with concurrent.futures.ThreadPoolExecutor(max_workers=jobs) as pool:
        printed = list(pool.map(lambda p: (p, *printer_reads(printer, p)), all_ifc))
    unread = [(p.name, why) for p, ok, why in printed if not ok]
    check(f"A1.1.2 ifc-printer reads all {len(all_ifc)} .ifc files without an error", not unread and bool(all_ifc), f"{unread[:5]}")

    cache = tempfile.mkdtemp(prefix="mcxx-ifc-")

    def roundtrip(unit):
        file, _, ifc = unit
        run = subprocess.run([probe, "--db", str(database.parent), "--resource", resource, "--cache", cache, "--ifc", str(ifc), file],
                             capture_output=True, text=True)
        line = next((json.loads(l) for l in run.stdout.splitlines() if l.startswith("{") and '"differences"' in l), None)
        return file, line, run.stderr[-300:]

    with concurrent.futures.ThreadPoolExecutor(max_workers=jobs) as pool:
        results = list(pool.map(roundtrip, [u for u in own if u[2].exists()]))
    declarations = sum(r[1]["declarations"] for r in results if r[1] and "declarations" in r[1])
    differ = [(f, r["differences"][:3] if r else why) for f, r, why in results if not r or r["differences"]]
    check(f"A1.1.3 {len(results)} interface units' declarations read back from their .ifc equal a parse's, field by field "
          f"({declarations} declarations)", not differ and bool(results), "\n      ".join(f"{f}: {d}" for f, d in differ[:8]))
    shutil.rmtree(cache, ignore_errors=True)

    # MC2 1.2.0 (MC2-3.1-1): libc++'s std module exports only using-declarations; its interface reaches
    # what they name -- beside the build's std BMI, or kept in the store by the BMI's content (mcpp copies
    # std's BMI out of its cache).
    import hashlib, os
    std_bmi = database.parent / "pcm.cache/std.pcm"
    store = pathlib.Path(os.environ.get("MCXX_IFC_STORE") or pathlib.Path(os.environ.get("XDG_CACHE_HOME", pathlib.Path.home() / ".cache")) / "mcxx/ifc")
    std_ifc = std_bmi.with_suffix(".ifc")
    if not std_ifc.exists() and std_bmi.exists():
        std_ifc = store / f"{hashlib.sha256(std_bmi.read_bytes()).hexdigest()}.ifc"
    std_read = read_ifc(std_ifc) if std_ifc.exists() else None
    reached = {d["qualified-name"] for d in (std_read or {}).get("reachable", [])}
    wanted = ["std::vector", "std::vector::size", "std::vector::push_back", "std::basic_string", "std::string", "std::move", "std::println",
              "std::errc::invalid_argument", "std::optional::value", "std::map::find"]
    check(f"std's interface reaches what its exported using-declarations name ({len(reached)} declarations): {', '.join(wanted[:4])}, ...",
          bool(std_read) and all(w in reached for w in wanted), f"{std_ifc}: missing {[w for w in wanted if w not in reached]}")

print(f"{'FAIL' if failures else 'PASS'}: MC2 v1 ({len(failures)} failed)")
sys.exit(1 if failures else 0)
