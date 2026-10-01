# Forks and derived repositories (early stage)

Under the early-stage rules (milestones doc §1.0), related repositories are forked under the
`speak-agent` account. Nothing is filed or pushed to the official repositories. This file lists
every fork, what it carries, and what it would take to merge back (a decision for after MS).

| Repository | Upstream | Branch | What it carries | Merge back |
|---|---|---|---|---|
| `speak-agent/llvm-clang-dev` | llvm/llvm-project 23.1.0 (a subset kept verbatim under `llvm/`, see `UPSTREAM-REV`) | `mcxx` (PR #1; packaging revisions are tags: 23.1.0 … 23.1.0.9; since 23.1.0.3 the default target is x86_64-unknown-linux-gnu over the openkal host, since 23.1.0.4 openkal's Windows and macOS targets build too, 23.1.0.5 the resource directory's generated intrinsics headers, 23.1.0.6 upstream's fix for `std::align_val_t` in MSVC's std module (#219151), 23.1.0.7 the code generator on macOS (`INT64_C`'s type), 23.1.0.8 a macOS program's own path (`_NSGetExecutablePath`), 23.1.0.9 MSVC's triple as Windows' default and on macOS the kernel's process identifier and Darwin's release) | Clang/LLVM frontend libraries built by mcpp on openkal (package `llvm.clang-dev` 23.1.0): generated config and TableGen output in `llvm-generated/`, generators in `tools/`, three link stubs in `port/`. Upstream code is unpatched. | Not an upstream fork in the usual sense: a build of it. It reaches mcpp's index as E-IDX-1. |
| `speak-agent/mcpp-language-server` | Sunrisepeak/mcpp-language-server (377d222) | `mcxx-engine` | The `mcxx` engine: libmc++ in process (`mcxx.backend` + `mcxx.lsp`) as the default core engine, with clangd kept as `--engine clangd`. Other changes: a payload without clangd (`mcxx/resource`, `engines.mcxx`); kit matching by the backend's libc++ release; the conformance runner's `--core-engine` and `only-engine`; clangd-only fixtures and checks marked. | E-LS-1…E-LS-5. After MS, upstream's decision (R5). |
| `speak-agent/openkal` | mcpplibs/openkal 0.14.0 (b0c3b5b) | `dlopen` (PR #1; tag 0.15.0) | `openkal.exec` 0.15: `kal_exec_publish_part` and `kal_exec_granularity`, what a loader above openkal needs (SPEC §11 entry 21); conformance observations. | Upstream's decision after MS: a spec revision (clause 8: two declarations added). |
| `speak-agent/openkal-linux` | mcpplibs/openkal-linux 0.15.0 (9ef9720) | `dlopen` (PR #1; tags 0.16.0 to 0.16.3) | the two operations; `_start` relocates a `-static-pie` program (relative relocations, `DT_RELR`) and biases the TLS image; 0.16.1: a program's arguments and environment are no longer capped at 512 entries (a link line of thousands of objects); 0.16.2: built optimised too -- `-O2` compiled the test of `&__ehdr_start` to a read of the unrelocated GOT word (0 for a PIE's header), so every `-static-pie` release program (the fork's first linux-x64 release build) stopped on SIGSEGV before `main`.; 0.16.3: a started context has 8 MiB of stack, its lowest page a guard (it was 256 KiB, unguarded) | With the spec. |
| `speak-agent/openkal-windows` | mcpplibs/openkal-windows 0.10.1 (1332fb6) | `dlopen` (PR #1; tags 0.11.0 and 0.11.1) | the two operations (`VirtualProtect`).; 0.11.1: a started context reserves 8 MiB of stack (it was the program header's 1 MiB) | With the spec. |
| `speak-agent/openkal-macos` | mcpplibs/openkal-macos 0.12.0 (f714a39) | `dlopen` (PR #1; tags 0.13.0 to 0.13.4) | the two operations (`mprotect`); 0.13.1: `kal_env_*` ask the system (`_NSGetArgc`/`_NSGetArgv`/`_NSGetEnviron`) before either entrance has recorded the vectors -- a C library started by an earlier constructor got no arguments; 0.13.2: the three names in `port/libSystem.tbd` and in the independence check; 0.13.3: no 512-entry cap on the arguments and environment.; 0.13.4: a started context has 8 MiB of stack (it was the thread library's 512 KiB, which Clang ran off the end of in mcppls); standalone, on a stack mapped there and entered through `okm_call_on_stack` | With the spec. |
| `speak-agent/openkal-musl` | mcpplibs/openkal-musl 0.19.2 (20b9268) | `dlopen` (PR #1; tags 0.19.3 to 0.19.9) | `dlopen`/`dlsym`/`dladdr` in a static program: an ELF loader above openkal (x86_64, aarch64), host-first binding, dynamic TLS, loaded objects in `dl_iterate_phdr`; `examples/dlopen`; 0.19.4: on macOS a constructor that precedes the library's start (libmc++'s plugin registrations) brings it up on first use -- the allocator and per-context state (`examples/early-constructor`); 0.19.5: openkal-macos 0.13.1, so that `main` then still has its arguments and environment; 0.19.6: openkal-macos 0.13.2 (the cross-link probe records the three names); 0.19.7: `posix_spawn` and the start's argument and environment stores take any number of entries (a static link of C-xlings passes thousands of objects: E2BIG before), with openkal-linux 0.16.1 and openkal-macos 0.13.3; 0.19.8: openkal-linux 0.16.2, and `examples/dlopen` runs as a release build in CI too; 0.19.9: those three releases, and `examples/threads-stack` (a started thread recursing through 4 MiB of frames) on every row. mcpp-safe's root names it by git tag, and each member with unit tests names it the same way among its `[dev-dependencies]` (a `mcpp test -p` graph of its own, which the root's declaration does not settle: those linked upstream's 0.19.2, whose start a constructor on macOS cannot use, and eight test programs crashed there before their first line; a dev-dependency is not handed to the member's dependents, whose graphs it would clash in) (0.19.9 satisfies openkal-llvm-runtime's `^0.19.2`). | With the spec; PE and Mach-O loading not yet. |

## Planned

None. Thread stacks (planned here until 2026-10-01: openkal-linux's were a fixed 256 KiB) are 8 MiB in all three implementations since openkal-linux 0.16.3, openkal-macos 0.13.4 and openkal-windows 0.11.1 (openkal-musl 0.19.9). libmc++ still runs Clang's calls on a 16 MiB stack of its own on ELF targets (`mcxx_call_on_stack`); on Mach-O and COFF, where that runs on the thread's own stack, the 8 MiB is what Clang has.

## Local layout during development

Forks are checked out under `mcpp-safe/forks/`, each its own git repository, and `.gitignore`
excludes the directory:

```
mcpp-safe/
  modules/ ...                        libmc++
  forks/
    llvm-clang-dev/                   speak-agent/llvm-clang-dev
    mcpp-language-server/             speak-agent/mcpp-language-server, branch mcxx-engine
```

The fork's `mcpp.toml` depends on libmc++ by path (`../../modules/{msa,backend/semantic,lsp}`, and
`../../plugins/{std,libs}` so the editor applies the same plugins a compile does).
`modules/backend/clang` takes Clang's libraries by version from this repository's own index
(`index/llvm`, tags of speak-agent/llvm-clang-dev), so building needs no checkout of the fork. A
package consumed from outside its own workspace must state its dependencies' versions, because
mcpp resolves `x.workspace = true` only for members of the consumer's workspace.

A fresh checkout: `git clone Sunrisepeak/mcpp-safe`, then clone the two forks into `forks/`, then
`mcpp build` in `forks/mcpp-language-server`.
