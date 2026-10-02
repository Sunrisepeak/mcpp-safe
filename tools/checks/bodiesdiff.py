#!/usr/bin/env python3
"""MC++'s full parse against Clang's AST (A3.0.1, second half): for self-contained C++ files, the statements and
expressions mcxx.frontend:bodies reads -- each one's kind and the tokens it spans -- against what Clang makes of the
same text (`-Xclang -ast-dump=json`).

    python3 tools/checks/bodiesdiff.py --lexdump MCXX_LEXDUMP --clang CLANGXX [--std c++23] [--show N] [--min-agree 99.5] FILE...

    MCXX_LEXDUMP  `--bodies --nodes`: every statement and expression, with its kind, its operator and its range
    CLANGXX       any clang++ of the toolchain: it is the reference, and reads FILE as it is (no includes, no macros of
                  headers: the fixtures in modules/frontend/corpus/bodies are such files)

A node agrees when both sides have a node of the same syntax class over the same bytes. What is compared, and what
is not:
  - Clang's implicit nodes (casts the language adds, temporaries, the constructor calls of a declaration, default
    arguments, the range-based for's hidden variables) are not nodes of the syntax: skipped, with Clang's wrappers
    that cover the same bytes as what they wrap (ExprWithCleanups, ConstantExpr, ...).
  - A statement an expression makes (`f();`) is Clang's expression alone: our `expression` wrapper is not compared.
    Clang's declarations (VarDecl, ...) are the outline's, not this tree's; a DeclStmt is compared.
  - A template's instantiations repeat its nodes: the nodes are a set of (class, bytes).
The agreement rate is agreeing nodes over the nodes either side has. The per-class table says where they disagree.
Exits non-zero below --min-agree.

The same comparison over the corpora, which `import std;`, needs Clang's AST from the Clang backend: `mcxx-probe --syntax-nodes`
would print for each file `{"nodes": [{"class": <Stmt::getStmtClassName>, "op": <opcode spelling or "">, "range": [begin offset,
end offset)}]}` for the explicit statements and expressions of its function bodies; this script's `clang_nodes` is that, as
Clang's JSON gives it.
"""
import collections, json, pathlib, subprocess, sys


def args_of(name):
    out, i = [], 1
    while i < len(sys.argv):
        if sys.argv[i] == f"--{name}" and i + 1 < len(sys.argv):
            out.append(sys.argv[i + 1])
            i += 1
        i += 1
    return out


def arg(name, default=None):
    found = args_of(name)
    return found[-1] if found else default


lexdump = str(pathlib.Path(arg("lexdump")).resolve())
clang = arg("clang")
std = arg("std", "c++23")
show = int(arg("show", 20))
min_agree = float(arg("min-agree", 99.5))
files = [a for a in sys.argv[1:] if a.endswith((".cpp", ".cppm", ".cc")) and not sys.argv[sys.argv.index(a) - 1].startswith("--")]

