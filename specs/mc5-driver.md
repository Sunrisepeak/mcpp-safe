# MC5 — The `mcxx` Driver and the Toolchain Contract

| | |
|---|---|
| Specification | MC5 |
| Version | 0.4.0 |
| Status | Draft |
| Schema | [`schema/mc5-version.schema.json`](schema/mc5-version.schema.json) |
| Examples | [`examples/mc5-version.json`](examples/mc5-version.json) |
| Implementation | `src/main.cpp` (the driver), `modules/backend/compiler` and `modules/backend/clang-compiler` (the compiler in process) |
| License | Apache-2.0 |

## Abstract

`mcxx` is MC++'s compiler driver. To a build system it is a Clang of a stated version -- it takes Clang's command line and produces what Clang produces -- with MC++'s feature gates (MC1) and plugins (MC4) in every compilation. This specification defines how `mcxx` is invoked, the command line it accepts, the environment it reads, what it outputs, how it describes itself, how its semantic services derive a unit's arguments, and what a build system's toolchain sees of it.

## 1. Conventions

- The key words **MUST**, **MUST NOT**, **REQUIRED**, **SHALL**, **SHALL NOT**, **SHOULD**, **SHOULD NOT**, **RECOMMENDED**, **MAY** and **OPTIONAL** are to be interpreted as described in BCP 14 (RFC 2119, RFC 8174) when, and only when, they appear in all capitals.
- "The compiler" is the Clang that `mcxx` runs in process; its version is `compiler.version` of §5.

## 2. Invocation

| Invocation | Meaning |
|---|---|
| `mcxx c++ ARGS` | A C++ compilation, exactly as `clang++ ARGS`. <a id="MC5-2-1"></a><sup>MC5-2-1</sup> |
| `mcxx cc ARGS` | A C compilation, exactly as `clang ARGS`. |
| `mcxx` named `clang++`, `c++` or `g++` (by its file name) | As `mcxx c++`. <a id="MC5-2-2"></a><sup>MC5-2-2</sup> |
| `mcxx` named `clang`, `cc` or `gcc` | As `mcxx cc`. |
| `mcxx -cc1 ...`, `mcxx -cc1as ...` | The compiler's own internal invocations, which its driver makes of itself; they MUST work, since the driver re-invokes its own executable. <a id="MC5-2-3"></a><sup>MC5-2-3</sup> |
| `mcxx check ARGS` | `ARGS` checked only (`-fsyntax-only` added): the compiler's diagnostics and MC++'s gates, no output files. <a id="MC5-2-4"></a><sup>MC5-2-4</sup> |
| `mcxx features [--json]` | The catalog (MC1 §10). |
| `mcxx version [--json]` | What this `mcxx` is (§5). |

Any other first argument is a usage error: `mcxx` prints its usage to standard error and exits with 2. <a id="MC5-2-5"></a><sup>MC5-2-5</sup>

## 3. Command line and behavior

- `mcxx` MUST accept every command line the compiler of its version accepts, with the same meaning, the same outputs and the same exit status, except for MC++'s diagnostics. <a id="MC5-3-1"></a><sup>MC5-3-1</sup> MC5 0.1.0 adds no option of its own to that command line.
- In every compilation of a translation unit, MC++'s source filters run before the main file is parsed and its gates after it is parsed (MC1, MC4); a gate at `deny` makes the compilation fail as any error does. <a id="MC5-3-2"></a><sup>MC5-3-2</sup>
- A finding MUST be reported once per build of a unit, also when the build precompiles a module interface (`--precompile`) and then compiles the interface to an object in a second step: the gates run where the source is parsed. <a id="MC5-3-3"></a><sup>MC5-3-3</sup>

The command lines a build system is expected to use, and that the conformance of MC5 is checked with: `-std=c++23` (and later), `-c`, `-o`, `--precompile`, `-fmodule-output=`, `-fmodule-file=NAME=PATH`, `-x c++-module`, `-fsyntax-only`, `-I`, `-isystem`, `-D`, `-U`, `--target=`, `--sysroot=`, `-O0` to `-O3`, `-g`, `-MD`, `-MF`, `-MT`, `-print-resource-dir`, `-print-file-name=`, `--version`, linking through `-fuse-ld=lld`.

## 4. Environment

| Variable | Meaning |
|---|---|
| `MCXX_LOG` | Log levels: a default, then per category: `info`, `debug,gates=info`. Categories include `gates` (`gates.facts`, `gates.rules`), `modules`, `parse`. Logs go to standard error. <a id="MC5-4-1"></a><sup>MC5-4-1</sup> |
| `MCXX_TRACE` | A file to write a Chrome trace of the run to (one event per timed span). |
| `MCXX_AUDIT` | A file to append waiver records to (MC1 §8). |

`mcxx` MUST NOT write anything to standard output or standard error that the compiler would not, unless one of these variables asks for it or a gate reports. <a id="MC5-4-2"></a><sup>MC5-4-2</sup>

## 5. `mcxx version`

`mcxx version` prints one line, `mcxx <version> (clang <compiler version>)`. `mcxx version --json` prints an object that MUST validate against [`schema/mc5-version.schema.json`](schema/mc5-version.schema.json): `mcxx` (its version), `compiler` (`name`, `version`), `specifications` (the versions of MC1, MC2, MC3, MC4 and MC5 it implements, and the MC4 protocol versions it speaks) and `providers` (the names of the active providers, MC4 §4). <a id="MC5-5-1"></a><sup>MC5-5-1</sup>

