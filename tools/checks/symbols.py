#!/usr/bin/env python3
"""No Clang in what does not need it (A0.2.3): a program built from libmc++ packages that do not depend on
the backend -- features, msa, the plugin SDK, plugins -- contains no symbol of Clang or LLVM.

    python3 tools/checks/symbols.py PROGRAM... [--expect-clang PROGRAM...]

Every PROGRAM before --expect-clang must have none; every one after it must have some (the check's own
control: mcxx does). Symbols are read with `nm -C`. Exits non-zero on any failure.
"""
import re, subprocess, sys

args = sys.argv[1:]
split = args.index("--expect-clang") if "--expect-clang" in args else len(args)
clean, clangy = args[:split], args[split + 1:]
pattern = re.compile(r"\b(clang|llvm)::")
failures = 0
for program, want in [(p, False) for p in clean] + [(p, True) for p in clangy]:
    out = subprocess.run(["nm", "-C", program], capture_output=True, text=True).stdout
    hits = [l for l in out.splitlines() if pattern.search(l)]
    ok = bool(hits) == want
    failures += 0 if ok else 1
    print(f"{'PASS' if ok else 'FAIL'}  {program}: {len(hits)} Clang/LLVM symbols{' (expected some)' if want else ''}"
          + ("" if ok or want else f"; e.g. {hits[0].strip()[:120]}"))
sys.exit(1 if failures else 0)
