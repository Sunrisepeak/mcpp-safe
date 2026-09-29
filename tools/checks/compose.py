#!/usr/bin/env python3
"""mcxx compose, end to end (MC4 section 3, A0.6.1), and the same plugin out of process (A0.6.2).

    python3 tools/checks/compose.py --mcxx PATH [--remote PROGRAM] [--library LIB] [--work DIR]

A package whose static plugin is plugins/examples/naming (declared names are snake_case):
1. plain mcxx refuses to compile the package's file and says to run `mcxx compose` (MC4-3-4);
2. `mcxx compose` builds the package's compiler (the first time takes minutes: libmc++ is built for it);
3. the same `mcxx check` is handed to the composed compiler, which reports the plugin's findings;
4. `mcxx compose` again does not relink (MC4-3-3);
5. with --remote: a package that declares PROGRAM (plugins/examples/naming-remote, built with any
   toolchain -- CI uses GCC 16) as an out-of-process plugin gets exactly the same diagnostics for the
   same file from plain mcxx (MC4-6.3-3);
6. with --library: a package that declares LIB (plugins/examples/naming-library, built by mcpp) as a
   plugin library gets exactly the same diagnostics from plain mcxx, the plugin loaded into it
   (MC4-3-1); a library that is not there, and a shared library not built against the SDK, are
   plugins that cannot be started, and the package's files fail (MC4-3-6, MC4-3-7, MC4-5-5).
The work directory is outside the repository (default ~/.cache/mcxx-checks/compose): a directory under
it would inherit this repository's .xlings.json. Exits non-zero on any failure.
"""
import os, pathlib, re, shutil, subprocess, sys

repository = pathlib.Path(__file__).resolve().parents[2]
mcxx = sys.argv[sys.argv.index("--mcxx") + 1]
remote = sys.argv[sys.argv.index("--remote") + 1] if "--remote" in sys.argv else None
library = sys.argv[sys.argv.index("--library") + 1] if "--library" in sys.argv else None
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
    if sys.platform.startswith("linux"):
        # e_type at offset 16: ET_DYN (3) for a position-independent program (MC4-3-1).
        check("the composed compiler is linked as mcxx is, and loads plugin libraries too (MC4-3-1)",
              int.from_bytes(binary.resolve().read_bytes()[16:18], "little") == 3)
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
if library:
    def app(name, declaration):
        out = work / name
        (out / "src").mkdir(parents=True)
        (out / "mcpp.toml").write_text(f'[package]\nname = "app"\nversion = "0.1.0"\n\n[package.metadata.mcxx.plugins]\n'
                                       f'naming = {{ library = "{declaration}" }}\n\n'
                                       f'[package.metadata.mcxx.features]\nsnake-case-names = "deny"\n')
        (out / "src/a.cpp").write_text(SOURCE)
        return subprocess.run(args, cwd=out, capture_output=True, text=True, env=env)

    run = app("library", pathlib.Path(library).resolve())
    loaded = findings(run.stderr)
    check("the plugin as a library gives the static composition's diagnostics, exactly (MC4-3-1)",
          run.returncode == 1 and len(loaded) == 4 and (loaded == static or not static), f"{loaded} != {static}; {run.stderr[:300]}")
    missing = app("library-missing", "lib/libnone.so")
    check("a library that is not there cannot be loaded, and the package's files fail (MC4-3-6, MC4-5-5)",
          missing.returncode == 1 and "could not be loaded" in missing.stderr and "library-missing/lib/libnone.so" in missing.stderr
          and "[mcxx-plugin]" in missing.stderr, missing.stderr[:300])
    # A shared library of this toolchain that is not a plugin: built here, C, nothing else.
    plain = work / "not-a-plugin"
    (plain / "src").mkdir(parents=True)
    (plain / "mcpp.toml").write_text('[package]\nname = "plain"\nversion = "0.1.0"\n\n[targets.plain]\nkind = "shared"\n')
    (plain / "src/plain.c").write_text("int plain_value(void) { return 1; }\n")
    built = subprocess.run(["mcpp", "build"], cwd=plain, capture_output=True, text=True)
    so = next(iter(sorted(plain.glob("target/*/*/bin/libplain.so"))), None)
    check("a shared library to try as a plugin is built", built.returncode == 0 and so is not None, built.stderr[-300:])
    if so:
        other = app("library-other", so)
        check("a shared library not built against the SDK is not taken (MC4-3-7)",
              other.returncode == 1 and "not a plugin library" in other.stderr and "[mcxx-plugin]" in other.stderr, other.stderr[:300])
print(f"\n{failures} failed")
sys.exit(1 if failures else 0)
