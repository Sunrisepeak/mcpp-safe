#!/usr/bin/env python3
"""mcpp-tools-safe end to end (A1.5.1-A1.5.3): a module program built with GCC, gated by MC++ through the
rule package (plugins/mcpp-tools-safe) as a blocking check action.

    python3 tools/checks/tools_safe.py --mcxx MCXX [--resource DIR] [--work DIR] [--toolchain gcc@16.1.0] [--corpus DIR]

    --mcxx      the mcxx the rule runs (a payload's bin/mcxx; a development one needs --resource)
    --corpus    also a copy of this program (C-mcpp: its compile database is GCC's), its first source
                given a goto: the build fails at the check, and passes again without it

Checks, on tests/consumer (strict profile: goto denied): the build passes and the check wrote its
stamp; built again unchanged, nothing runs; a goto added, the build fails at the check, naming the
goto; the goto removed, the build passes again. Exits non-zero on any failure.
"""
import os, pathlib, re, shutil, subprocess, sys

def arg(name, default=None):
    return sys.argv[sys.argv.index(f"--{name}") + 1] if f"--{name}" in sys.argv else default

repo = pathlib.Path(__file__).resolve().parents[2]
mcxx = str(pathlib.Path(arg("mcxx")).resolve())
resource = arg("resource")
toolchain = arg("toolchain", "gcc@16.1.0")
work = pathlib.Path(arg("work", str(pathlib.Path.home() / ".cache" / "mcxx-checks" / "tools-safe")))
rule = repo / "plugins" / "mcpp-tools-safe"
env = dict(os.environ, MCXX=mcxx, **({"MCXX_RESOURCE": resource} if resource else {}))
failures = 0


def check(what, ok, detail=""):
    global failures
    failures += 0 if ok else 1
    print(f"{'PASS' if ok else 'FAIL'}  {what}" + (f": {detail}" if detail and not ok else ""))


def build(where):
    run = subprocess.run(["mcpp", "build", "--toolchain", toolchain], cwd=where, env=env, capture_output=True, text=True)
    return run.returncode, run.stdout + run.stderr


def prepare(source, where):
    shutil.rmtree(where, ignore_errors=True)
    shutil.copytree(source, where, ignore=shutil.ignore_patterns("target", ".mcpp"))
    manifest = where / "mcpp.toml"
    text = manifest.read_text()
    text = re.sub(r'mcpp-tools-safe = \{ path = "[^"]*"', f'mcpp-tools-safe = {{ path = "{rule}"', text)
    manifest.write_text(text)


def goto(path):
    text = path.read_text()
    path.write_text(text.replace("return 0;", "int n = 0;\n    if (n) goto done;\ndone:\n    return 0;", 1))


consumer = work / "consumer"
prepare(rule / "tests" / "consumer", consumer)
status, out = build(consumer)
check("the consumer builds with the check passing (A1.5.1)", status == 0, out[-600:])
stamps = list(consumer.glob("target/**/mcxx-check.stamp"))
check("the check ran and wrote its stamp", bool(stamps))
status, out = build(consumer)
check("built again unchanged, nothing runs (A1.5.3)", status == 0 and "Compiling" not in out and "mcxx check" not in out, out[-400:])
main = consumer / "src" / "main.cpp"
original = main.read_text()
goto(main)
status, out = build(consumer)
check("a goto (strict denies it) fails the build at the check (A1.5.2)", status != 0 and "[goto]" in out, out[-600:])
main.write_text(original)
status, out = build(consumer)
check("the goto removed, the build passes again", status == 0, out[-600:])

corpus = arg("corpus")
if corpus:
    # A copy of a GCC-built program with no build.mcpp of its own (C-mcpp), given the rule's.
    copy = work / "corpus"
    prepare(pathlib.Path(corpus), copy)
    manifest = copy / "mcpp.toml"
    text = manifest.read_text()
    if "[build-dependencies]" not in text:
        text += "\n[build-dependencies]\n"
    text = text.replace("[build-dependencies]\n", f'[build-dependencies]\nmcpp-tools-safe = {{ path = "{rule}", host-module = true }}\n', 1)
    manifest.write_text(text)
    # What is gated: goto (the program sets no gates of its own).
    manifest.write_text(manifest.read_text() + '\n[package.metadata.mcxx.features]\ngoto = "deny"\n')
    (copy / "build.mcpp").write_text((rule / "tests" / "consumer" / "build.mcpp").read_text())
    status, out = build(copy)
    check(f"{corpus} builds with {toolchain}, the check passing", status == 0, out[-800:])
    first = sorted((copy / "src").rglob("*.cpp"))[0]
    text = first.read_text()
    first.write_text(text + "\nstatic int mcxx_tools_safe_probe(int n) {\n    if (n) goto out;\nout:\n    return n;\n}\n")
    status, out = build(copy)
    check("a goto in it fails the build at the check (A1.5.2)", status != 0 and "goto" in out, out[-800:])
    first.write_text(text)
    status, out = build(copy)
    check("the goto removed, it builds again", status == 0, out[-800:])

sys.exit(1 if failures else 0)
