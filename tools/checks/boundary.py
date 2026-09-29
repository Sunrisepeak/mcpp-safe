#!/usr/bin/env python3
"""The dialect boundary across modules (M1.2), compiled by mcxx: conformance/boundary.

    python3 tools/checks/boundary.py --mcxx MCXX [--toolchain llvm@23.1.0-mcxx] [--work DIR]

The legacy package's interfaces are built first (BMI and .ifc), then each file of the safe package that
imports them. A1.2.1: every finding at an import is expected (`// expect: ids`), every expected one is
reported, and every waiver an import's [[mcpp::allow]] makes is in the audit (`// expect-waived: ids`).
A1.2.2: with the legacy package's sources deleted the importers' findings and audit are the same -- the
judgement reads the imported modules' .ifc, never their source. With the .ifc beside the BMI deleted, the
store's copy (kept under the BMI's content by the compile that wrote it) is read and nothing changes; with
the store empty too, every feature the importer denies is "not known" at the import. With --toolchain (mcxx
installed as an mcpp toolchain), app-manifest -- whose manifest allows what `import legacy` brings in -- is
built by mcpp with it, the audit holding the four waivers at the import. The work directory
(default ~/.cache/mcxx-checks/boundary) is outside the repository. Exits non-zero on any failure.
"""
import json, os, pathlib, re, shutil, subprocess, sys

def arg(name, default=None):
    return sys.argv[sys.argv.index(f"--{name}") + 1] if f"--{name}" in sys.argv else default

repo = pathlib.Path(__file__).resolve().parents[2]
mcxx = str(pathlib.Path(arg("mcxx")).resolve())
work = pathlib.Path(arg("work", pathlib.Path.home() / ".cache/mcxx-checks/boundary")).resolve()
failures = []


def check(label, ok, detail=""):
    print(f"{'PASS' if ok else 'FAIL'}  {label}" + (f"\n      {detail}" if detail and not ok else ""))
    if not ok:
        failures.append(label)


shutil.rmtree(work, ignore_errors=True)
shutil.copytree(repo / "conformance/boundary", work / "boundary")
legacy, app = work / "boundary/legacy/src", work / "boundary/app"
store, audit = work / "store", work / "audit.jsonl"
env = dict(os.environ, MCXX_IFC_STORE=str(store), MCXX_AUDIT=str(audit))
FLAGS = ["c++", "-std=c++23"]
MODULES = [f"-fmodule-file=legacy={legacy / 'legacy.pcm'}", f"-fmodule-file=legacy:detail={legacy / 'legacy-detail.pcm'}",
           f"-fmodule-file=app.table={app / 'app.table.pcm'}"]


def compile_(cwd, args, environment=None):
    return subprocess.run([mcxx, *FLAGS, *args], cwd=cwd, capture_output=True, text=True, env=environment or env)


FINDING = re.compile(r"^(?P<file>[^:\s]+):(?P<line>\d+):\d+: (?:error|warning): .*? \[(?P<id>[a-z][a-z0-9:.-]*)\](?:;|$)")


def findings(stderr):
    out = set()
    for line in stderr.splitlines():
        if m := FINDING.match(line):
            out.add((pathlib.Path(m["file"]).name, int(m["line"]), m["id"]))
    return out


def expected(tag):
    out = set()
    for f in sorted(app.glob("*.cpp")) + sorted(app.glob("*.cppm")):
        for n, text in enumerate(f.read_text().splitlines(), 1):
            # `// expect: ids` and `// expect-waived: ids`, several on a line separated by `;`.
            comment = text.split("// ", 1)[1] if "// " in text else ""
            for part in comment.split(";"):
                name, _, ids = part.strip().partition(": ")
                if name == tag and re.fullmatch(r"[a-z0-9: .-]+", ids):
                    out |= {(f.name, n, i) for i in ids.split()}
    return out


def waivers():
    if not audit.exists():
        return set()
    out = set()
    for line in audit.read_text().splitlines():
        r = json.loads(line)
        out.add((pathlib.Path(r["path"]).name, int(r["line"]), r["feature"]))
    return out


