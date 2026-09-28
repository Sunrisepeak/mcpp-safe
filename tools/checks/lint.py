#!/usr/bin/env python3
"""Source rules a compiler does not enforce, checked over libmc++ and its consumers.

    tools/checks/lint.py [ROOT...]        default: modules tools

json-brace-init
    `Json x { expr };` (and nlohmann::json, a member's `{ expr }` default) is list-initialization:
    nlohmann makes a ONE-ELEMENT ARRAY of expr, never a copy of it. `Json x { nullptr }` is `[null]`;
    `Json id { other.id }` is `[id]`. Write `Json x = expr;`. An object literal, `Json x { { "k", v } }`,
    starts with a second brace and is not affected.
"""
import pathlib
import re
import sys

JSON = r"(?:Json|nlohmann::json|nlohmann::ordered_json)"
BRACE_INIT = re.compile(r"\b" + JSON + r"\s+[A-Za-z_][A-Za-z0-9_]*\s*\{(?!\s*[{}])")


def strip_comment(line: str) -> str:
    # Good enough for this rule: a `//` outside a string literal ends the code on the line.
    out, quote = [], None
    i = 0
    while i < len(line):
        c = line[i]
        if quote:
            out.append(c)
            if c == "\\" and i + 1 < len(line):
                out.append(line[i + 1])
                i += 2
                continue
            if c == quote:
                quote = None
        elif c in "\"'":
            quote = c
            out.append(c)
        elif line.startswith("//", i):
            break
        else:
            out.append(c)
        i += 1
    return "".join(out)


def main() -> int:
    roots = [pathlib.Path(a) for a in sys.argv[1:]] or [pathlib.Path("modules"), pathlib.Path("tools")]
    problems = []
    for root in roots:
        files = [root] if root.is_file() else sorted(p for p in root.rglob("*") if p.suffix in (".cpp", ".cppm", ".h", ".hpp")
                                                    and "target" not in p.parts)
        for path in files:
            lines = path.read_text(errors="replace").splitlines()
            code = "\n".join(strip_comment(line) for line in lines)   # one text, so a literal may span lines
            for m in BRACE_INIT.finditer(code):
                n = code.count("\n", 0, m.start()) + 1
                problems.append(f"{path}:{n}: json-brace-init: `Json x {{ expr }}` makes a one-element array; write `Json x = expr;`\n    {lines[n - 1].strip()}")
    for p in problems:
        print(p)
    print(f"lint: {len(problems)} problem(s)" if problems else "lint: ok")
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
