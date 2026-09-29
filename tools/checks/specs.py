#!/usr/bin/env python3
"""Checks MC++'s specifications (specs/): their machine-readable parts and their traceability (A0.8.1).

    python3 tools/checks/specs.py                  # from the repository root; needs the jsonschema package
    python3 tools/checks/specs.py --mcxx PATH      # also checks what a built mcxx prints (features --json, version --json)

Checks: every JSON file parses and every schema is valid draft 2020-12; every example validates (JSON, JSON
lines, and the [package.metadata.mcxx] table of the TOML examples); the JSON form of the MC1 example
configuration is its TOML read as JSON; the catalog has MC1 section 3's features and section 4's profiles;
negative cases the schemas must reject; relative links resolve; every requirement (a capitalized RFC 2119
keyword) carries an identifier, identifiers are unique, and every identifier has evidence in
conformance/traceability.json whose evidence exists. Exits non-zero on any failure.
"""
import json, os, pathlib, re, subprocess, sys, tomllib

from jsonschema import Draft202012Validator

repository = pathlib.Path(__file__).resolve().parents[2]
specs = repository / "specs"
failures = 0
passed = []   # labels of passing checks: evidence the traceability file may name


def check(label, ok, detail=""):
    global failures
    print(f"{'PASS' if ok else 'FAIL'}  {label}{('  ' + detail) if detail and not ok else ''}")
    if ok:
        passed.append(label)
    else:
        failures += 1


def load(path):
    return json.loads(path.read_text(encoding="utf-8"))


def lines(path):
    return [json.loads(l) for l in path.read_text(encoding="utf-8").splitlines() if l.strip()]


def validate(label, validator, doc, expect_valid=True):
    errors = sorted(validator.iter_errors(doc), key=lambda e: list(e.path))
    ok = (not errors) if expect_valid else bool(errors)
    detail = "; ".join(f"{list(e.path)}: {e.message[:160]}" for e in errors[:3]) if expect_valid else "accepted an invalid document"
    check(label, ok, detail)


mcxx = None
if "--mcxx" in sys.argv:
    mcxx = os.path.abspath(sys.argv[sys.argv.index("--mcxx") + 1])   # the live checks run in other directories

# 1. every JSON file parses; every schema is a valid draft 2020-12 schema
schemas = {}
for path in sorted(specs.rglob("*.json")):
    try:
        doc = load(path)
        check(f"parse {path.relative_to(specs)}", True)
    except Exception as e:
        check(f"parse {path.relative_to(specs)}", False, str(e))
        continue
    if path.parent.name == "schema":
        try:
            Draft202012Validator.check_schema(doc)
            check(f"schema is valid 2020-12: {path.name}", True)
        except Exception as e:
            check(f"schema is valid 2020-12: {path.name}", False, str(e))
        schemas[path.stem.removesuffix(".schema")] = Draft202012Validator(doc)

config, catalog_v, audit, facts, protocol, version, serve = (schemas[n] for n in
    ("mc1-config", "mc1-catalog", "mc1-audit", "mc3-facts", "mc4-protocol", "mc5-version", "mc6-requests"))
ex = specs / "examples"

# 2. examples validate
toml_config = tomllib.loads((ex / "mc1-config.toml").read_text(encoding="utf-8"))["package"]["metadata"]["mcxx"]
validate("MC1 example validates: mc1-config.toml", config, toml_config)
validate("MC1 example validates: mc1-config.json", config, load(ex / "mc1-config.json"))
check("MC1 example: mc1-config.json is mc1-config.toml's table", load(ex / "mc1-config.json") == toml_config)
plugins_config = tomllib.loads((ex / "mc4-plugins.toml").read_text(encoding="utf-8"))["package"]["metadata"]["mcxx"]
validate("MC4 example validates: mc4-plugins.toml", config, plugins_config)
for i, record in enumerate(lines(ex / "mc1-audit.jsonl")):
    validate(f"MC1 example validates: mc1-audit.jsonl line {i + 1}", audit, record)
validate("MC1 example validates: mc1-catalog.json", catalog_v, load(ex / "mc1-catalog.json"))
validate("MC3 example validates: mc3-facts.json", facts, load(ex / "mc3-facts.json"))
for i, message in enumerate(lines(ex / "mc4-session.jsonl")):
    validate(f"MC4 example validates: mc4-session.jsonl {message.get('type')}", protocol, message)
    if message.get("type") == "check":
        validate("MC4 example: a check's facts are MC3 facts", facts, message["facts"])
validate("MC5 example validates: mc5-version.json", version, load(ex / "mc5-version.json"))
for entry in load(ex / "mc6-session.json"):
    validate(f"MC6 example validates: mc6-session.json {entry['method']}", serve, entry)
    if entry["method"] == "mcxx/facts":
        validate("MC6 example: mcxx/facts answers MC3 facts", facts, entry["result"])

# 3. what the schemas must reject
bad = dict(toml_config, features={"goto": "loud"})
validate("MC1 schema rejects a level that is not one", config, bad, expect_valid=False)
validate("MC1 schema rejects a malformed feature id", config, dict(toml_config, features={"Goto": "deny"}), expect_valid=False)
validate("MC1 schema rejects a plugin both static and out of process", config,
         {"plugins": {"x": {"path": "p", "command": ["x"]}}}, expect_valid=False)