# The interfaces: legacy's partition, then its primary interface; the safe package's own module.
for cwd, args in ((legacy, ["--precompile", "legacy-detail.cppm", "-o", "legacy-detail.pcm"]),
                  (legacy, [MODULES[1], "--precompile", "legacy.cppm", "-o", "legacy.pcm"]),
                  (app, ["--precompile", "table.cppm", "-o", "app.table.pcm"])):
    run = compile_(cwd, args)
    check(f"mcxx builds {args[-3]} with its .ifc", run.returncode == 0 and (cwd / pathlib.Path(args[-1]).with_suffix(".ifc")).exists(), run.stderr[-400:])


def importers(environment=None):
    if audit.exists():
        audit.unlink()
    got = set()
    for f in sorted(app.glob("*.cpp")):
        run = compile_(app, [*MODULES, "-c", f.name, "-o", f.with_suffix(".o").name], environment)
        got |= findings(run.stderr)
    return got, waivers()


got, waived = importers()
want = expected("expect")
check(f"A1.2.1 a safe module's import of what its dialect denies is an error there, unless the import says so: {len(got)} findings",
      got == want, f"missing {sorted(want - got)}; unexpected {sorted(got - want)}")
want_waived = expected("expect-waived") - {w for w in expected("expect-waived") if w[0].endswith(".cppm")}
check(f"A1.2.1 what an import's [[mcpp::allow]] waives is in the audit: {len(waived)} waivers",
      waived == want_waived, f"missing {sorted(want_waived - waived)}; unexpected {sorted(waived - want_waived)}")

for source in legacy.glob("*.cppm"):
    source.unlink()
again, waived_again = importers()
check("A1.2.2 with the imported package's sources deleted, the findings and the audit are the same (only its .ifc is read)",
      again == got and waived_again == waived, f"now {sorted(again ^ got)} {sorted(waived_again ^ waived)}")

for beside in legacy.glob("*.ifc"):
    beside.unlink()
kept, _ = importers()
check("without the .ifc beside the BMI, the store's copy (by the BMI's content) is read: the same findings", kept == got, f"{sorted(kept ^ got)}")

empty = dict(env, MCXX_IFC_STORE=str(work / "empty-store"))
unknown, _ = importers(empty)
plain = {f for f in unknown if f[0] == "plain.cpp"}
check("with no interface anywhere, what the import brings in is not known: an error for each feature the importer denies",
      plain == {("plain.cpp", 1, i) for i in ("c-array", "c-varargs", "raw-pointers", "union")}, f"{sorted(plain)}")
missing = compile_(app, [*MODULES, "-c", "plain.cpp", "-o", "plain.o"], empty)
check("  and it says why: no MC2 interface was found", "no MC2 interface of legacy" in missing.stderr, missing.stderr[:300])

toolchain = arg("toolchain")
if toolchain:
    manifest = work / "boundary/app-manifest"
    shutil.rmtree(work / "boundary/legacy", ignore_errors=True)
    shutil.copytree(repo / "conformance/boundary/legacy", work / "boundary/legacy")   # its sources again: mcpp builds it
    if audit.exists():
        audit.unlink()
    built = subprocess.run(["mcpp", "build", "--toolchain", toolchain], cwd=manifest, capture_output=True, text=True, env=env)
    records = [json.loads(l) for l in audit.read_text().splitlines()] if audit.exists() else []
    at_import = sorted(r["feature"] for r in records if r["path"].endswith("main.cpp") and r["line"] == 1 and r["declaration"].startswith("import legacy"))
    check("mcpp builds app-manifest with mcxx: the manifest's allowance for `import legacy` waives, in the audit, what it brings in",
          built.returncode == 0 and at_import == ["c-array", "c-varargs", "raw-pointers", "union"], f"{at_import} {(built.stdout + built.stderr)[-600:]}")

print(f"{'FAIL' if failures else 'PASS'}: the dialect boundary ({len(failures)} failed)")
sys.exit(1 if failures else 0)
