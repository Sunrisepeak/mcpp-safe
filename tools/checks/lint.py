#!/usr/bin/env python3
"""Source rules a compiler does not enforce, checked over libmc++ and its consumers.

    tools/checks/lint.py [ROOT...]        default: the repository (src modules plugins tools, manifests)

The rules: clang-exposure, platform-exposure, json-brace-init (below).

clang-exposure
    Clang and LLVM are named in two packages only: modules/backend/clang and
    modules/backend/clang-compiler. Anywhere else, an #include of <clang/...> or <llvm/...> and a
    dependency on an llvm.* package are errors: code above the backend sees MSA, facts and the
    facades (modules/backend/README.md). The root manifest's version settlement of llvm.clang-dev
    (a table mcpp asks the root for) is the one exception.

The compiler catches this itself now (plugins/json, in every mcxx compile and in the editor); this
check stays for code not yet built by mcxx.

platform-exposure (A0.2.2, A0.2.4)
    The platform is named in modules/os/* and modules/arch/* only: anywhere else, an import of an
    openkal module or an #include of an openkal header, and a preprocessor test of a platform macro
    (_WIN32, __linux__, __APPLE__, __x86_64__, __aarch64__, __ELF__, _MSC_VER, ...), are errors. Code
    branches on mcxx.os and mcxx.arch constants with `if constexpr` (plan P9).

json-brace-init
    `Json x { expr };` (and nlohmann::json, a member's `{ expr }` default) is list-initialization:
    nlohmann makes a ONE-ELEMENT ARRAY of expr, never a copy of it. `Json x { nullptr }` is `[null]`;
    `Json id { other.id }` is `[id]`. Write `Json x = expr;`. An object literal, `Json x { { "k", v } }`,
    starts with a second brace and is not affected.
"""
import os
import pathlib
import re
import sys

SKIP_DIRS = {"target", ".mcpp", ".deps", ".git", "forks", "node_modules"}


def walk(root: pathlib.Path, names):
    """Files under root whose suffix (or name) is in names, never entering build output or stores."""
    if root.is_file():
        yield root
        return
    for dirpath, dirnames, filenames in os.walk(root):
        dirnames[:] = sorted(d for d in dirnames if d not in SKIP_DIRS)
        for f in sorted(filenames):
            p = pathlib.Path(dirpath) / f
            if p.suffix in names or f in names:
                yield p

JSON = r"(?:Json|nlohmann::json|nlohmann::ordered_json)"
BRACE_INIT = re.compile(r"\b" + JSON + r"\s+[A-Za-z_][A-Za-z0-9_]*\s*\{(?!\s*[{}])")


def strip_comment(line: str) -> str:
    # Good enough for this rule: a `//` outside a string literal ends the code on the line, and what
    # a literal holds is not code (a test's source text) -- it is blanked.
    out, quote = [], None
    i = 0
    while i < len(line):
        c = line[i]
        if quote:
            if c == "\\" and i + 1 < len(line):
                out.append("  ")
                i += 2
                continue
            if c == quote:
                quote = None
                out.append(c)
            else:
                out.append(" ")
        elif c in "\"'":
            quote = c
            out.append(c)
        elif line.startswith("//", i):
            break
        else:
            out.append(c)
        i += 1
    return "".join(out)


CLANG_PACKAGES = ("modules/backend/clang/", "modules/backend/clang-compiler/")
CLANG_INCLUDE = re.compile(r'^\s*#\s*include\s*[<"](clang|llvm|clang-c|llvm-c)/')


def exposure(path: pathlib.Path) -> bool:
    """Whether this file may name Clang: inside the two backend packages (or not in this repository)."""
    text = path.resolve().as_posix()
    root = pathlib.Path(__file__).resolve().parents[2].as_posix() + "/"
    if not text.startswith(root):
        return True
    rel = text[len(root):]
    return rel.startswith(CLANG_PACKAGES) or rel.startswith(("forks/", ".deps/", "index/"))


PLATFORM_PACKAGES = ("modules/os/", "modules/arch/")
OPENKAL = re.compile(r'^\s*(export\s+)?import\s+openkal[.;\s]|^\s*#\s*include\s*[<"]openkal/')
PLATFORM_MACRO = re.compile(r'^\s*#\s*(if|ifdef|ifndef|elif)\b.*\b(_WIN32|_WIN64|__linux__|__linux|__APPLE__|__MACH__|__x86_64__|__aarch64__|'
                            r'__arm__|__i386__|__ELF__|_MSC_VER|__unix__|__unix|__FreeBSD__|__ANDROID__|__MINGW32__|__MINGW64__|__EMSCRIPTEN__)\b')


def platform(path: pathlib.Path) -> bool:
    """Whether this file may name the platform: inside modules/os or modules/arch (or not ours)."""
    text = path.resolve().as_posix()
    root = pathlib.Path(__file__).resolve().parents[2].as_posix() + "/"
    if not text.startswith(root):
        return True
    rel = text[len(root):]
    return rel.startswith(PLATFORM_PACKAGES) or rel.startswith(("forks/", ".deps/", "index/"))


def manifest_problems(root: pathlib.Path):
    problems = []
    for path in walk(root, {"mcpp.toml"}):
        if exposure(path):
            continue
        rel = path.resolve().relative_to(pathlib.Path(__file__).resolve().parents[2]).as_posix()
        table = None
        for n, line in enumerate(path.read_text().splitlines(), 1):
            stripped = line.split("#", 1)[0].strip()
            if stripped.startswith("["):
                table = stripped
                continue
            if not stripped or "=" not in stripped:
                continue
            key = stripped.split("=", 1)[0].strip().strip('"')
            under_llvm = table in ("[dependencies.llvm]", "[dev-dependencies.llvm]")
            if under_llvm and rel == "mcpp.toml" and key == "clang-dev":
                continue   # the root's version settlement
            if under_llvm or key.startswith("llvm."):
                problems.append(f"{path}:{n}: clang-exposure: a dependency on llvm.{key.removeprefix('llvm.')} outside modules/backend/clang*\n    {line.strip()}")
    return problems


def main() -> int:
    roots = [pathlib.Path(a) for a in sys.argv[1:]]
    repo = pathlib.Path(__file__).resolve().parents[2]
    if not roots:
        roots = [repo / d for d in ("src", "modules", "plugins", "tools")]
    problems = []
    for root in roots:
        for path in walk(root, {".cpp", ".cppm", ".h", ".hpp"}):
            lines = path.read_text(errors="replace").splitlines()
            code = "\n".join(strip_comment(line) for line in lines)   # one text, so a literal may span lines
            for m in BRACE_INIT.finditer(code):
                n = code.count("\n", 0, m.start()) + 1
                problems.append(f"{path}:{n}: json-brace-init: `Json x {{ expr }}` makes a one-element array; write `Json x = expr;`\n    {lines[n - 1].strip()}")
            if not exposure(path):
                for n, line in enumerate(lines, 1):
                    if CLANG_INCLUDE.match(line):
                        problems.append(f"{path}:{n}: clang-exposure: Clang/LLVM is named outside modules/backend/clang*\n    {line.strip()}")
            if not platform(path):
                for n, line in enumerate(lines, 1):
                    if OPENKAL.match(line) or PLATFORM_MACRO.match(strip_comment(line)):
                        problems.append(f"{path}:{n}: platform-exposure: the platform is named outside modules/os, modules/arch\n    {line.strip()}")
    if not sys.argv[1:]:
        problems += manifest_problems(repo)
    for p in problems:
        print(p)
    print(f"lint: {len(problems)} problem(s)" if problems else "lint: ok")
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