validate("MC1 schema rejects a plugin with no way to reach it", config, {"plugins": {"x": {"timeout-ms": 5}}}, expect_valid=False)
cat = load(ex / "mc1-catalog.json")
iso = next(f for f in cat["features"] if f["category"] == "iso")
validate("MC1 schema rejects an iso feature without stable names", catalog_v,
         dict(cat, features=[dict(iso, standard="")]), expect_valid=False)
validate("MC1 audit schema rejects a 0-based line", audit, dict(lines(ex / "mc1-audit.jsonl")[0], line=0), expect_valid=False)
f = load(ex / "mc3-facts.json")
validate("MC3 schema rejects facts without a kind's array", facts, {k: v for k, v in f.items() if k != "casts"}, expect_valid=False)
validate("MC3 schema rejects an unknown cast kind", facts, dict(f, casts=[dict(f["casts"][0], kind="bit_cast")]), expect_valid=False)
session = lines(ex / "mc4-session.jsonl")
validate("MC4 schema rejects a welcome without providers", protocol, dict(session[1], providers=[]), expect_valid=False)
validate("MC4 schema rejects a message without an id", protocol, {"type": "shutdown"}, expect_valid=False)
validate("MC4 schema rejects an unknown message type", protocol, {"type": "reload", "id": 3}, expect_valid=False)

# 4. semantics: the catalog has MC1 section 3's features and section 4's profiles
mc1 = (specs / "mc1-features.md").read_text(encoding="utf-8")
section3 = mc1[mc1.index("## 3. The built-in features"):mc1.index("## 4. Profiles")]
table = {m.group(1): m.group(2) for m in re.finditer(r"^\| `([a-z-]+)` \| ((?:\[[a-z0-9.]+\] ?)+) \|", section3, re.M)}
section4 = mc1[mc1.index("## 4. Profiles"):mc1.index("## 5. Configuration")]
profiles = {}
for m in re.finditer(r"^\| `([a-z]+)` \| [^|]+ \| (.+) \|$", section4, re.M):
    profiles[m.group(1)] = m.group(2)


def check_catalog(label, doc):
    by_id = {f["id"]: f for f in doc["features"]}
    missing = [i for i in table if i not in by_id]
    check(f"{label}: every MC1 section 3 feature is there", not missing and len(table) == 15, f"missing {missing}, table has {len(table)}")
    wrong = [i for i, std in table.items() if i in by_id and (by_id[i]["category"] != "iso" or by_id[i]["standard"] != std.strip()
                                                            or by_id[i]["default"] != "allow")]
    check(f"{label}: MC1 section 3 categories, stable names and defaults", not wrong, f"differ: {wrong}")
    got = {p["name"]: set(p["features"]) for p in doc["profiles"]}
    safe = set(re.findall(r"`([a-z-]+)`", profiles.get("safe", "")))
    check(f"{label}: profile safe is MC1 section 4's", got.get("safe") == safe, f"{sorted(got.get('safe', []))} != {sorted(safe)}")
    check(f"{label}: profile modules is MC1 section 4's", got.get("modules") == {"include"})
    check(f"{label}: profile strict is safe, modules, goto and macros", got.get("strict") == safe | {"include", "goto", "macros"})
    extensions = {f["id"] for f in doc["features"] if f["category"] == "extension"}
    check(f"{label}: profile portable is every extension", got.get("portable") == extensions and extensions)
    check(f"{label}: library ids start with lib:, extension ids with ext:",
          all(f["id"].startswith("lib:") for f in doc["features"] if f["category"] == "library") and all(i.startswith("ext:") for i in extensions))
    check(f"{label}: no conflicts", doc["problems"] == [], str(doc["problems"]))


check_catalog("MC1 example catalog", cat)

