#!/usr/bin/env python3
"""E-IDX-5: MC++'s packages from this repository's own index (index/mcxx), as a consumer names them.

    python3 tools/checks/index.py --mcxx MCXX [--resource DIR] [--work DIR]

Two consumers, outside the repository (default ~/.cache/mcxx-checks/index):

- the rule package's test program, its [build-dependencies] `mcxx.mcpp-tools-safe = "0.1.0"` from the
  index instead of by path, built with GCC 16: the check runs and writes its stamp;
- a program depending on `mcxx.mcxx-plugins-std` and `mcxx.mcxx-plugins-libs` 0.1.0: it builds, and the
  catalog it links lists their providers.

Each package is the tagged archive the index entry names (its sha256 checked by mcpp). Exits non-zero on
any failure.
"""
import os, pathlib, shutil, subprocess, sys

def arg(name, default=None):
    return sys.argv[sys.argv.index(f"--{name}") + 1] if f"--{name}" in sys.argv else default

repo = pathlib.Path(__file__).resolve().parents[2]
mcxx = str(pathlib.Path(arg("mcxx")).resolve())
resource = arg("resource")
work = pathlib.Path(arg("work", pathlib.Path.home() / ".cache/mcxx-checks/index")).resolve()
index = repo / "index/mcxx"
env = dict(os.environ, MCXX=mcxx, **({"MCXX_RESOURCE": resource} if resource else {}))
failures = []


def check(label, ok, detail=""):
    print(f"{'PASS' if ok else 'FAIL'}  {label}" + (f"\n      {detail}" if detail and not ok else ""))
    if not ok:
        failures.append(label)


shutil.rmtree(work, ignore_errors=True)
consumer = work / "consumer"
shutil.copytree(repo / "plugins/mcpp-tools-safe/tests/consumer", consumer, ignore=shutil.ignore_patterns("target", ".mcpp"))
manifest = consumer / "mcpp.toml"
text = manifest.read_text()
by_path = '[build-dependencies]\nmcpp-tools-safe = { path = "../..", host-module = true }\n'
assert by_path in text, "the consumer's manifest changed: update this check"
manifest.write_text(text.replace(by_path, f'[indices]\nmcxx = {{ path = "{index}" }}\n\n[build-dependencies.mcxx]\n'
                                          'mcpp-tools-safe = { version = "0.1.0", host-module = true }\n'))
run = subprocess.run(["mcpp", "build", "--toolchain", "gcc@16.1.0"], cwd=consumer, capture_output=True, text=True, env=env)
check("mcxx.mcpp-tools-safe 0.1.0 from the index: the consumer builds with GCC, the check passing",
      run.returncode == 0 and "mcxx:mcpp-tools-safe" in run.stdout + run.stderr, (run.stdout + run.stderr)[-600:])
check("  and the check wrote its stamp", any(consumer.glob("target/**/mcxx-check.stamp")))

program = work / "plugins"
(program / "src").mkdir(parents=True)
(program / "mcpp.toml").write_text(f'''[package]
name    = "index-plugins"
version = "0.1.0"

[targets.index-plugins]
kind = "bin"
main = "src/main.cpp"

[indices]
mcxx = {{ path = "{index}" }}

[dependencies]
openkal-llvm-runtime = "0.15.2"

[dependencies.mcxx]
mcxx-plugins-std  = "0.1.0"
mcxx-plugins-libs = "0.1.0"
''')
(program / "src/main.cpp").write_text('''import std;
import mcxx.plugin;
import mcxx.plugins.std;
import mcxx.plugins.json;

// The plugins register their providers at static initialization: the catalog lists them.
int main() {
    for (const auto& p : mcxx::plugin::catalog()->providers) std::println("{}", p.provider->name());
    return 0;
}
''')
run = subprocess.run(["mcpp", "build"], cwd=program, capture_output=True, text=True)
check("mcxx.mcxx-plugins-std and -libs 0.1.0 from the index: a program builds with them", run.returncode == 0, (run.stdout + run.stderr)[-600:])
built = sorted(program.glob("target/*/*/bin/index-plugins"))
listed = subprocess.run([str(built[-1])], capture_output=True, text=True).stdout.split() if built else []
check("  and its catalog lists their providers", {"mc++.policy", "mcxx.plugins.cfg", "mcxx.plugins.json"} <= set(listed), f"{listed}")

print(f"{'FAIL' if failures else 'PASS'}: MC++'s packages from its index ({len(failures)} failed)")
sys.exit(1 if failures else 0)
