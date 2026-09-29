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

config, catalog_v, audit, facts, protocol, version, serve, interface, diagnostic = (schemas[n] for n in
    ("mc1-config", "mc1-catalog", "mc1-audit", "mc3-facts", "mc4-protocol", "mc5-version", "mc6-requests", "mc2-interface",
     "mc5-diagnostic"))
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
for i, d in enumerate(lines(ex / "mc5-diagnostic.jsonl")):
    validate(f"MC5 example validates: mc5-diagnostic.jsonl line {i + 1}", diagnostic, d)
validate("MC2 example validates: mc2-interface.json", interface, load(ex / "mc2-interface.json"))
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
i2 = load(ex / "mc2-interface.json")
validate("MC2 schema rejects a level that is not one", interface, dict(i2, dialect=dict(i2["dialect"], features={"goto": "loud"})), expect_valid=False)
validate("MC2 schema rejects a local declaration", interface, dict(i2, declarations=[dict(i2["declarations"][0], local=True)]), expect_valid=False)
validate("MC2 schema rejects an interface without its dialect", interface, {k: v for k, v in i2.items() if k != "dialect"}, expect_valid=False)
mc2_decl = load(specs / "schema/mc2-interface.schema.json")["$defs"]["declaration"]
mc3_decl = load(specs / "schema/mc3-facts.schema.json")["$defs"]["declaration"]
check("MC2's declaration is MC3's, `local` false",
      {**mc2_decl, "properties": {k: v for k, v in mc2_decl["properties"].items() if k != "local"}}
      == {**mc3_decl, "properties": {k: v for k, v in mc3_decl["properties"].items() if k != "local"}})