# Our kinds, as the Clang classes that are them.
MINE = {
    "integer": {"IntegerLiteral"}, "floating": {"FloatingLiteral"}, "character": {"CharacterLiteral"}, "string": {"StringLiteral"},
    "boolean": {"CXXBoolLiteralExpr"}, "null-pointer": {"CXXNullPtrLiteralExpr"}, "this": {"CXXThisExpr"},
    "id": {"DeclRefExpr", "UnresolvedLookupExpr", "DependentScopeDeclRefExpr", "UnresolvedMemberExpr", "MemberExpr"}, "paren": {"ParenExpr"},
    "unary": {"UnaryOperator", "CXXOperatorCallExpr"}, "binary": {"BinaryOperator", "CXXOperatorCallExpr"}, "assign": {"BinaryOperator", "CompoundAssignOperator", "CXXOperatorCallExpr"},
    "conditional": {"ConditionalOperator", "BinaryConditionalOperator"},
    "call": {"CallExpr", "CXXMemberCallExpr", "CXXOperatorCallExpr", "UserDefinedLiteral"}, "subscript": {"ArraySubscriptExpr", "CXXOperatorCallExpr"},
    "member": {"MemberExpr", "CXXDependentScopeMemberExpr"},
    "cast-named": {"CXXStaticCastExpr", "CXXDynamicCastExpr", "CXXConstCastExpr", "CXXReinterpretCastExpr"}, "cast-c": {"CStyleCastExpr"},
    "cast-functional": {"CXXFunctionalCastExpr", "CXXTemporaryObjectExpr", "CXXScalarValueInitExpr", "CXXUnresolvedConstructExpr"},
    "sizeof": {"UnaryExprOrTypeTraitExpr"}, "sizeof-pack": {"SizeOfPackExpr"}, "noexcept": {"CXXNoexceptExpr"}, "typeid": {"CXXTypeidExpr"},
    "new": {"CXXNewExpr"}, "delete": {"CXXDeleteExpr"}, "throw": {"CXXThrowExpr"}, "co-await": {"CoawaitExpr"}, "co-yield": {"CoyieldExpr"},
    "lambda": {"LambdaExpr"}, "requires": {"RequiresExpr"}, "fold": {"CXXFoldExpr"}, "pack-expansion": {"PackExpansionExpr"},
    "init-list": {"InitListExpr", "CXXConstructExpr"},
    "builtin": {"CallExpr", "TypeTraitExpr", "VAArgExpr", "OffsetOfExpr", "BuiltinBitCastExpr", "ConvertVectorExpr", "ShuffleVectorExpr", "ChooseExpr",
                "ArrayTypeTraitExpr", "ExpressionTraitExpr"},
    "static-assert": {"DeclStmt"}, "statement-expression": {"StmtExpr"}, "label-address": {"AddrLabelExpr"},
    "compound": {"CompoundStmt"}, "declaration": {"DeclStmt"}, "if": {"IfStmt"}, "switch": {"SwitchStmt"}, "while": {"WhileStmt"},
    "do": {"DoStmt"}, "for": {"ForStmt"}, "range-for": {"CXXForRangeStmt"}, "return": {"ReturnStmt"}, "break": {"BreakStmt"},
    "continue": {"ContinueStmt"}, "goto": {"GotoStmt", "IndirectGotoStmt"}, "labeled": {"LabelStmt"}, "case": {"CaseStmt"}, "default": {"DefaultStmt"},
    "try": {"CXXTryStmt"}, "handler": {"CXXCatchStmt"}, "co-return": {"CoreturnStmt"}, "asm": {"GCCAsmStmt", "MSAsmStmt"}, "null": {"NullStmt"},
    "attributed": {"AttributedStmt"},
}
# Ours that have no Clang node of the same bytes: not compared.
NOT_COMPARED = {"expression", "function-body", "contract-assert", "consteval-block", "error", "designated", "member-init",
                "reflect", "splice", "pack-index", "paren-list"}
CLANG = collections.defaultdict(set)
for mine, classes in MINE.items():
    for c in classes:
        CLANG[c].add(mine)
# Clang's nodes that are syntax only where we have a node for them too: a constructor call a declaration makes is no expression written.
SOFT = {"CXXConstructExpr"}
# Clang's nodes that are not syntax: what the language adds, wrappers of the same bytes, and the hidden parts of a loop.
IMPLICIT = {"ImplicitCastExpr", "ExprWithCleanups", "MaterializeTemporaryExpr", "CXXBindTemporaryExpr", "CXXDefaultArgExpr",
            "ConstantExpr", "SubstNonTypeTemplateParmExpr", "CXXStdInitializerListExpr", "ImplicitValueInitExpr", "ArrayInitLoopExpr",
            "ArrayInitIndexExpr", "OpaqueValueExpr", "RecoveryExpr", "SourceLocExpr", "CXXInheritedCtorInitExpr", "FullExpr"}


