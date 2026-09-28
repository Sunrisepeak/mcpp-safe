# MC5 — The `mcxx` Driver and the Toolchain Contract

| | |
|---|---|
| Specification | MC5 |
| Version | 0.1.0 |
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

`mcxx version` prints one line, `mcxx <version> (clang <compiler version>)`. `mcxx version --json` prints an object that MUST validate against [`schema/mc5-version.schema.json`](schema/mc5-version.schema.json): `mcxx` (its version), `compiler` (`name`, `version`), `specifications` (the versions of MC1, MC3, MC4 and MC5 it implements, and the MC4 protocol versions it speaks) and `providers` (the names of the active providers, MC4 §4). <a id="MC5-5-1"></a><sup>MC5-5-1</sup>

## 6. Arguments of a unit in the semantic services

libmc++'s semantic services (the backend behind mcppls) derive, from a unit's build command, the arguments that describe the program: the command without what the backend decides itself -- outputs (`-o`, `-MF`, `-MT`, `-MQ`, `-fmodule-output`), the interfaces read (`-fmodule-file`, `-fprebuilt-module-path`), the input language (`-x`), the resource directory, the phase (`-c`, `--precompile`, `-fsyntax-only`), dependency and color flags, and the source file itself. <a id="MC5-6-1"></a><sup>MC5-6-1</sup> A unit MUST get byte-for-byte the same derived arguments whichever way it reaches the services: a build database, a cache, or a command line. <a id="MC5-6-2"></a><sup>MC5-6-2</sup> The derived arguments key the cache of built module interfaces: two units with the same derived arguments share an interface. <a id="MC5-6-3"></a><sup>MC5-6-3</sup>

## 7. The toolchain contract

A build system uses `mcxx` as it uses the LLVM toolchain of the compiler's version (V0.6, E-XIM-1): an `llvm`-shaped payload whose `bin/clang++` and `bin/clang` are `mcxx` (§2 selects the mode by name), next to `bin/ld.lld`, the compiler's resource directory and the C++ standard library with its module sources. A build system MUST NOT need to know that the compiler is `mcxx` to build with it; it MAY ask `mcxx version --json` to find out. <a id="MC5-7-1"></a><sup>MC5-7-1</sup>
