#!/usr/bin/env python3
"""mcxx compose, end to end (MC4 section 3, A0.6.1), and the same plugin out of process (A0.6.2).

    python3 tools/checks/compose.py --mcxx PATH [--remote PROGRAM] [--work DIR]

A package whose static plugin is plugins/examples/naming (declared names are snake_case):
1. plain mcxx refuses to compile the package's file and says to run `mcxx compose` (MC4-3-4);
2. `mcxx compose` builds the package's compiler (the first time takes minutes: libmc++ is built for it);
3. the same `mcxx check` is handed to the composed compiler, which reports the plugin's findings;
4. `mcxx compose` again does not relink (MC4-3-3);
5. with --remote: a package that declares PROGRAM (plugins/examples/naming-remote, built with any
   toolchain -- CI uses GCC 16) as an out-of-process plugin gets exactly the same diagnostics for the
   same file from plain mcxx (MC4-6.3-3).
The work directory is outside the repository (default ~/.cache/mcxx-checks/compose): a directory under
it would inherit this repository's .xlings.json. Exits non-zero on any failure.
"""
import os, pathlib, re, shutil, subprocess, sys

repository = pathlib.Path(__file__).resolve().parents[2]
mcxx = sys.argv[sys.argv.index("--mcxx") + 1]
remote = sys.argv[sys.argv.index("--remote") + 1] if "--remote" in sys.argv else None
work = pathlib.Path(sys.argv[sys.argv.index("--work") + 1] if "--work" in sys.argv else pathlib.Path.home() / ".cache/mcxx-checks/compose")
failures = 0


def check(label, ok, detail=""):
    global failures
    print(f"{'PASS' if ok else 'FAIL'}  {label}{('  ' + detail) if detail and not ok else ''}")
    failures += 0 if ok else 1


def findings(stderr):
    return sorted(set(re.findall(r"^src/a\.cpp:(\d+:\d+): error: (.*\[snake-case-names\])", stderr, re.M)))


SOURCE = """namespace app {
int TotalCount(int count) { int Doubled = count * 2; return Doubled; }
int ok_name(int value) { return value; }
struct Point { int X; int y; };
}
"""

shutil.rmtree(work, ignore_errors=True)
(work / "src").mkdir(parents=True)
(work / "mcpp.toml").write_text(f'[package]\nname = "app"\nversion = "0.1.0"\n\n[package.metadata.mcxx.plugins]\n'
                                f'naming = {{ path = "{repository}/plugins/examples/naming" }}\n\n'
                                f'[package.metadata.mcxx.features]\nsnake-case-names = "deny"\n')
(work / "src/a.cpp").write_text(SOURCE)
args = [mcxx, "check", "--target=x86_64-unknown-linux-gnu", "src/a.cpp"]
env = dict(os.environ, MCXX_COMPOSE_CACHE=str(work / ".compose"))

before = subprocess.run(args, cwd=work, capture_output=True, text=True, env=env)
check("plain mcxx refuses a package with static plugins it lacks, and says to compose", before.returncode == 1 and "mcxx compose" in before.stderr,
      before.stderr[:300])
first = subprocess.run([mcxx, "compose"], cwd=work, capture_output=True, text=True, env=env)
binary = pathlib.Path(first.stdout.strip().splitlines()[-1]) if first.returncode == 0 and first.stdout.strip() else None
check("mcxx compose builds the package's compiler", binary is not None and binary.exists(), first.stderr[-600:])
static = []
if binary:
    after = subprocess.run(args, cwd=work, capture_output=True, text=True, env=env)
    static = findings(after.stderr)
    check("the same check is handed to the composed compiler, which has the plugin",
          after.returncode == 1 and len(static) == 4 and any("`app::TotalCount` is not snake_case" in m for _, m in static), after.stderr[:400])
    stamp = binary.resolve().stat().st_mtime_ns
    again = subprocess.run([mcxx, "compose"], cwd=work, capture_output=True, text=True, env=env)
    check("mcxx compose again: not relinked (MC4-3-3)", again.returncode == 0 and binary.resolve().stat().st_mtime_ns == stamp, "relinked")
if remote:
    out = work / "remote"
    (out / "src").mkdir(parents=True)
    (out / "mcpp.toml").write_text(f'[package]\nname = "app"\nversion = "0.1.0"\n\n[package.metadata.mcxx.plugins]\n'
                                   f'naming = {{ command = ["{pathlib.Path(remote).resolve()}"] }}\n\n'
                                   f'[package.metadata.mcxx.features]\nsnake-case-names = "deny"\n')
    (out / "src/a.cpp").write_text(SOURCE)
    run = subprocess.run(args, cwd=out, capture_output=True, text=True, env=env)
    remote_findings = findings(run.stderr)
    check("the plugin out of process gives the static composition's diagnostics, exactly (MC4-6.3-3)",
          run.returncode == 1 and remote_findings == static and static, f"{remote_findings} != {static}; {run.stderr[:300]}")
print(f"\n{failures} failed")
sys.exit(1 if failures else 0)
