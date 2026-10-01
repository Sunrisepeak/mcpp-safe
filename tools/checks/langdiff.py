#!/usr/bin/env python3
"""MC++'s own preprocessor against Clang on the language features of C++26 and C++29 (ML-F, the differential of the
lexer and preprocessor papers: #embed, P1967R14, with P3540R3's offset as Clang's `clang::offset`; the stringizing and
macro rules of P2843R3; the identifiers of P3658R1): for every .cpp file of the given directories (default
modules/frontend/corpus/pp26), the tokens mcxx.frontend:preprocess gives with the features on are the main file's
tokens in Clang's `-E` output, as tools/checks/ppdiff.py does for a corpus.

    python3 tools/checks/langdiff.py --lexdump MCXX_LEXDUMP --clang CLANGXX [DIR...] [--show N]

    --clang   a Clang with C23's #embed: 23.1 (xim's LLVM under ~/.mcpp/registry/data/xpkgs/xim-x-llvm); its -E
              output is the reference where its reading is the paper's. The Clang fork's S4 line implements the same
              papers (P3540R3's own `offset`, P2843R3's diagnostics) and becomes the reference for the rest.
    DIR       a directory of .cpp files; the first line `// langdiff: c++2c [c23]` is the standard both read it as
              (-std= for Clang, --std for MC++), and, when a C standard follows, the C23 reading too: Clang's -E in C
              mode over the same text (#embed is C23's, and where the papers agree with it the output is the same);
              resources are found next to the file (`-I DIR`)

MC++ gets `--feature all`: every language feature of C++26 and C++29 on. Each file is `equal` or differs at a token
(the first one is printed); exits non-zero when any file differs or one cannot be preprocessed.
"""
import concurrent.futures, json, os, pathlib, re, subprocess, sys, tempfile


def arg(name, default=None):
    return sys.argv[sys.argv.index(f"--{name}") + 1] if f"--{name}" in sys.argv else default


lexdump = str(pathlib.Path(arg("lexdump")).resolve())
clang = str(pathlib.Path(arg("clang")).resolve())
show = int(arg("show", 10))
options = {"--lexdump", "--clang", "--show"}
roots = [a for i, a in enumerate(sys.argv[1:], 1) if a not in options and sys.argv[i - 1] not in options] or ["modules/frontend/corpus/pp26"]
files = sorted(str(p.resolve()) for root in roots for p in pathlib.Path(root).rglob("*.cpp"))
scratch = tempfile.mkdtemp(prefix="mcxx-langdiff-")


def check(path):
    first = pathlib.Path(path).read_text(encoding="utf-8").splitlines()[0]
    m = re.match(r"//\s*langdiff:\s*(\S+)(?:\s+(\S+))?", first)
    std = m.group(1) if m else "c++2c"
    directory = os.path.dirname(path)
    readings = [("c++", f"-std={std}", "c++")] + ([("c", f"-std={m.group(2)}", "c")] if m and m.group(2) else [])
    for language, flag, x in readings:
        output = os.path.join(scratch, f"{abs(hash(path))}.i")
        run = subprocess.run([clang, "-E", flag, "-Wno-c23-extensions", "-x", x, "-I", directory, path, "-o", output], capture_output=True, text=True)
        if run.returncode != 0:
            return path, "clang-failed", (language + ": " + run.stderr.strip().splitlines()[0][:300]) if run.stderr.strip() else ""
        ours = subprocess.run([lexdump, "--ppdiff", "--std", std, "--feature", "all", "-I", directory, path, output], capture_output=True, text=True)
        os.unlink(output)
        if ours.returncode != 0 or not ours.stdout.strip():
            return path, "lexdump-failed", ours.stderr[-300:]
        r = json.loads(ours.stdout)
        if r["verdict"] != "equal":
            return path, r["verdict"], f"({language}) token {r['at']} (line {r['line']})\n    ours:  {r['ours']}\n    clang: {r['clang']}"
    return path, "equal", ""


with concurrent.futures.ThreadPoolExecutor(max_workers=2) as pool:
    results = list(pool.map(check, files))
for f in os.listdir(scratch):
    os.unlink(os.path.join(scratch, f))
os.rmdir(scratch)
bad = [r for r in results if r[1] != "equal"]
for path, verdict, note in bad[:show]:
    print(f"{path}: {verdict} {note}")
print(f"langdiff: {len(results) - len(bad)} of {len(results)} files equal")
sys.exit(1 if bad or not results else 0)