def clang_nodes(path, text):
    """(class, begin, end) of the explicit statements and expressions in Clang's JSON AST of the file."""
    run = subprocess.run([clang, f"-std={std}", "-fsyntax-only", "-w", "-Xclang", "-ast-dump=json", path], capture_output=True, text=True)
    if not run.stdout.startswith("{"):
        raise SystemExit(f"{path}: clang gave no AST: {run.stderr[:300]}")
    tree = json.loads(run.stdout)
    out = set()
    names = set()

    def offsets(node):
        r = node.get("range", {})
        b, e = r.get("begin", {}), r.get("end", {})
        b = b.get("expansionLoc", b)
        e = e.get("expansionLoc", e)
        if "offset" not in b or "offset" not in e:
            return None
        return b["offset"], e["offset"] + e.get("tokLen", 0)

    def walk(node, in_function, parent=""):
        kind = node.get("kind", "")
        if node.get("implicit"):
            return
        implicit = bool(node.get("isImplicit"))
        if implicit:
            # the range-based for's hidden range variable holds the range expression the program wrote
            hidden_range = (kind == "DeclStmt" and parent == "CXXForRangeStmt") or \
                           (kind == "VarDecl" and parent == "DeclStmt" and str(node.get("name", "")).startswith("__range"))
            if not hidden_range:
                return
        # a call of a lambda is a call of its operator(): the DeclRefExpr to it has the call's bytes, not a name's
        if kind == "DeclRefExpr" and str(node.get("referencedDecl", {}).get("name", "")).startswith(("operator", "__builtin")):
            return
        # attribute arguments are the attribute's (not parsed yet); a structured binding's parts are made, not written
        if kind.endswith("Attr") or kind == "BindingDecl":
            return
        inner0 = (node.get("inner") or [{}])[0]
        # an operator-> is an overloaded call whose bytes are the object's; a conversion function is called where the type is converted
        skip_node = (kind == "CXXOperatorCallExpr" and str(inner0.get("inner", [{}])[0].get("referencedDecl", {}).get("name", "")) == "operator->") or \
                    (kind == "CXXMemberCallExpr" and str(inner0.get("name", "")).startswith("operator "))
        if not implicit and kind in ("VarDecl", "BindingDecl", "ParmVarDecl", "FieldDecl") and "offset" in node.get("loc", {}):
            names.add(node["loc"]["offset"])
        if not implicit and not skip_node and kind in CLANG and kind not in IMPLICIT:
            span = offsets(node)
            if span is not None and span[1] > span[0]:   # a node of no bytes is one Clang made
                # a DeclStmt without its `;` is a condition's or a range-for's variable, the outline's declaration
                if (kind != "DeclStmt" or text[span[1] - 1:span[1]] == b";") and text[span[0]:span[1]] != b":":   # `:` of a range-based for: made
                    out.add((kind, span[0], span[1]))
        # a template's instantiations repeat its pattern, with the nodes of what was substituted: only the pattern is written
        patterns = 0
        for child in node.get("inner", []):
            child_kind = child.get("kind")
            if kind == "FunctionTemplateDecl" and child_kind in ("FunctionDecl", "CXXMethodDecl", "CXXConstructorDecl", "CXXConversionDecl", "CXXDestructorDecl"):
                patterns += 1
                if patterns > 1:
                    continue
            if kind == "ClassTemplateDecl" and child_kind == "ClassTemplateSpecializationDecl":
                continue
            # a lambda's captures are initialized by expressions of their own, which no statement of the text is: its body is
            if kind == "LambdaExpr" and child_kind not in ("CompoundStmt", "CXXRecordDecl"):
                continue
            # the range-based for's condition and increment are made, not written
            if kind == "CXXForRangeStmt" and child_kind in ("BinaryOperator", "UnaryOperator"):
                continue
            walk(child, in_function, kind)

    walk(tree, False)
    # what Clang makes of a declared name's own bytes (the condition variable a condition names, a structured binding's parts) is no expression written
    return {n for n in out if not (n[0] in ("DeclRefExpr", "MemberExpr") and n[1] in names)}