session = lines(ex / "mc4-session.jsonl")
validate("MC4 schema rejects a welcome without providers", protocol, dict(session[1], providers=[]), expect_valid=False)
validate("MC4 schema rejects a message without an id", protocol, {"type": "shutdown"}, expect_valid=False)
validate("MC4 schema rejects an unknown message type", protocol, {"type": "reload", "id": 3}, expect_valid=False)
d5 = lines(ex / "mc5-diagnostic.jsonl")[0]
validate("MC5 diagnostic schema rejects another form's version", diagnostic, dict(d5, **{"mcxx-diagnostic": "9.0.0"}), expect_valid=False)
validate("MC5 diagnostic schema rejects a level that is not one", diagnostic, dict(d5, level="loud"), expect_valid=False)
validate("MC5 diagnostic schema rejects a location without its column", diagnostic, dict(d5, location="src/a.cpp:3"), expect_valid=False)

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
        # MC2: the interface beside the BMI, from --precompile and from -fmodule-output; IFC 0.43, its hash
        # the SHA-256 of what follows it; written again only when it would change.
        import hashlib, time
        (w / "b.cppm").write_text("export module b;\nexport namespace b { int* data(); struct S { int x; }; }\n")
        (w / "c.cppm").write_text("export module c:p;\nexport int c_value(int v);\n")
        pre = subprocess.run([mcxx, *args, "--precompile", "b.cppm", "-o", "b.pcm"], cwd=w, capture_output=True, text=True)
        out = subprocess.run([mcxx, *args, "-fmodule-output=c-p.pcm", "-c", "c.cppm", "-o", "c.o"], cwd=w, capture_output=True, text=True)
        b_ifc, c_ifc = w / "b.ifc", w / "c-p.ifc"
        check("mcxx: an interface's .ifc beside its BMI, from --precompile and -fmodule-output",
              pre.returncode == 0 and out.returncode == 0 and b_ifc.exists() and c_ifc.exists(), pre.stderr[:200] + out.stderr[:200])
        if b_ifc.exists():
            data = b_ifc.read_bytes()
            check("mcxx: the .ifc is IFC 0.43 (signature, format version)", data[:4] == bytes([0x54, 0x51, 0x45, 0x1A]) and data[36:38] == bytes([0, 43]))
            check("mcxx: the .ifc's content hash is the SHA-256 of every byte after it", data[4:36] == hashlib.sha256(data[36:]).digest())
            before = b_ifc.stat().st_mtime_ns
            time.sleep(0.05)
            again = subprocess.run([mcxx, *args, "--precompile", "b.cppm", "-o", "b.pcm"], cwd=w, capture_output=True, text=True)
            check("mcxx: the same interface is the same bytes, and an unchanged .ifc is not written again",
                  again.returncode == 0 and b_ifc.read_bytes() == data and b_ifc.stat().st_mtime_ns == before)
        (w / "bad.cppm").write_text("export module bad;\nexport int f() { return undeclared; }\n")
        bad = subprocess.run([mcxx, *args, "--precompile", "bad.cppm", "-o", "bad.pcm"], cwd=w, capture_output=True, text=True)
        check("mcxx: a compile with an error writes no .ifc", bad.returncode != 0 and not (w / "bad.ifc").exists())
        # MC5 §9: the views of a compile's diagnostics.
        (w / "g.cpp").write_text("int f(int n) {\n    if (n) goto out;\n    return 1;\nout:\n    return 0;\n}\nint h() { return undeclared; }\n")
        view = lambda v: subprocess.run([mcxx, *args, f"--mcxx-diagnostics={v}", "-fsyntax-only", "g.cpp"], cwd=w, capture_output=True, text=True)
        agent = view("agent")
        records = [json.loads(l) for l in agent.stderr.splitlines() if l.strip()] if agent.stderr.lstrip().startswith("{") else []
        check("mcxx: the agent view is one JSON object per line and nothing else",
              agent.returncode != 0 and records and all(l.startswith("{") for l in agent.stderr.splitlines() if l.strip()), agent.stderr[:300])
        for r in records:
            validate(f"mcxx: an agent-view diagnostic validates ({r.get('code')})", diagnostic, r)
        g = next((r for r in records if r.get("code") == "goto"), {})
        check("mcxx: the agent view of a gate's finding: its exact range, level and where it is set, fix, waiver",
              g.get("range") == {"begin": {"line": 1, "column": 11}, "end": {"line": 1, "column": 19}} and g.get("level") == "warn"
              and "mcpp.toml" in g.get("level-from", "") and g.get("fix") and 'mcpp::allow("goto"' in g.get("waiver", ""), str(g))
        check("mcxx: the agent view carries the compiler's own errors", any(r.get("severity") == "error" and "undeclared" in r["message"] for r in records))
        human = view("human")
        check("mcxx: the human view: headline with its code, place, excerpt underlined, help and note",
              all(s in human.stderr for s in ("warning[goto]:", " --> g.cpp:2:12", "2 |     if (n) goto out;", "^^^^^^^^", "= help:", "= note: warn,"))
              and "\x1b[" not in human.stderr, human.stderr[:400])
        own = view("clang")
        check("mcxx: the clang view is the compiler's own format", "g.cpp:2:12: warning:" in own.stderr and "g.cpp:7:" in own.stderr, own.stderr[:300])
        env = subprocess.run([mcxx, *args, "-fsyntax-only", "g.cpp"], cwd=w, capture_output=True, text=True, env=dict(os.environ, MCXX_DIAGNOSTICS="agent"))
        check("mcxx: MCXX_DIAGNOSTICS chooses the view; off a terminal the default is the compiler's",
              env.stderr.lstrip().startswith("{") and "g.cpp:2:12: warning:" in subprocess.run([mcxx, *args, "-fsyntax-only", "g.cpp"], cwd=w,
              capture_output=True, text=True, env={k: v for k, v in os.environ.items() if k != "MCXX_DIAGNOSTICS"}).stderr)
        wrong = view("loud")
        check("mcxx: a view that is not one is an error", wrong.returncode != 0 and "--mcxx-diagnostics" in wrong.stderr, wrong.stderr[:200])
        (w / "d.cppm").write_text("export module d;\nexport int d_value();\n")
        checked = subprocess.run([mcxx, "check", "--target=x86_64-unknown-linux-gnu", "-std=c++23", "d.cppm"], cwd=w, capture_output=True, text=True)
        check("mcxx: `mcxx check` of an interface writes no .ifc", checked.returncode == 0 and not (w / "d.ifc").exists(), checked.stderr[:200])
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
