#!/usr/bin/env python3
"""MC++'s own front end against Clang's tests (the conformance measurement of the roadmap, M3 "cxxtests"):
Clang's test/CXX, Parser, SemaCXX, SemaTemplate, Preprocessor and Lexer at a pinned revision, read (never copied into
the repository: Apache-2.0 WITH LLVM-exception) from a cache directory, each test's RUN lines and `expected-*`
directives read, MC++'s front end (tools/lexdump) run on every test the way a RUN line says, and what it can be
judged on today counted per directory and per standard section (CXX/<section>).

    python3 tools/checks/cxxtests.py --lexdump MCXX_LEXDUMP [--clang CLANGXX] [--cache DIR] [--rev TAG] [--only SUBSTRING]
                                     [--jobs N] [--report FILE] [--min RATE] [--depth N] [--show N] [--fetch-only] [--no-reference] [--match-text]

    MCXX_LEXDUMP  tools/lexdump: `--diagnostics` runs the preprocessor and the parser and prints their diagnostics
    CLANGXX       a reference Clang (default: xim's LLVM 23.1 under ~/.mcpp/registry/data/xpkgs/xim-x-llvm): for the
                  Lexer and Preprocessor directories, the tokens (-dump-raw-tokens) and the -E output the front end's
                  are compared with, token for token. Without one, only the diagnostics are judged
    --cache DIR   where the tests are (default ~/.cache/mcxx/clang-tests/REV: a blobless, shallow, sparse clone of
                  https://github.com/llvm/llvm-project made on first use, about 60 MB)
    --rev TAG     the revision (default llvmorg-23.1.0)
    --only STR    the tests whose path under clang/test has STR in it
    --depth N     how many path components below CXX make a group (CXX/<section> is 1; default 1)
    --report FILE the JSON report: the totals, every group, every test judged with the reasons
    --min RATE    exit non-zero when the share of tests that should be accepted and are is under RATE (0..1)
    --jobs N      processes at a time (default 2)
    --show N      how many false errors to print (default 10)
    --no-reference  do not run the reference Clang: only the diagnostics are judged
    --match-text  a directive's {{text}} must be in the diagnostic's message too (for a family whose wording follows Clang's;
                  today's `parse` does not, so it is off)

What a test is judged on. Each RUN line that runs the compiler over the test (`%clang_cc1`, `%clang`, `%clangxx`;
`%s` the input) is one *instance*, with its standard, -D and -U, `-E`, and `-verify` prefixes. The `expected-error`,
`-warning`, `-note`, `-remark` and `-no-diagnostics` directives of those prefixes (`@+N`, `@-N`, `@N`, counts `N` and
`N+`, `{{text}}`, `-re`; one in a group the preprocessor skips is not one, as Clang does not see it) say where Clang
reports what. An instance is

    accept            Clang reports no error: no `expected-error` for its prefixes (or no `-verify`, and no `not`)
    reject            Clang reports errors, each at the line of a directive
    reject-unlocated  `not %clang_cc1` without -verify: Clang fails, and where is not said
    skip              not C++ as this front end reads it: C, Objective-C, CUDA, OpenCL, HLSL, OpenMP, Microsoft
                      extensions, C++98 and C++03, or a RUN line that does not run on `%s`

What the front end says is a list of diagnostics, each in a *family* (FAMILIES): `parse` for the errors the preprocessor
and the parser report. A family either claims every diagnostic of Clang's in it (`complete`) or only the ones it
reports. Today `parse` is not complete (the parser reports where it cannot follow; it does not recognize every error),
so an accept instance is judged on whether the front end reports an error (a false error: a failure), and a reject
instance on the errors it reports at a line where Clang expects none (a false error again; an expected error it does
not report is not counted -- `missing` is for a complete family). Gate diagnostics (those naming an MC1 feature) are
not parse errors and are not judged. A later milestone that gives semantic diagnostics (M3.3 expressions, M3.6 constant
evaluation) adds a family to FAMILIES with its `complete` claim, its source in the front end's output and the
directives it answers for: `compare()` then also counts what is not matched, and the table gets a column; the line by
line comparison with the `expected-*` directives is already `compare()`'s.

For the Lexer and Preprocessor directories, with a reference Clang: MC++'s lexer against Clang's -dump-raw-tokens
(C++23, comments included; kind, place and text), and MC++'s preprocessor against Clang's -E (the main file's tokens,
`mcxx-lexdump --ppdiff`, as tools/checks/ppdiff.py does), on one C++ instance of each file whose standard is the default
or C++23 or later, read as C++23.
"""
import collections, concurrent.futures, dataclasses, json, os, pathlib, re, shlex, subprocess, sys, tempfile, time


