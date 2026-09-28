#!/usr/bin/env python3
"""Self-host (A0.5.3): C-mcppls -- mcpp-language-server at the corpus revision -- built by mcpp with mcxx as
its toolchain, passes its own unit tests (`mcpp test`).

    python3 tools/checks/selfhost.py [--toolchain llvm@23.1.0-mcxx] [--work DIR] [--source GIT-URL-OR-PATH]

The corpus is checked out at REVISION into the work directory (default ~/.cache/mcxx-checks/c-mcppls,
outside the repository). Its code is not changed; its manifest gets one local line: MC++'s default
gate json-brace-init is `warn`, not `deny`, because the corpus has two real instances of the pitfall
(a one-element array meant as a copy, src/bin/conformance.cpp and src/orchestrator/workspace.cpp),
which MC++ reports and the measurement lets through. Exits non-zero when the build or a test fails.
"""
import pathlib, shutil, subprocess, sys

REVISION = "377d222"
toolchain = sys.argv[sys.argv.index("--toolchain") + 1] if "--toolchain" in sys.argv else "llvm@23.1.0-mcxx"
work = pathlib.Path(sys.argv[sys.argv.index("--work") + 1] if "--work" in sys.argv else pathlib.Path.home() / ".cache/mcxx-checks/c-mcppls")
source = sys.argv[sys.argv.index("--source") + 1] if "--source" in sys.argv else "https://github.com/Sunrisepeak/mcpp-language-server.git"

shutil.rmtree(work, ignore_errors=True)
subprocess.run(["git", "clone", "--quiet", source, str(work)], check=True)
subprocess.run(["git", "checkout", "--quiet", REVISION], cwd=work, check=True)
with open(work / "mcpp.toml", "a") as m:
    m.write('\n# Local, for the self-host measurement (A0.5.3): MC++ reports the pitfall, the build goes on.\n'
            '[package.metadata.mcxx.features]\n"json-brace-init" = "warn"\n')
test = subprocess.run(["mcpp", "test", "--toolchain", toolchain], cwd=work, capture_output=True, text=True)
tail = "\n".join((test.stdout + test.stderr).splitlines()[-6:])
print(tail)
ok = test.returncode == 0 and "test result ok" in test.stdout + test.stderr
print(f"{'PASS' if ok else 'FAIL'}  C-mcppls ({REVISION}) built with {toolchain} passes its own tests")
sys.exit(0 if ok else 1)
