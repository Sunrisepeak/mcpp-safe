#!/usr/bin/env python3
"""M2.2: MC++'s own front end resolves what a file imports from the modules' MC2 interfaces alone.

    python3 tools/checks/imports.py --mcxx MCXX --lexdump MCXX_LEXDUMP --probe MCXX_PROBE --resource DIR [--work DIR]

A2.2.1: module `shapes` is compiled by mcxx to a BMI, and its interface (shapes.ifc) is written beside
it. The importer's names are resolved by `mcxx-lexdump --references` (mcxx.frontend:lookup, the
interface found beside the BMI a -fprebuilt-module-path names). Then shapes' source is deleted and
its BMI emptied -- the front end reads neither and builds nothing -- and every answer is the same.
A2.2.3: a member of what a function template returns, its type deduced from the body (`auto`), is not
guessed: the front end says it is uncertain ("deduced"), and the Clang backend (mcxx-probe, the
service's other engine) answers that very name.
The work directory is outside the repository (default ~/.cache/mcxx-checks/imports). Exits non-zero
on any failure.
"""
import json, os, pathlib, shutil, subprocess, sys

def arg(name, default=None):
    return sys.argv[sys.argv.index(f"--{name}") + 1] if f"--{name}" in sys.argv else default

mcxx = str(pathlib.Path(arg("mcxx")).absolute())   # links kept: mcxx selects its mode by the name it is started by
lexdump = str(pathlib.Path(arg("lexdump")).resolve())
probe = str(pathlib.Path(arg("probe")).resolve())
resource = arg("resource")
work = pathlib.Path(arg("work", pathlib.Path.home() / ".cache/mcxx-checks/imports"))
failures = 0


def check(label, ok, detail=""):
    global failures
    print(f"{'PASS' if ok else 'FAIL'}  {label}{('  ' + detail) if detail and not ok else ''}")
    failures += 0 if ok else 1


SHAPES = """export module shapes;
export namespace shapes {
struct Point { int x; int y; };
struct Circle : Point { int radius; };
using Center = Point;
int area(const Circle& c);
Circle unit();
enum class Kind { point, circle };
template <class T> struct Box { T value; };
template <class T> auto boxed(T v) { return Box<T> { v }; }
}
"""
MAIN = """import shapes;
int use() {
    shapes::Circle c = shapes::unit();
    shapes::Center at { 1, 2 };
    auto made = shapes::unit();
    shapes::Kind k = shapes::Kind::circle;
    return shapes::area(c) + c.radius + c.x + at.y + made.radius + static_cast<int>(k) + shapes::boxed(3).value;
}
"""

shutil.rmtree(work, ignore_errors=True)
(work / "prebuilt").mkdir(parents=True)
(work / "shapes.cppm").write_text(SHAPES)
(work / "main.cpp").write_text(MAIN)
target = "--target=x86_64-unknown-linux-gnu"
built = subprocess.run([mcxx, "c++", "-std=c++23", target, "--precompile", "shapes.cppm", "-o", "prebuilt/shapes.pcm"], cwd=work,
                       capture_output=True, text=True)
check("mcxx builds shapes' BMI and writes its interface beside it", built.returncode == 0 and (work / "prebuilt/shapes.ifc").exists(), built.stderr[:300])


def lookup():
    run = subprocess.run([lexdump, "--references", "--target", "x86_64-unknown-linux-gnu", f"-fprebuilt-module-path={work / 'prebuilt'}",
                          str(work / "main.cpp")], capture_output=True, text=True)
    return json.loads(run.stdout) if run.returncode == 0 and run.stdout.startswith("{") else None


def resolved():
    found = lookup()
    if found is None:
        return None
    return sorted((tuple(r["range"][:2]), r["name"], r["target"], r["kind"]) for r in found["references"])


before = resolved()
# A2.2.3, while the module's source and BMI are there for the Clang backend.
uncertain = (lookup() or {}).get("uncertain", [])
value = [u for u in uncertain if u["name"] == "value"]
check("A2.2.3: a member of what a function template returns (its type deduced) is said uncertain, not guessed",
      len(value) == 1 and value[0]["why"] == "deduced" and not any(n == "value" for _, n, _, _ in (before or [])), str(uncertain))
commands = [{"directory": str(work), "file": str(work / f), "arguments": ["clang++", "-std=c++23", target, "-c", str(work / f)]}
            for f in ("shapes.cppm", "main.cpp")]
(work / "db").mkdir()
(work / "db/compile_commands.json").write_text(json.dumps(commands))
clang = subprocess.run([probe, "--db", str(work / "db"), "--resource", resource, "--cache", str(work / "probe-cache"), "--references", str(work / "main.cpp")],
                       capture_output=True, text=True)
answers = next((json.loads(l) for l in clang.stdout.splitlines() if l.startswith("{") and '"references"' in l), {"references": []})["references"]
at = tuple(value[0]["range"][:2]) if value else None
check("the Clang backend answers that name: shapes::Box::value",
      any(tuple(r["range"][:2]) == at and r["target"].split("<")[0] == "shapes::Box" + "::value" for r in answers) if at else False,
      clang.stderr[-300:])
expected = { ("Circle", "shapes::Circle"), ("unit", "shapes::unit"), ("Center", "shapes::Center"), ("area", "shapes::area"),
             ("radius", "shapes::Circle::radius"), ("x", "shapes::Point::x"), ("y", "shapes::Point::y"), ("Kind", "shapes::Kind"),
             ("circle", "shapes::Kind::circle") }
found = { (name, target) for _, name, target, _ in (before or []) }
missing = sorted(expected - found)
check("the importer's names resolve from the interface: classes, an alias, functions, a base's member, auto, an enumeration",
      before is not None and not missing, f"missing {missing}")

# Nothing but the interface is read: the source goes, the BMI is emptied.
(work / "shapes.cppm").unlink()
(work / "prebuilt/shapes.pcm").write_bytes(b"")
after = resolved()
check("A2.2.1: with shapes' source deleted and its BMI emptied, every answer is the same", after == before,
      f"{len(before or [])} before, {len(after or [])} after")
# And nothing was built to answer.
check("nothing was built for it: no BMI or object beside the importer (the Clang backend's cache apart)",
      not any(p.suffix in (".pcm", ".o") and p.stat().st_size > 0 for p in work.rglob("*") if "probe-cache" not in p.parts))
print(f"\n{failures} failed")
sys.exit(1 if failures else 0)
