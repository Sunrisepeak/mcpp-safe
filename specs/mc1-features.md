# MC1 — Feature Registry, Profiles and Gates

| | |
|---|---|
| Specification | MC1 |
| Version | 0.3.0 |
| Status | Draft |
| Schemas | [`schema/mc1-config.schema.json`](schema/mc1-config.schema.json), [`schema/mc1-catalog.schema.json`](schema/mc1-catalog.schema.json), [`schema/mc1-audit.schema.json`](schema/mc1-audit.schema.json) |
| Examples | [`examples/mc1-config.toml`](examples/mc1-config.toml) with [`examples/mc1-config.json`](examples/mc1-config.json), [`examples/mc1-catalog.json`](examples/mc1-catalog.json), [`examples/mc1-audit.jsonl`](examples/mc1-audit.jsonl) |
| Implementation | `modules/features` (gates, the built-in provider `mc++.iso`), `modules/plugin/sdk` (the catalog) |
| License | Apache-2.0 |

## Abstract

MC++ gates C++ features: a package says which features its code may use, and a compiler that implements this specification reports every use of a feature that is not allowed where it occurs. This specification defines what a feature is, the features MC++ itself provides (ISO C++ language features, named by the standard's stable names), profiles, where and how a package configures the gates, which setting applies to a piece of code, how a declaration waives a gate, the audit record of a waiver, the diagnostics, and the machine-readable listing of what a compiler can gate. How features are provided, extended and replaced by plugins is MC4; the facts features are decided from are MC3.

## 1. Conventions

- The key words **MUST**, **MUST NOT**, **REQUIRED**, **SHALL**, **SHALL NOT**, **SHOULD**, **SHOULD NOT**, **RECOMMENDED**, **MAY** and **OPTIONAL** are to be interpreted as described in BCP 14 (RFC 2119, RFC 8174) when, and only when, they appear in all capitals.
- JSON is as defined by RFC 8259, encoded in UTF-8. TOML is TOML 1.0.
- A **gate** is the level a feature has for a piece of code. A **finding** is one use of a feature in a file, reported by the feature's provider. A **provider** is a named set of features and profiles (MC4 §2); MC++'s own is `mc++.iso`.
- Positions follow MC3 §2: 0-based lines, columns in UTF-8 bytes.

## 2. Features

A feature is described by these fields; the catalog (§10) lists them.

| Field | Type | Requirement | Description |
|---|---|---|---|
| `id` | string | MUST | Unique among the features a compiler has. It matches `^(lib:\|ext:)?[a-z0-9]+([-.][a-z0-9]+)*$`. <a id="MC1-2-1"></a><sup>MC1-2-1</sup> |
| `category` | string | MUST | One of `iso`, `policy`, `library`, `pitfall`, `extension` (§2.1). <a id="MC1-2-2"></a><sup>MC1-2-2</sup> |
| `standard` | string | MUST for `iso` | The ISO C++ stable names of what it controls, space-separated, each in brackets: `[stmt.goto]`. Empty for the other categories. <a id="MC1-2-3"></a><sup>MC1-2-3</sup> |
| `layer` | string | SHOULD | Where it is decided: `syntax`, `decl`, `expr`, `text`, `flow`. <a id="MC1-2-4"></a><sup>MC1-2-4</sup> |
| `summary` | string | MUST | One line: what the feature is, and for an undefined-behavior source, why. <a id="MC1-2-5"></a><sup>MC1-2-5</sup> |
| `fix` | string | SHOULD | What to write instead. <a id="MC1-2-6"></a><sup>MC1-2-6</sup> |
| `default` | level | MUST | Its level where nothing else says (§5). <a id="MC1-2-7"></a><sup>MC1-2-7</sup> |
| `waivable` | boolean | MUST | Whether `[[mcpp::allow]]` may waive it (§7). <a id="MC1-2-8"></a><sup>MC1-2-8</sup> |
| `profiles` | object[] | MAY | The profiles it joins, each with a level: `{ "profile": "safe", "level": "deny" }`. |
| `needs` | string[] | MAY | The kinds of MC3 facts it is decided from. |
| `requires-declaration` | string | MAY | A name at global scope without which it cannot occur (`nlohmann`). |

### 2.1 Categories

| Category | What it is | Subtraction |
|---|---|---|
| `iso` | An ISO C++ language feature. Only MC++'s built-in provider defines one, or a plugin that replaces one of its features (MC4 §4). <a id="MC1-2.1-1"></a><sup>MC1-2.1-1</sup> | yes |
| `policy` | A rule on how the language is used that is not one ISO feature: `raw-pointers`. | yes |
| `library` | Which library facilities a program may use. Its id MUST start with `lib:`: `lib:std.vector`. <a id="MC1-2.1-2"></a><sup>MC1-2.1-2</sup> | yes |
| `pitfall` | A trap in a library or the language: `json-brace-init`. | yes |
| `extension` | What MC++ adds to C++. Its id MUST start with `ext:`: `ext:cfg`. Code that uses an extension needs MC++. <a id="MC1-2.1-3"></a><sup>MC1-2.1-3</sup> | no |

Denying a feature of any category but `extension` MUST NOT change what a program that compiles means: a program that uses none of them is ISO C++, compiled by any conforming compiler. <a id="MC1-2.1-4"></a><sup>MC1-2.1-4</sup>

### 2.2 Levels

A level is `allow` (nothing is reported), `warn` (a warning) or `deny` (an error: the compilation fails). A configuration MAY also write `off` for `allow`, `warning` for `warn` and `error` for `deny`. <a id="MC1-2.2-1"></a><sup>MC1-2.2-1</sup> Levels are ordered `allow` < `warn` < `deny`; "the strictest" is the greatest.

## 3. The built-in features (`mc++.iso`)

A compiler that implements this specification MUST provide these features, with these ids, categories and stable names, unless a plugin replaces them (MC4 §4). <a id="MC1-3-1"></a><sup>MC1-3-1</sup> All of them default to `allow`: with no configuration, MC++ compiles ISO C++. <a id="MC1-3-2"></a><sup>MC1-3-2</sup> What each one reports, in the file's own code (never in what it includes or imports):

| Id | Stable names | A finding for | Not a finding |
|---|---|---|---|
| `raw-pointer-arithmetic` | [expr.add] [expr.sub] [expr.pre.incr] | `+`, `-`, `+=`, `-=`, `++`, `--` with an operand of pointer type; `[]` on a pointer | `[]` on a C array (that is `c-array`'s) |
| `new-delete` | [expr.new] [expr.delete] | every new-expression and delete-expression | a placement form inside a library the file includes |
| `reinterpret-cast` | [expr.reinterpret.cast] | `reinterpret_cast`; a C-style or functional cast that performs a reinterpret_cast | a cast to or from `void*` |
| `c-style-cast` | [expr.cast] [expr.type.conv] | `(T)e`; `T(e)` where T is a scalar type | `T(e)` that constructs a class |
| `const-cast` | [expr.const.cast] | `const_cast` | |
| `union` | [class.union] | the definition of a union | |
| `c-array` | [dcl.array] | a variable, member or parameter of array type | `std::array` |
| `c-varargs` | [dcl.fct] [cstdarg.syn] | a function with a C `...` parameter; a use of `va_arg` | a function parameter pack; `catch (...)` |
| `uninitialized` | [dcl.init.general] [basic.indet] | a variable with automatic storage and no initializer whose default-initialization leaves its value indeterminate: a scalar, an array of scalars, a non-empty class with a trivial default constructor | a static or thread-local variable; `T x {}`; a class with a default member initializer |
| `asm` | [dcl.asm] | an asm-declaration, in a function or at namespace scope | a compiler intrinsic |
| `include` | [cpp.include] [module.global.frag] | an `#include` that is not in a global module fragment | an `#include` in the global module fragment; an `#include` a conditional excludes |
| `goto` | [stmt.goto] | `goto` and computed `goto` | |
| `macros` | [cpp.replace] | a macro definition | a definition a conditional excludes, or in a comment or string |
| `exceptions` | [except.throw] [except.pre] | a throw-expression; a try-block (a function-try-block too) | `noexcept` |
| `rtti` | [expr.typeid] [expr.dynamic.cast] | `typeid`; `dynamic_cast` | |

Each of these rows is REQUIRED as stated; the conformance fixtures under `conformance/gates` check every row with at least five cases that must be found and five that must not. <a id="MC1-3-3"></a><sup>MC1-3-3</sup>

## 4. Profiles

A profile is a named set of levels. A feature joins a profile by naming it (`profiles` in §2); a profile MAY also include other profiles, set a level for every feature of a category, and set levels for features by id, whichever provider provides them. <a id="MC1-4-1"></a><sup>MC1-4-1</sup>

The level a feature has under a profile is the strictest of: the levels the feature gives itself for that profile, the level the profile gives the feature's category, the level the profile gives the feature by id, and the level the feature has under each profile it includes (recursively, each profile once). <a id="MC1-4-2"></a><sup>MC1-4-2</sup> Under several profiles, a feature has the strictest level any of them gives it; a profile that gives it none does not lower it. <a id="MC1-4-3"></a><sup>MC1-4-3</sup>

MC++ defines these profiles. A compiler MUST provide them with these contents, unless a plugin replaces them (MC4 §4): <a id="MC1-4-4"></a><sup>MC1-4-4</sup>

| Profile | Meaning | Contents (all `deny`) |
|---|---|---|
| `safe` | The sources of undefined behavior a compiler does not check for you | `raw-pointer-arithmetic`, `new-delete`, `reinterpret-cast`, `c-style-cast`, `const-cast`, `union`, `c-array`, `c-varargs`, `uninitialized`, `asm` |
| `modules` | Every dependency by import | `include` |
| `strict` | `safe` and `modules`, and control flow and the preprocessor | includes `safe` and `modules`; `goto`, `macros` |
| `portable` | ISO C++ only | every feature of category `extension` |

## 5. Configuration

A source file's configuration is the table `[package.metadata.mcxx]` of the nearest `mcpp.toml` that has a `[package]` table, looking from the file's directory upward. <a id="MC1-5-1"></a><sup>MC1-5-1</sup> A file with no such manifest has the empty configuration. Its shape (a JSON Schema over the table read as JSON: [`schema/mc1-config.schema.json`](schema/mc1-config.schema.json)):

```toml
[package.metadata.mcxx]
profile = "safe"                                  # or several: ["safe", "modules"]

[package.metadata.mcxx.features]                  # the package's levels, by feature id
"json-brace-init" = "deny"
exceptions = "deny"

[package.metadata.mcxx.modules."app.legacy"]      # a named module's levels
goto = "allow"

[package.metadata.mcxx.namespaces."app::detail"]  # a namespace's levels (and those inside it)
reinterpret-cast = "allow"

[package.metadata.mcxx.imports."legacy"]          # what an import of a module may bring in (§7)
allow = ["c-array", "raw-pointers"]
reason = "the C library, wrapped"
```

| Key | Type | Description |
|---|---|---|
| `profile` | string or string[] | The profiles that apply to the package. |
| `features` | table: id → level | The package's levels. |
| `modules` | table: module name → (id → level) | A named module's levels. A partition `m:p` takes `m`'s, and then its own if `m:p` is listed. <a id="MC1-5-2"></a><sup>MC1-5-2</sup> |
| `namespaces` | table: qualified name → (id → level) | A namespace's levels; they apply to the namespaces nested in it. <a id="MC1-5-3"></a><sup>MC1-5-3</sup> |
| `imports` | table: module name → { `allow`: id[], `reason`: string } | A waiver over every import of the module in the package's files (§7, 0.3.0). |

A compiler MUST report, as a warning and never silently ignore: a value that is not a level; a feature id that no provider it has declares (in `imports` too); an `imports` entry without `allow`; a profile that no provider defines and no feature joins; a `profile` that is neither a string nor a list of strings. <a id="MC1-5-4"></a><sup>MC1-5-4</sup> Other keys of `[package.metadata.mcxx]` are reserved for later versions and SHOULD be ignored. <a id="MC1-5-5"></a><sup>MC1-5-5</sup>

## 6. Which level applies

The level of a feature for a finding is, from the most specific: <a id="MC1-6-1"></a><sup>MC1-6-1</sup>

1. a waiver on a declaration that contains the finding (§7), if the feature is waivable;
2. within a region -- a declaration carrying an attribute a provider declares as one (MC4 §2) -- the level the region's profile gives the feature, where it is stricter than the level of the steps below; <a id="MC1-6-2"></a><sup>MC1-6-2</sup>
3. the level of the innermost configured namespace that contains the finding's enclosing namespace and sets the feature;
4. the level of the finding's module (a partition's own over its module's, §5);
5. the package's level (`features`);
6. the profiles' level (§4);
7. the feature's default.

## 7. Waivers

A declaration waives gates with the attribute `[[mcpp::allow("id")]]`, `[[mcpp::allow("id, id2")]]` (several ids, comma-separated) or `[[mcpp::allow("id", "reason")]]`. The arguments MUST be string literals; anything else is an error at the attribute. <a id="MC1-7-1"></a><sup>MC1-7-1</sup> A waiver covers the source range of the declaration it is attached to; the innermost waiver that names a feature is the one that applies. <a id="MC1-7-2"></a><sup>MC1-7-2</sup>

- A waiver of a feature that is not waivable MUST NOT change its level; the diagnostic says that it cannot be waived. <a id="MC1-7-3"></a><sup>MC1-7-3</sup>
- A waiver that names an id no provider declares MUST be reported as a warning at the attribute. <a id="MC1-7-4"></a><sup>MC1-7-4</sup>
- Every finding a waiver waives MUST be recorded as a waiver (§8); a waiver is never silent. <a id="MC1-7-5"></a><sup>MC1-7-5</sup>

An import brings in what another module's interface exposes (MC3 §4.13), and a feature's rule may find it at the import (MC4 `report_imports`): what crosses a dialect boundary. Such a finding is waived on the import: `import legacy [[mcpp::allow("c-array, raw-pointers", "reason")]];` -- the same arguments as on a declaration, its range the import declaration -- or, for a build tool whose own dependency scanner does not take an attribute on an import (mcpp's), by the package's `imports` table (§5), over every import of that module in the package. A compiler MUST accept both, and MUST NOT pass the attribute on an import to a compiler that refuses it (Clang does: it is blanked, every position kept). <a id="MC1-7-6"></a><sup>MC1-7-6</sup> The audit names the waiver `import legacy`, and `import legacy (mcpp.toml)` for the table's.

## 8. Audit

When the environment variable `MCXX_AUDIT` names a file, a compiler MUST append one JSON object per waived finding, one per line, to that file ([`schema/mc1-audit.schema.json`](schema/mc1-audit.schema.json)): `feature`, `path` (absolute), `line` and `column` (1-based), `declaration` (the qualified name of the declaration carrying the waiver) and `reason` (empty when none was given). <a id="MC1-8-1"></a><sup>MC1-8-1</sup>

## 9. Diagnostics

A finding at `warn` is a warning and at `deny` an error, at the finding's range. <a id="MC1-9-1"></a><sup>MC1-9-1</sup> Its code (the LSP `code`, the SARIF `ruleId`) is the feature id; its message contains the id in brackets, `[id]`, the feature's `fix`, and how to waive it. <a id="MC1-9-2"></a><sup>MC1-9-2</sup> Configuration problems (§5) have the code `mcxx-config`. <a id="MC1-9-3"></a><sup>MC1-9-3</sup>

```
error: `x` (`int`) is not initialized [uninitialized]; give it an initializer (`T x {};` for zero); to allow it here, [[mcpp::allow("uninitialized")]] on the declaration
```

## 10. The catalog

`mcxx features --json` prints what the compiler can gate: its active providers, every feature with the provider that provides it and the providers it replaces, every profile with the features it does not leave at `allow`, the attributes providers claim (MC4 §2), and the conflicts (MC4 §4). The output MUST validate against [`schema/mc1-catalog.schema.json`](schema/mc1-catalog.schema.json). <a id="MC1-10-1"></a><sup>MC1-10-1</sup> `mcxx features` without `--json` prints the same for people, and exits with 1 when there are conflicts. <a id="MC1-10-2"></a><sup>MC1-10-2</sup>
