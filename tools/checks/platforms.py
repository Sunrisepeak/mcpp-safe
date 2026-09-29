#!/usr/bin/env python3
"""libmc++'s unit tests on openkal's other targets: built here for Windows and macOS, run where they can be.

    python3 tools/checks/platforms.py --target x86_64-windows-gnu [--wine] [--out DIR]
    python3 tools/checks/platforms.py --target aarch64-macos --out DIR
    python3 tools/checks/platforms.py --run DIR

--target: every member's tests (`mcpp test -p <member> --target T --no-run`), cross-built from this
host; with --wine each Windows executable runs under wine here; with --out they are copied to DIR/<T>/,
to be run on the platform itself. --run: every executable under DIR runs (on macOS signed ad hoc
first). A test reads the repository's data through $MCXX_REPOSITORY, set here to this checkout (its
source path is the build machine's). Exits non-zero when a build or a test fails.
"""
import os, pathlib, platform, shutil, subprocess, sys, time

MEMBERS = ["modules/base", "modules/graph", "modules/lsp", "modules/features", "modules/diagnostics", "modules/frontend", "modules/ifc",
           "modules/plugin/wire", "modules/plugin/remote", "modules/plugin/host", "modules/serve", "plugins/std",
           "plugins/libs", "modules/backend/clang"]

repo = pathlib.Path(__file__).resolve().parents[2]
args = sys.argv[1:]
failures = []


def arg(name, default=None):
    return args[args.index(f"--{name}") + 1] if f"--{name}" in args else default


def check(label, ok, detail=""):
    print(f"{'PASS' if ok else 'FAIL'}  {label}" + (f"\n      {detail}" if detail and not ok else ""), flush=True)
    if not ok:
        failures.append(label)


def run_test(exe, command, env):
    start = time.monotonic()
    try:
        done = subprocess.run(command, capture_output=True, text=True, env=env, timeout=1200, cwd=exe.parent)
    except subprocess.TimeoutExpired:
        check(f"{exe.name}: timed out", False)
        return
    last = next((l for l in reversed(done.stdout.splitlines()) if "test cases" in l), "")
    check(f"{exe.name} ({time.monotonic() - start:.1f} s) {last}", done.returncode == 0 and ", 0 failed;" in last,
          (done.stdout + done.stderr)[-1500:])


def built(member, target):
    """The member's test executables from the newest build for `target`, one per tests/*.cpp."""
    suffix = ".exe" if "windows" in target else ""
    found = []
    for source in sorted((repo / member / "tests").glob("*.cpp")):
        candidates = sorted((repo / member / "target" / target).glob(f"*/bin/{source.stem}{suffix}"), key=lambda p: p.stat().st_mtime)
        if candidates:
            found.append(candidates[-1])
        else:
            check(f"{member}: {source.stem}{suffix} built for {target}", False)
    return found


if arg("run"):
    bundle = pathlib.Path(arg("run")).resolve()
    env = dict(os.environ, MCXX_REPOSITORY=str(repo))
    for exe in sorted(p for p in bundle.rglob("*") if p.is_file()):
        exe.chmod(0o755)
        if platform.system() == "Darwin":
            subprocess.run(["codesign", "-s", "-", "-f", str(exe)], capture_output=True)
        run_test(exe, [str(exe)], env)
else:
    target = arg("target")
    out = pathlib.Path(arg("out")).resolve() if arg("out") else None
    wine = "--wine" in args
    for member in MEMBERS:
        build = subprocess.run(["mcpp", "test", "-p", member, "--target", target, "--no-run"], cwd=repo, capture_output=True, text=True)
        check(f"{member}: its tests built for {target}", build.returncode == 0, (build.stdout + build.stderr)[-1500:])
        if build.returncode != 0:
            continue
        for exe in built(member, target):
            if out:
                (out / target).mkdir(parents=True, exist_ok=True)
                shutil.copy2(exe, out / target / exe.name)
            if wine:
                # wine names this machine's root Z:, so the checkout is Z:<path> to a Windows program.
                env = dict(os.environ, WINEDEBUG="-all", MCXX_REPOSITORY=f"Z:{repo}")
                run_test(exe, ["wine", str(exe)], env)

print(f"{'FAIL' if failures else 'PASS'}: libmc++'s unit tests on other targets ({len(failures)} failed)")
sys.exit(1 if failures else 0)