def arg(name, default=None):
    return sys.argv[sys.argv.index(f"--{name}") + 1] if f"--{name}" in sys.argv else default


REV = arg("rev", "llvmorg-23.1.0")
REPOSITORY = "https://github.com/llvm/llvm-project"
DIRECTORIES = ("CXX", "Parser", "SemaCXX", "SemaTemplate", "Preprocessor", "Lexer")
SPARSE = [f"clang/test/{d}" for d in DIRECTORIES]
SUFFIXES = {".cpp", ".cc", ".cxx", ".c++", ".cppm", ".ixx", ".h", ".hpp", ".hh", ".hxx", ".c", ".m", ".mm", ".cu", ".hip", ".cl", ".hlsl"}
TOKEN_DIRECTORIES = ("Lexer", "Preprocessor")      # where the reference Clang is asked for tokens and -E output
MATCH_TEXT = "--match-text" in sys.argv


# ---- the tests: fetched once, read-only ----

def fetch(cache):
    """A blobless, shallow, sparse clone at the revision: only the six directories' blobs are downloaded."""
    if not (cache / ".git").exists():
        cache.parent.mkdir(parents=True, exist_ok=True)
        subprocess.run(["git", "clone", "--filter=blob:none", "--sparse", "--depth", "1", "--branch", REV, REPOSITORY, str(cache)], check=True,
                       stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    if not all((cache / d).is_dir() and any((cache / d).iterdir()) for d in SPARSE):
        subprocess.run(["git", "-C", str(cache), "sparse-checkout", "set", *SPARSE], check=True)
    return cache / "clang" / "test"


def test_files(root, only):
    """The tests' paths under clang/test: lit's tests are the files of the six directories, not an Inputs or Output directory's."""
    found = []
    for directory in DIRECTORIES:
        for path in sorted((root / directory).rglob("*")):
            relative = path.relative_to(root)
            if path.is_file() and path.suffix in SUFFIXES and not {"Inputs", "Output"} & set(relative.parts) and (not only or only in str(relative)):
                found.append(str(relative))
    return found


# ---- reading a test: comments, RUN lines, expected-* directives ----

# What is not a comment is skipped as a whole, so that a `//` in a string is not one: raw strings, strings, character
# literals (not a digit separator), and then the two kinds of comment.
LEXICAL = re.compile(r"""
      (?P<raw>(?:u8|u|U|L)?R"(?P<delimiter>[^()\\\s]{0,16})\(.*?\)(?P=delimiter)")
    | (?P<string>(?:u8|u|U|L)?"(?:[^"\\\n]|\\.)*")
    | (?P<character>(?<!\w)(?:u8|u|U|L)?'(?:[^'\\\n]|\\.)*')
    | (?P<line>//(?:[^\n\\]|\\.)*)
    | (?P<block>/\*.*?\*/)
""", re.S | re.X)


@dataclasses.dataclass
class Comment:
    line: int          # of its first character
    text: str          # without the comment's own delimiters


def comments(text):
    out = []
    for m in LEXICAL.finditer(text):
        if m.lastgroup in ("line", "block"):
            body = m.group(0)[2:-2] if m.lastgroup == "block" else m.group(0)[2:]
            out.append(Comment(text.count("\n", 0, m.start()) + 1, body))
    return out


def run_lines(text):
    """The RUN lines of a test, as lit reads them (`RUN:` anywhere in a line, not only in a comment), the ones ending in a
    backslash joined to the next."""
    lines, pending = [], None
    for raw in text.split("\n"):
        m = re.search(r"\bRUN:\s?(.*)$", raw)
        if not m:
            continue
        piece = re.sub(r"\s*\*/\s*$", "", m.group(1)).rstrip()
        if pending is not None:
            piece = pending + " " + piece
            pending = None
        if piece.endswith("\\"):
            pending = piece[:-1].rstrip()
        else:
            lines.append(piece)
    if pending:
        lines.append(pending)
    return lines


@dataclasses.dataclass
class Directive:
    prefix: str
    kind: str                  # error, warning, note, remark
    line: int | None           # in the test; None for one in another file or anywhere (`@file:N`, `@*`) or at a marker
    count: int = 1
    at_least: bool = False     # `N+`
    text: str = ""             # the {{...}}
    regex: bool = False        # `-re`: the text is a regular expression


def directives(found, prefixes):
    """The expected-* directives of the given -verify prefixes: (directives, whether `expected-no-diagnostics` is there)."""
    names = "|".join(re.escape(p) for p in prefixes)
    pattern = re.compile(rf"(?<![\w-])(?:{names})-(?:(?P<kind>error|warning|note|remark)(?P<re>-re)?(?:@(?P<loc>[^\s{{]+))?[ \t]*(?P<count>\d+\+?|\*)?[ \t]*\{{\{{(?P<text>.*?)\}}\}}"
                         rf"|(?P<none>no-diagnostics))", re.S)
    markers = {}
    for c in found:
        m = re.match(r"\s*#(\w+)", c.text)
        if m:
            markers[m.group(1)] = c.line
    out, none = [], False
    for c in found:
        for m in pattern.finditer(c.text):
            if m.group("none"):
                none = True
                continue
            own = c.line + c.text.count("\n", 0, m.start())
            loc, line = m.group("loc"), own
            if loc is not None:
                if re.fullmatch(r"[+-]\d+", loc):
                    line = own + int(loc)
                elif loc.isdigit():
                    line = int(loc)
                elif loc.startswith("#") and loc[1:] in markers:
                    line = markers[loc[1:]]
                else:
                    line = None
            count = m.group("count") or "1"
            prefix = re.match(rf"(?:{names})", m.group(0)).group(0)
            out.append(Directive(prefix, m.group("kind"), line, 0 if count == "*" else int(count.rstrip("+")), count.endswith("+") or count == "*",
                                 m.group("text"), m.group("re") is not None))
    return out, none


# ---- the RUN line as an instance ----

EXTENSION_LANGUAGE = {".c": "c", ".m": "objective-c", ".mm": "objective-c++", ".cu": "cuda", ".hip": "hip", ".cl": "opencl", ".hlsl": "hlsl",
                      ".h": "c-header"}
SKIPPED_FLAGS = ("-fobjc", "-fms-extensions", "-fms-compatibility", "-fdeclspec", "-fborland", "-fopenmp", "-fopenacc", "-fsycl", "-fcuda",
                 "-fgpu", "-fblocks", "-fopencl", "-cl-std", "-fhlsl", "-fasm-blocks", "-fmicrosoft", "-fapple", "-fdelayed-template-parsing")
PROGRAMS = ("%clang_cc1", "%clang", "%clangxx")


@dataclasses.dataclass
class Instance:
    test: str
    index: int
    std: str | None = None
    defines: list = dataclasses.field(default_factory=list)
    triple: str | None = None
    prefixes: list = dataclasses.field(default_factory=list)   # the -verify prefixes: none without -verify
    preprocess_only: bool = False                               # -E
    not_: bool = False                                          # `not`: the compiler is expected to fail
    skip: str | None = None                                     # why this front end does not judge it

    def key(self):
        return (self.std, tuple(self.defines), self.triple, tuple(self.prefixes), self.preprocess_only, self.not_)


def commands(line):
    """The commands of a RUN line, split at | && ;: lists of words (lit's substitutions as written)."""
    try:
        lexer = shlex.shlex(line, posix=True, punctuation_chars=True)
        lexer.whitespace_split = True
        words = list(lexer)
    except ValueError:
        words = line.split()
    out, current = [], []
    for w in words:
        if w in ("|", "&&", ";", "||"):
            out.append(current)
            current = []
        else:
            current.append(w)
    out.append(current)
    return [c for c in out if c]


def instance_of(test, index, words, language):
    """The instance a command makes of the test, or None when it does not run the compiler."""
    i = 0
    not_ = False
    while i < len(words) and (words[i] in ("not", "env") or ("=" in words[i] and not words[i].startswith(("-", "%")))):
        not_ = not_ or words[i] == "not"
        i += 1
    if i >= len(words) or not (words[i] in PROGRAMS or words[i].startswith("%clang_cc1")):
        return None
    ins = Instance(test, index, not_=not_)
    flags = words[i + 1:]
    if "%s" not in flags and not any(f.endswith("%s") and f.startswith("<") for f in flags):
        ins.skip = "does not run on %s"
        return ins
    lang = language
    k = 0
    while k < len(flags):
        f = flags[k]
        nxt = flags[k + 1] if k + 1 < len(flags) else ""
        if f == "-verify":
            ins.prefixes.append("expected")
        elif f.startswith("-verify="):
            ins.prefixes += f[8:].split(",")
        elif f.startswith("-std=") or f.startswith("--std="):
            ins.std = f.split("=", 1)[1]
        elif f == "-x":
            lang, k = nxt, k + 1
        elif f.startswith("-x") and len(f) > 2:
            lang = f[2:]
        elif f in ("-D", "-U") and nxt:
            ins.defines.append(f + nxt)
            k += 1
        elif f.startswith(("-D", "-U")):
            ins.defines.append(f)
        elif f in ("-triple", "-target") and nxt:
            ins.triple, k = nxt, k + 1
        elif f.startswith(("-triple=", "--target=")):
            ins.triple = f.split("=", 1)[1]
        elif f == "-E":
            ins.preprocess_only = True
        elif f.startswith(SKIPPED_FLAGS) and not ins.skip:
            ins.skip = f"{f.split('=')[0]}"
        k += 1
    std = ins.std or ""
    if lang not in ("c++", "c++-header", "c++-module", "c++-user-header", "c++-system-header", "c++-header-unit-header", "c++-cpp-output", "c++26", "c++2c"):
        ins.skip = ins.skip or f"language {lang}"
    elif re.fullmatch(r"(?:gnu|c)\+\+(?:98|03)", std):
        ins.skip = ins.skip or "C++98/03"
    elif std and not re.fullmatch(r"(?:gnu|c)\+\+\w+", std):
        ins.skip = ins.skip or f"-std={std}"
    return ins


def instances_of(test, language, text):
    out, seen = [], set()
    for n, line in enumerate(run_lines(text)):
        if "%if" in line:
            continue
        for words in commands(line):
            ins = instance_of(test, n, words, language)
            if ins is not None and ins.key() not in seen:
                seen.add(ins.key())
                out.append(ins)
    return out


# ---- running the front end ----

def standard_of(ins):
    """-std as lexdump takes it; the default of Clang's cc1 for C++ is gnu++17."""
    return ins.std or "gnu++17"


def front_end(lexdump, path, ins, resource_dirs):
    command = [lexdump, "--diagnostics", "--std", standard_of(ins), *ins.defines, *(["--preprocess-only"] if ins.preprocess_only else []),
               *(a for d in resource_dirs for a in ("-I", d)), path]
    run = subprocess.run(command, capture_output=True)
    try:
        return json.loads(run.stdout)
    except ValueError:
        return {"failed": run.stderr.decode("utf-8", "replace")[-300:] or f"exit {run.returncode}", "diagnostics": [], "skipped": []}


# ---- the comparison with the directives: what a later milestone adds a family to ----

@dataclasses.dataclass
class Family:
    """A family of the front end's diagnostics and the part of Clang's it answers for."""
    sources: tuple                  # the `family` strings of lexdump's diagnostics it takes
    kinds: tuple                    # the directive kinds it answers for
    complete: bool                  # every directive of its kinds has a diagnostic: what is not matched is a miss
    answers: object = None          # a directive predicate (None: all); how a later family leaves the parser's to the parser


FAMILIES = {
    # What the lexer, the preprocessor and the parser say. Not complete: the parser reports where it cannot follow, it
    # does not claim every syntax error Clang has (M3.0 will, with the parser that follows the whole grammar).
    "parse": Family(sources=("preprocess", "parse"), kinds=("error",), complete=False),
    # M3.3, M3.6: Family(sources=("sema",), kinds=("error", "warning", "note"), complete=True, answers=semantic) ...
}

SYNTAX_MESSAGE = re.compile(r"expected |unexpected |cannot be (?:used|declared)|invalid (?:token|character)|extraneous|missing '|stray |unterminated")


def syntactic(directive):
    """Whether Clang's message is the parser's: `expected ';'`, `unexpected ...` (a rough rule for the table's information)."""
    return bool(SYNTAX_MESSAGE.search(directive.text))


def directive_matches(d, diagnostic, match_text):
    if d.kind != diagnostic["severity"] and not (d.kind == "remark" and diagnostic["severity"] == "note"):
        return False
    if not match_text or not d.text:
        return True
    return re.search(d.text, diagnostic["message"]) is not None if d.regex else d.text in diagnostic["message"]


def compare(expected, actual, family, match_text=False):
    """The front end's diagnostics of one family against the directives it answers for, line by line.

    matched     (directive, diagnostic) pairs: the same severity at the same line (with `match_text`, text too)
    missing     directives no diagnostic answers (counted only for a complete family: otherwise the front end never claimed it)
    unexpected  diagnostics at a line where no directive of that severity is: false errors
    """
    mine = [d for d in expected if d.kind in family.kinds and (family.answers is None or family.answers(d)) and d.line is not None]
    diagnostics = [x for x in actual if x["family"] in family.sources and x["severity"] in family.kinds]
    by_line = collections.defaultdict(list)
    for x in diagnostics:
        by_line[x["line"]].append(x)
    matched, missing, used = [], [], set()
    for d in mine:
        found = 0
        for x in by_line.get(d.line, []):
            if (found == d.count and not d.at_least) or id(x) in used or not directive_matches(d, x, match_text):
                continue
            used.add(id(x))
            matched.append((d, x))
            found += 1
        if found < d.count and family.complete:
            missing.append(d)
    unexpected = [x for x in diagnostics if id(x) not in used and not any(d.line == x["line"] and d.kind == x["severity"] for d in expected)]
    return matched, missing, unexpected


def judge(ins, found, lexdump, path, resource_dirs):
    """The record of one instance: its class, and what the front end said against what Clang expects."""
    record = {"run": ins.index, "std": ins.std, "prefixes": ins.prefixes, "E": ins.preprocess_only}
    if ins.skip:
        return {**record, "class": "skip", "why": ins.skip}
    said = front_end(lexdump, path, ins, resource_dirs)
    if "failed" in said:
        return {**record, "class": "skip", "why": "front end failed: " + said["failed"]}
    skipped = [tuple(g) for g in said["skipped"]]
    expected, none = directives(found, ins.prefixes) if ins.prefixes else ([], False)
    # Clang's lexer hands a comment in a group the preprocessor skips to nobody.
    expected = [d for d in expected if d.line is None or not any(a <= d.line <= b for a, b in skipped)]
    errors = [d for d in expected if d.kind == "error"]
    gate = [x for x in said["diagnostics"] if x.get("feature")]
    said["diagnostics"] = [x for x in said["diagnostics"] if not x.get("feature")]
    cls = "reject-unlocated" if ins.not_ and not ins.prefixes else "reject" if errors else "accept"
    matched, missing, unexpected = compare(expected, said["diagnostics"], FAMILIES["parse"], MATCH_TEXT)
    if cls == "accept":
        unexpected = [x for x in said["diagnostics"] if x["severity"] == "error" and x["family"] in FAMILIES["parse"].sources]
    elif cls == "reject-unlocated":
        unexpected = []          # Clang fails and does not say where: nothing the front end reports is false
    return {**record, "class": cls, "certain": said["certain"], "expected_errors": len(errors), "expected_syntax": sum(1 for d in errors if syntactic(d)),
            "reported": sum(1 for x in said["diagnostics"] if x["severity"] == "error"), "matched": len(matched), "missing": len(missing),
            "false_errors": [{"line": x["line"], "column": x["column"], "message": x["message"], "family": x["family"]} for x in unexpected],
            "gate": len(gate)}


# ---- the reference Clang for the Lexer and Preprocessor directories ----

def default_clang():
    found = sorted(pathlib.Path(os.path.expanduser("~/.mcpp/registry/data/xpkgs/xim-x-llvm")).glob("23.*/bin/clang++"))
    return str(found[-1]) if found else None


def clang_tokens(clang, path):
    """Clang's raw tokens under C++23 as lexdump prints ours: (kind, line, column, text), whitespace aside, comments kept."""
    run = subprocess.run([clang, "-Xclang", "-dump-raw-tokens", "-fsyntax-only", "-std=c++23", "-x", "c++", path], capture_output=True)
    data = pathlib.Path(path).read_bytes()
    starts = [0]
    for m in re.finditer(rb"\r\n|\n|\r", data):
        starts.append(m.end())
    found = []
    for raw in run.stderr.decode("utf-8", "replace").splitlines():
        m = re.match(r"(\w+)\s+'.*'\s+Loc=<.*:(\d+):(\d+)>", raw)
        if m:
            found.append((m.group(1), int(m.group(2)), int(m.group(3))))
    out = []
    for n, (kind, line, column) in enumerate(found):
        begin = starts[line - 1] + column - 1 if line - 1 < len(starts) else len(data)
        nline, ncolumn = found[n + 1][1:] if n + 1 < len(found) else (None, None)
        end = starts[nline - 1] + ncolumn - 1 if nline is not None and nline - 1 < len(starts) else len(data)
        piece = data[begin:end].decode("utf-8", "replace")
        if kind == "unknown" and not re.sub(r"\\(?:\r\n|\n|\r)", "", piece).strip():
            continue                                  # whitespace (a splice among it): Clang's raw lexer prints it as unknown
        out.append((kind, line, column, piece))
    return out


def lexer_agreement(lexdump, clang, path):
    ours = subprocess.run([lexdump, path], capture_output=True).stdout.decode("utf-8", "replace").splitlines()
    mine = []
    for line in ours:
        t = json.loads(line)
        mine.append((t["kind"], t["line"], t["column"], t["text"]))
    theirs = clang_tokens(clang, path)
    if mine == theirs:
        return {"verdict": "equal", "tokens": len(theirs)}
    at = next((i for i, (a, b) in enumerate(zip(mine, theirs)) if a != b), min(len(mine), len(theirs)))
    return {"verdict": "differ", "tokens": len(theirs), "at": at, "ours": mine[at] if at < len(mine) else None, "clang": theirs[at] if at < len(theirs) else None}


def preprocessor_agreement(lexdump, clang, path, ins, scratch):
    defines = [d for d in ins.defines]
    output = os.path.join(scratch, f"{abs(hash(path))}.i")
    directory = os.path.dirname(path)
    include = ["-I", directory, "-I", os.path.join(directory, "Inputs")]
    run = subprocess.run([clang, "-E", "-std=c++23", "-x", "c++", *defines, *include, path, "-o", output], capture_output=True, text=True)
    if "fatal error" in run.stderr or not os.path.exists(output):
        return {"verdict": "clang-failed", "note": run.stderr.strip().splitlines()[0][:200] if run.stderr.strip() else ""}
    ours = subprocess.run([lexdump, "--ppdiff", "--std", "c++23", *defines, "-I", directory, "-I", os.path.join(directory, "Inputs"), path, output],
                          capture_output=True, text=True)
    os.unlink(output)
    try:
        return json.loads(ours.stdout)
    except ValueError:
        return {"verdict": "lexdump-failed", "note": ours.stderr[-200:]}


# ---- the tests, one at a time ----

def check(relative, root, lexdump, clang, scratch):
    path = str(root / relative)
    text = pathlib.Path(path).read_bytes().decode("latin-1")
    found = comments(text)
    language = EXTENSION_LANGUAGE.get(pathlib.Path(relative).suffix, "c++")
    directory = os.path.dirname(path)
    resource_dirs = [directory, os.path.join(directory, "Inputs")]
    record = {"test": relative, "directory": relative.split("/")[0], "instances": []}
    instances = instances_of(relative, language, text)
    for ins in instances:
        record["instances"].append(judge(ins, found, lexdump, path, resource_dirs))
    judged = [(ins, r) for ins, r in zip(instances, record["instances"]) if r["class"] != "skip"]
    record["judged"] = bool(judged)
    if clang and record["directory"] in TOKEN_DIRECTORIES and judged:
        # One C++ instance whose standard is the default or C++23 or later, read as C++23.
        modern = [ins for ins, _ in judged if not ins.std or re.fullmatch(r"(?:gnu|c)\+\+(?:2[3-9]|2[bcd])", ins.std)]
        if modern:
            record["lexer"] = lexer_agreement(lexdump, clang, path)
            record["preprocessor"] = preprocessor_agreement(lexdump, clang, path, modern[0], scratch)
    return record


def group_names(record, depth):
    parts = record["test"].split("/")
    names = [parts[0]]
    if parts[0] == "CXX" and len(parts) > 2:
        names.append("/".join(parts[:1 + depth]))
    return names


def summarize(records, depth):
    groups = collections.OrderedDict()

    def slot(name):
        return groups.setdefault(name, collections.Counter())

    for r in records:
        for name in group_names(r, depth):
            g = slot(name)
            g["files"] += 1
            if not r["judged"]:
                g["skipped"] += 1
                continue
            g["judged"] += 1
            judged = [i for i in r["instances"] if i["class"] != "skip"]
            classes = {i["class"] for i in judged}
            # A false error the front end itself said it was not certain of (a header's macro, a __has_cpp_attribute it
            # cannot answer) is its own count: the answer is the compiler's then, plan P4.
            failures = [i for i in judged if i.get("false_errors") and i.get("certain", True)]
            doubtful = [i for i in judged if i.get("false_errors") and not i.get("certain", True)]
            if classes == {"accept"}:
                g["accept"] += 1
                g["accept_ok"] += 0 if failures or doubtful else 1
                g["accept_uncertain"] += 1 if doubtful and not failures else 0
            if "reject" in classes:
                g["reject"] += 1
                g["reject_false"] += 1 if failures else 0
            if "reject-unlocated" in classes:
                g["reject_unlocated"] += 1
            if failures:
                g["false_error_files"] += 1
            if doubtful and not failures:
                g["uncertain_error_files"] += 1
            g["expected_errors"] += sum(i.get("expected_errors", 0) for i in r["instances"])
            g["expected_syntax"] += sum(i.get("expected_syntax", 0) for i in r["instances"])
            g["matched"] += sum(i.get("matched", 0) for i in r["instances"])
            g["uncertain"] += 1 if any(i.get("certain") is False for i in r["instances"]) else 0
            if "lexer" in r:
                g["tokens_files"] += 1
                g["tokens_equal"] += r["lexer"]["verdict"] == "equal"
                p = r["preprocessor"]["verdict"]
                g["pp_files"] += 1
                g[f"pp_{p.replace('-', '_')}"] += 1
    return groups


def rate(a, b):
    return f"{100 * a / b:5.1f}%" if b else "    -"


def table(groups, with_tokens):
    header = f"{'group':<26}{'files':>6}{'judged':>7}{'accept':>7}{'ok':>6}{'unsure':>7}{'rate':>7}{'reject':>7}{'false':>6}{'unsure':>7}{'unlocated':>10}"
    if with_tokens:
        header += f"{'tokens eq':>11}{'pp equal':>9}{'pp uncert':>10}{'pp defect':>10}{'pp n/a':>8}"
    lines = [header, "-" * len(header)]
    for name, g in groups.items():
        line = (f"{name:<26}{g['files']:>6}{g['judged']:>7}{g['accept']:>7}{g['accept_ok']:>6}{g['accept_uncertain']:>7}{rate(g['accept_ok'], g['accept']):>7}"
                f"{g['reject']:>7}{g['false_error_files']:>6}{g['uncertain_error_files']:>7}{g['reject_unlocated']:>10}")
        if with_tokens:
            line += (f"{(str(g['tokens_equal']) + '/' + str(g['tokens_files'])) if g['tokens_files'] else '-':>11}"
                     f"{g['pp_equal'] if g['pp_files'] else '-':>9}{g['pp_uncertain'] + g['pp_header_macro'] if g['pp_files'] else '-':>10}"
                     f"{g['pp_certain'] if g['pp_files'] else '-':>10}{g['pp_clang_failed'] + g['pp_lexdump_failed'] if g['pp_files'] else '-':>8}")
        lines.append(line)
    return "\n".join(lines)


def main():
    cache = pathlib.Path(arg("cache", os.path.expanduser(f"~/.cache/mcxx/clang-tests/{REV}"))).resolve()
    root = fetch(cache)
    only = arg("only")
    files = test_files(root, only)
    if "--fetch-only" in sys.argv:
        print(f"cxxtests: {len(files)} tests under {root}")
        return 0
    lexdump = str(pathlib.Path(arg("lexdump")).resolve())
    clang = arg("clang") or default_clang()
    if "--no-reference" in sys.argv:
        clang = None
    jobs = int(arg("jobs", 2))
    depth = int(arg("depth", 1))
    show = int(arg("show", 10))
    minimum = float(arg("min")) if arg("min") else None
    started = time.perf_counter()
    scratch = tempfile.mkdtemp(prefix="mcxx-cxxtests-")
    try:
        with concurrent.futures.ThreadPoolExecutor(max_workers=jobs) as pool:
            records = list(pool.map(lambda f: check(f, root, lexdump, clang, scratch), files))
    finally:
        for f in os.listdir(scratch):
            os.unlink(os.path.join(scratch, f))
        os.rmdir(scratch)
    groups = summarize(records, depth)
    print(table(groups, bool(clang)))
    false_files = [r for r in records if any(i.get("false_errors") and i.get("certain", True) for i in r["instances"])]
    for r in false_files[:show]:
        shown = set()
        for i in r["instances"]:
            for x in (i.get("false_errors", [])[:1] if i.get("certain", True) else []):
                if (x["line"], x["message"]) not in shown:
                    shown.add((x["line"], x["message"]))
                    print(f"false error: {r['test']}:{x['line']}:{x['column']}: {x['message'][:160]} ({x['family']}, {i['class']})")
    if len(false_files) > show:
        print(f"... and {len(false_files) - show} more files")
    sums = collections.Counter()
    for name in DIRECTORIES:
        sums.update(groups.get(name, collections.Counter()))
    seconds = time.perf_counter() - started
    print(f"cxxtests {REV}: {sums['files']} tests, {sums['judged']} judged ({sums['skipped']} not C++ for this front end); "
          f"{sums['accept_ok']} of {sums['accept']} that should be accepted are ({rate(sums['accept_ok'], sums['accept']).strip()}); "
          f"{sums['false_error_files']} files with a false error, {sums['uncertain_error_files']} more with one the front end was not certain of; {seconds:.0f}s")
    if report := arg("report"):
        pathlib.Path(report).write_text(json.dumps({"revision": REV, "cache": str(cache), "front_end": lexdump, "reference": clang,
                                                    "families": {n: dataclasses.asdict(dataclasses.replace(f, answers=None)) for n, f in FAMILIES.items()},
                                                    "totals": dict(sums), "groups": {n: dict(g) for n, g in groups.items()}, "tests": records},
                                                   indent=1))
    if minimum is not None and (not sums["accept"] or sums["accept_ok"] / sums["accept"] < minimum):
        print(f"cxxtests: the share accepted is under {minimum}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