## 6. Arguments of a unit in the semantic services

libmc++'s semantic services (the backend behind mcppls) derive, from a unit's build command, the arguments that describe the program: the command without what the backend decides itself -- outputs (`-o`, `-MF`, `-MT`, `-MQ`, `-fmodule-output`), the interfaces read (`-fmodule-file`, `-fprebuilt-module-path`), the input language (`-x`), the resource directory, the phase (`-c`, `--precompile`, `-fsyntax-only`), dependency and color flags, the source file itself, and -- in a GCC command -- GCC's C++20 modules switches (`-fmodules`, `-fmodule-only`, `-fmodule-header`, `-fdeps-*`, ...), which mean something else to Clang (0.1.1). <a id="MC5-6-1"></a><sup>MC5-6-1</sup> A unit MUST get byte-for-byte the same derived arguments whichever way it reaches the services: a build database, a cache, or a command line. <a id="MC5-6-2"></a><sup>MC5-6-2</sup> The derived arguments key the cache of built module interfaces: two units with the same derived arguments share an interface. <a id="MC5-6-3"></a><sup>MC5-6-3</sup>

## 7. The toolchain contract

A build system uses `mcxx` as it uses an LLVM toolchain (V0.6, E-XIM-1): an `llvm`-shaped payload whose `bin/clang++` and `bin/clang` are `mcxx` (§2 selects the mode by name), next to `bin/ld.lld`, the compiler's resource directory (`lib/clang/23`) and the C++ standard library with its module sources. A build system MUST NOT need to know that the compiler is `mcxx` to build with it; it MAY ask `mcxx version --json` to find out. <a id="MC5-7-1"></a><sup>MC5-7-1</sup> With no `--target` and no configuration file, `mcxx` compiles for x86_64-unknown-linux-gnu on x86-64 Linux, whatever C library it itself runs on, as xim's LLVM does; a build tool that passes `--no-default-config` for a native build relies on it. <a id="MC5-7-2"></a><sup>MC5-7-2</sup>

For mcpp, the payload is the xpkg `mcxx:mcxx` of this repository (`xpkgs/pkgs/m/mcxx.lua`), used as `mcpp build --toolchain llvm@23.1.0-mcxx`; mcpp is not changed.

## 8. Module layout (recommended, 0.3.0)

This section is advice to the projects MC++ builds, and the layout this repository keeps.

- A module SHOULD be divided by what its parts are about: a primary interface unit that re-exports interface partitions, one per concern, and the definitions in implementation units (`.cpp`: `module m;`), not in the interface units (`.cppm`), which declare. A source file SHOULD stay under 2000 lines. <a id="MC5-8-1"></a><sup>MC5-8-1</sup>
- Why: with today's compilers an interface unit's BMI changes when any function body in it changes, and every unit that imports it is compiled again; a body in an implementation unit recompiles that unit alone. A one-file module is the slow case.
- MC++ plans to make the one-file form as cheap as the divided one: an interface unit's BMI carrying only what an importer needs (a reduced BMI: no non-inline function bodies), and a BMI not written again when those bytes would not change, as MC2 already does for the `.ifc` (MC2-2-4) -- so a build tool that looks at what changed rebuilds importers only when the interface did.

## 9. Diagnostic views (0.4.0)

A compile's diagnostics -- the compiler's own and MC++'s feature gates' (MC1 §9) -- are read by people and by agents, which want different things of them.

- `--mcxx-diagnostics=human|agent|clang` chooses a compile's view (`mcxx c++`, `mcxx cc`, `mcxx check`, and `mcxx` started as `clang++` or `clang`); the driver takes it out of the compiler's arguments and passes it on as the environment variable `MCXX_DIAGNOSTICS`, which a build tool that cannot add an option MAY set itself. With neither, the view is `human` when standard error is a terminal and `clang` otherwise, so a build log and an editor's problem matcher see the compiler's own format. A value that names no view is an error. <a id="MC5-9-1"></a><sup>MC5-9-1</sup>
- `human` lays each diagnostic out as Rust does: `error[code]: headline` (or `warning[...]`), ` --> file:line:column`, the source lines of its range with the range underlined, and then `= help:` lines -- what to write instead, how to allow it here, the compiler's fix-its -- and `= note:` lines, among them for a gate's finding its level and where that level is set (MC1 §6: a profile, the package, a module, a file pattern, a namespace, with the manifest). A compiler's notes follow their diagnostic, each with its place. Color on a terminal, none with `NO_COLOR`. <a id="MC5-9-2"></a><sup>MC5-9-2</sup>
- `agent` writes each diagnostic as one JSON object on one line, and nothing else, on standard error, valid against [`schema/mc5-diagnostic.schema.json`](schema/mc5-diagnostic.schema.json): `mcxx-diagnostic` (this form's version, `0.1.0`), `severity`, `code` (a gate's feature id, the compiler's own diagnostic name), `message` (the headline, without what to do about it), `file`, `range` (MC3's positions, from 0) and `location` (`file:line:column`, from 1); for a gate's finding `level`, `level-from`, `fix` and `waiver` (the attribute to write, as code); and `fixits` and `notes` when there are any. <a id="MC5-9-3"></a><sup>MC5-9-3</sup> An agent acts on a finding from these fields alone: which rule, where exactly, what to write instead, and which setting to change if the rule should not apply there.
- `mcxx check -p` prints its diagnostics in the same views.