def our_nodes(path, text):
    run = subprocess.run([lexdump, "--bodies", "--nodes", path], capture_output=True, text=True)
    if not run.stdout.startswith("{"):
        raise SystemExit(f"{path}: mcxx-lexdump gave no tree: {run.stderr[:300]}")
    result = json.loads(run.stdout)
    starts = [0]
    for i, c in enumerate(text):
        if c == 10:
            starts.append(i + 1)
    out = set()
    for n in result["nodes"]:
        if n["kind"] in NOT_COMPARED:
            continue
        if "in-type" in n["detail"]:
            continue   # an array bound, a template argument: Clang keeps them in types
        l1, c1, l2, c2 = n["range"]
        out.add((n["kind"], starts[l1] + c1, starts[l2] + c2, n["detail"]))
    return out, result


total = collections.Counter()
by_class = collections.defaultdict(collections.Counter)
shown = 0
for path in files:
    text = pathlib.Path(path).read_bytes()
    theirs = clang_nodes(path, text)
    ours, result = our_nodes(path, text)
    # Clang's JSON lists no arguments for what a new-expression constructs: nothing inside one is compared.
    new_spans = [(b, e) for kind, b, e, _ in ours if kind == "new"] + [(b, e) for c, b, e in theirs if c == "CXXNewExpr"]
    inside = lambda b, e: any(nb <= b and e <= ne and (nb, ne) != (b, e) for nb, ne in new_spans)
    ours = {n for n in ours if not inside(n[1], n[2])}
    theirs = {n for n in theirs if not inside(n[1], n[2])}
    # `P p{1, 2};` where P has a constructor is Clang's CXXConstructExpr, with no InitListExpr: our init-list ends where it does
    constructed = {e for c, _, e in theirs if c == "CXXConstructExpr"}
    ours = {n for n in ours if not (n[0] == "init-list" and n[2] in constructed)}
    ours_by_span = collections.defaultdict(set)
    for kind, b, e, _ in ours:
        ours_by_span[(b, e)].add(kind)
    theirs_by_span = collections.defaultdict(set)
    for c, b, e in theirs:
        theirs_by_span[(b, e)].add(c)
    for kind, b, e, _ in sorted(ours, key=lambda n: (n[1], n[2], n[0])):
        classes = theirs_by_span.get((b, e), set())
        if classes & MINE[kind]:
            total["agree"] += 1
            by_class[kind]["agree"] += 1
        else:
            total["ours only"] += 1
            by_class[kind]["ours only"] += 1
            if shown < show:
                shown += 1
                line = text[:b].count(b"\n") + 1
                print(f"{path}:{line}: ours {kind} `{text[b:e].decode(errors='replace')[:60]}`: clang has {sorted(classes) or 'nothing'} over these bytes")
    ours_spans = {(b, e): kinds for (b, e), kinds in ours_by_span.items()}
    for c, b, e in sorted(theirs, key=lambda n: (n[1], n[2], n[0])):
        mine = CLANG[c]
        if ours_spans.get((b, e), set()) & mine or c in SOFT:
            continue
        total["clang only"] += 1
        by_class[sorted(mine)[0]]["clang only"] += 1
        if shown < show:
            shown += 1
            line = text[:b].count(b"\n") + 1
            print(f"{path}:{line}: clang {c} `{text[b:e].decode(errors='replace')[:60]}`: ours has {sorted(ours_spans.get((b, e), set())) or 'nothing'} over these bytes")
union = sum(total.values())
rate = 100 * total["agree"] / union if union else 0.0
print(f"{'class':<20} {'agree':>6} {'ours only':>10} {'clang only':>11}")
for kind in sorted(by_class):
    c = by_class[kind]
    print(f"{kind:<20} {c['agree']:>6} {c['ours only']:>10} {c['clang only']:>11}")
print(f"bodiesdiff: {len(files)} files; {total['agree']} nodes agree, {total['ours only']} only ours, {total['clang only']} only Clang's: {rate:.3f}% of {union}")
sys.exit(0 if rate >= min_agree and union else 1)
