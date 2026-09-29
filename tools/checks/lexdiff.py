#!/usr/bin/env python3
"""MC++'s own lexer against Clang's (A1.6.1): for every C++ source file under the given roots, the tokens
mcxx.frontend:lex finds are Clang's raw lexer's -- kind, place and text, comments included -- token by token.

    python3 tools/checks/lexdiff.py --lexdump MCXX_LEXDUMP --probe MCXX_PROBE ROOT... [--any] [--jobs N] [--show N]

    MCXX_LEXDUMP  tools/lexdump: MC++'s lexer, one JSON object per token
    MCXX_PROBE    tools/probe: `--tokens` prints the same for Clang's raw lexer under C++23 (what
                  `-dump-raw-tokens` prints), through the backend
    ROOT          directories searched for .cpp, .cppm, .cc, .cxx, .h, .hpp, .hh, .ipp, .inl and .tpp files,
                  skipping build output and copies (target/, .mcpp/, .git/, .claude/, .deps/, forks/); with --any,
                  files without a suffix too (libc++'s headers)

Whitespace is not a token of either. Prints each file that differs with its first difference, the totals
and each lexer's time; exits non-zero when any file differs.
"""
import concurrent.futures, json, os, pathlib, subprocess, sys, time

def arg(name, default=None):
    return sys.argv[sys.argv.index(f"--{name}") + 1] if f"--{name}" in sys.argv else default

lexdump = str(pathlib.Path(arg("lexdump")).resolve())
probe = str(pathlib.Path(arg("probe")).resolve())
jobs = int(arg("jobs", os.cpu_count() or 4))
show = int(arg("show", 20))
options = {"--lexdump", "--probe", "--jobs", "--show"}
any_file = "--any" in sys.argv
roots = [a for i, a in enumerate(sys.argv[1:], 1) if a not in options | {"--any"} and sys.argv[i - 1] not in options]
SUFFIXES = {".cpp", ".cppm", ".cc", ".cxx", ".c++", ".ixx", ".h", ".hpp", ".hh", ".hxx", ".ipp", ".inl", ".tpp"}
SKIP = {"target", ".mcpp", ".git", ".xlings", ".claude", ".deps", "node_modules", "forks"}

files = []
for root in roots:
    for directory, subdirectories, names in os.walk(root):
        subdirectories[:] = [d for d in subdirectories if d not in SKIP]
        files += [str(pathlib.Path(directory, n).resolve()) for n in names if pathlib.Path(n).suffix in SUFFIXES or (any_file and not pathlib.Path(n).suffix)]
files = sorted(set(files))
chunks = [files[i:i + 64] for i in range(0, len(files), 64)]


def tokens(command, chunk):
    started = time.perf_counter()
    run = subprocess.run(command + chunk, capture_output=True)
    elapsed = time.perf_counter() - started
    by_file = {f: [] for f in chunk}
    for line in run.stdout.decode("utf-8", "replace").splitlines():
        t = json.loads(line)
        by_file[t["file"]].append((t["kind"], t["line"], t["column"], t["text"]))
    return by_file, elapsed, run.returncode


def compare(chunk):
    ours, ours_time, ours_status = tokens([lexdump], chunk)
    clangs, clang_time, clang_status = tokens([probe, "--tokens"], chunk)
    differences = []
    for f in chunk:
        a, b = ours[f], clangs[f]
        if a != b:
            i = next((i for i, (x, y) in enumerate(zip(a, b)) if x != y), min(len(a), len(b)))
            differences.append((f, i, a[i] if i < len(a) else None, b[i] if i < len(b) else None, len(a), len(b)))
    count = sum(len(v) for v in clangs.values())
    return differences, count, ours_time, clang_time, ours_status or clang_status


with concurrent.futures.ThreadPoolExecutor(max_workers=jobs) as pool:
    results = list(pool.map(compare, chunks))

differences = [d for r in results for d in r[0]]
total = sum(r[1] for r in results)
for f, i, ours, clangs, n_ours, n_clang in differences[:show]:
    print(f"{f}: token {i} (of {n_clang} Clang's, {n_ours} ours)\n    ours:  {ours}\n    Clang: {clangs}")
if len(differences) > show:
    print(f"... and {len(differences) - show} more files")
failed = [r for r in results if r[4]]
print(f"lexdiff: {len(files) - len(differences)} of {len(files)} files agree ({total} tokens); "
      f"time summed over runs: MC++'s lexer {sum(r[2] for r in results):.1f}s, Clang's through the probe {sum(r[3] for r in results):.1f}s"
      + (f"; {len(failed)} runs exited non-zero" if failed else ""))
sys.exit(1 if differences or failed or not files else 0)