# 5. what a built mcxx prints
if mcxx:
    out = subprocess.run([mcxx, "features", "--json"], capture_output=True, text=True)
    live = json.loads(out.stdout)
    validate("mcxx features --json validates", catalog_v, live)
    check_catalog("mcxx features --json", live)
    check("mcxx features --json is the example catalog", live == cat, "regenerate specs/examples/mc1-catalog.json")
    text = subprocess.run([mcxx, "features"], capture_output=True, text=True)
    check("mcxx features exits 0 without conflicts", text.returncode == 0 and "providers" in text.stdout)
    v = json.loads(subprocess.run([mcxx, "version", "--json"], capture_output=True, text=True).stdout)
    validate("mcxx version --json validates", version, v)
    def frame(m):
        body = json.dumps(m).encode()
        return b"Content-Length: %d\r\n\r\n" % len(body) + body
    lifecycle = [{"jsonrpc": "2.0", "id": 1, "method": "initialize", "params": {}}, {"jsonrpc": "2.0", "id": 2, "method": "shutdown"},
                 {"jsonrpc": "2.0", "method": "exit"}]
    served = subprocess.run([mcxx, "serve", "--cache", "/tmp/mcxx-specs-serve"], input=b"".join(frame(m) for m in lifecycle), capture_output=True)
    check("mcxx serve: framed answers, MC6's requests named, exit 0 after shutdown",
          served.returncode == 0 and served.stdout.startswith(b"Content-Length: ") and b'"mcxx/facts"' in served.stdout)
    unshut = subprocess.run([mcxx, "serve", "--cache", "/tmp/mcxx-specs-serve"], input=frame(lifecycle[0]) + frame(lifecycle[2]), capture_output=True)
    check("mcxx serve: exit without shutdown ends with 1", unshut.returncode == 1)
    usage = subprocess.run([mcxx, "frobnicate"], capture_output=True, text=True)
    check("mcxx with an unknown command exits 2 with its usage", usage.returncode == 2 and "usage:" in usage.stderr and usage.stdout == "")
    import tempfile
    with tempfile.TemporaryDirectory(prefix="mcxx-specs-") as work:
        w = pathlib.Path(work)
        (w / "mcpp.toml").write_text('[package]\nname = "t"\nversion = "0.1.0"\n[package.metadata.mcxx.features]\ngoto = "warn"\n')
        (w / "a.cppm").write_text("export module a;\nexport int f(int n) { if (n) goto out; return 1; out: return 0; }\n")
        (w / "clean.cpp").write_text("int g(int n) { return n + 1; }\n")
        args = ["c++", "-std=c++23", "--target=x86_64-unknown-linux-gnu"]
        one = subprocess.run([mcxx, *args, "--precompile", "a.cppm", "-o", "a.pcm"], cwd=w, capture_output=True, text=True)
        two = subprocess.run([mcxx, *args, "-c", "a.pcm", "-o", "a.o"], cwd=w, capture_output=True, text=True)
        found = (one.stderr + two.stderr).count("[goto]")
        check("mcxx: a finding is reported once across --precompile and -c of the interface",
              one.returncode == 0 and two.returncode == 0 and found == 1, f"{found} reports; {one.stderr[:200]} {two.stderr[:200]}")
        clean = subprocess.run([mcxx, "check", "--target=x86_64-unknown-linux-gnu", "clean.cpp"], cwd=w, capture_output=True, text=True)
        check("mcxx check of a clean file prints nothing", clean.returncode == 0 and clean.stdout == "" and clean.stderr == "", clean.stderr[:200])
else:
    print("note: --mcxx not given; what a built mcxx prints is not checked")

# 6. links, requirement identifiers, traceability
ids = {}
keyword = re.compile(r"\b(MUST|MUST NOT|REQUIRED|SHALL|SHALL NOT|SHOULD|SHOULD NOT|RECOMMENDED)\b")
for md in sorted(specs.glob("mc*.md")):
    text = md.read_text(encoding="utf-8")
    for target in re.findall(r"\]\(([^)#:]+)(?:#[^)]*)?\)", text):
        check(f"link resolves: {md.name} -> {target}", (md.parent / target).exists())
    body = text[text.index("## 2."):]
    for n, line in enumerate(body.splitlines()):
        if keyword.search(line) and "<sup>MC" not in line:
            check(f"{md.name}: a requirement without an identifier", False, line.strip()[:120])
    for rid in re.findall(r'<a id="(MC\d-[0-9.]+-\d+)"></a>', text):
        check(f"{md.name}: identifier {rid} is unique", rid not in ids)
        ids[rid] = md.name

trace = load(repository / "conformance" / "traceability.json")
pending = trace.get("$pending", {})
for rid in sorted(ids):
    evidence = trace.get(rid)
    if rid in pending:
        print(f"PENDING  {rid}: {pending[rid]}")
        continue
    if not evidence:
        check(f"traceability: {rid} has evidence", False)
        continue
    for e in evidence:
        if "validate" in e:
            ok = any(p.startswith(e["validate"]) for p in passed)
            live = e["validate"].startswith(("mcxx ", "mcxx:"))   # a check of a built mcxx: only with --mcxx
            check(f"traceability: {rid} validate `{e['validate']}`", ok or (not mcxx and live),
                  "no passing check has that label")
        elif "test" in e:
            file, _, name = e["test"].partition(": ")
            p = repository / file
            check(f"traceability: {rid} test {e['test'][:80]}", p.exists() and f'"{name}"_test' in p.read_text(encoding="utf-8"))
        elif "fixture" in e:
            check(f"traceability: {rid} fixture {e['fixture']}", (repository / e["fixture"]).exists())
        elif "script" in e:
            p = repository / e["script"]
            check(f"traceability: {rid} script {e['script']}", p.exists() and e["contains"] in p.read_text(encoding="utf-8"))
        elif "manual" in e:
            check(f"traceability: {rid} manual", bool(e["manual"]))
        else:
            check(f"traceability: {rid} evidence kind", False, str(e))
for rid in trace:
    if not rid.startswith("$") and rid not in ids:
        check(f"traceability: {rid} is an identifier of a specification", False)

print(f"\n{len(passed)} passed, {failures} failed, {len([r for r in ids if r in pending])} pending of {len(ids)} requirements")
sys.exit(1 if failures else 0)
