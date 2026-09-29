# Forks and derived repositories (early stage)

Under the early-stage rules (milestones doc §1.0), related repositories are forked under the
`speak-agent` account. Nothing is filed or pushed to the official repositories. This file lists
every fork, what it carries, and what it would take to merge back (a decision for after MS).

| Repository | Upstream | Branch | What it carries | Merge back |
|---|---|---|---|---|
| `speak-agent/llvm-clang-dev` | llvm/llvm-project 23.1.0 (a subset kept verbatim under `llvm/`, see `UPSTREAM-REV`) | `mcxx` (PR #1; packaging revisions are tags: 23.1.0 … 23.1.0.4; since 23.1.0.3 the default target is x86_64-unknown-linux-gnu over the openkal host, since 23.1.0.4 openkal's Windows and macOS targets build too) | Clang/LLVM frontend libraries built by mcpp on openkal (package `llvm.clang-dev` 23.1.0): generated config and TableGen output in `llvm-generated/`, generators in `tools/`, three link stubs in `port/`. Upstream code is unpatched. | Not an upstream fork in the usual sense: a build of it. It reaches mcpp's index as E-IDX-1. |
| `speak-agent/mcpp-language-server` | Sunrisepeak/mcpp-language-server (377d222) | `mcxx-engine` | The `mcxx` engine: libmc++ in process (`mcxx.backend` + `mcxx.lsp`) as the default core engine, with clangd kept as `--engine clangd`. Other changes: a payload without clangd (`mcxx/resource`, `engines.mcxx`); kit matching by the backend's libc++ release; the conformance runner's `--core-engine` and `only-engine`; clangd-only fixtures and checks marked. | E-LS-1…E-LS-5. After MS, upstream's decision (R5). |
| `speak-agent/openkal` | mcpplibs/openkal 0.14.0 (b0c3b5b) | `dlopen` (PR #1; tag 0.15.0) | `openkal.exec` 0.15: `kal_exec_publish_part` and `kal_exec_granularity`, what a loader above openkal needs (SPEC §11 entry 21); conformance observations. | Upstream's decision after MS: a spec revision (clause 8: two declarations added). |
| `speak-agent/openkal-linux` | mcpplibs/openkal-linux 0.15.0 (9ef9720) | `dlopen` (PR #1; tag 0.16.0) | the two operations; `_start` relocates a `-static-pie` program (relative relocations, `DT_RELR`) and biases the TLS image. | With the spec. |
| `speak-agent/openkal-windows` | mcpplibs/openkal-windows 0.10.1 (1332fb6) | `dlopen` (PR #1; tag 0.11.0) | the two operations (`VirtualProtect`). | With the spec. |
| `speak-agent/openkal-macos` | mcpplibs/openkal-macos 0.12.0 (f714a39) | `dlopen` (PR #1; tags 0.13.0, 0.13.1, 0.13.2) | the two operations (`mprotect`); 0.13.1: `kal_env_*` ask the system (`_NSGetArgc`/`_NSGetArgv`/`_NSGetEnviron`) before either entrance has recorded the vectors -- a C library started by an earlier constructor got no arguments; 0.13.2: the three names in `port/libSystem.tbd` and in the independence check. | With the spec. |
| `speak-agent/openkal-musl` | mcpplibs/openkal-musl 0.19.2 (20b9268) | `dlopen` (PR #1; tags 0.19.3 to 0.19.6) | `dlopen`/`dlsym`/`dladdr` in a static program: an ELF loader above openkal (x86_64, aarch64), host-first binding, dynamic TLS, loaded objects in `dl_iterate_phdr`; `examples/dlopen`; 0.19.4: on macOS a constructor that precedes the library's start (libmc++'s plugin registrations) brings it up on first use -- the allocator and per-context state (`examples/early-constructor`); 0.19.5: openkal-macos 0.13.1, so that `main` then still has its arguments and environment; 0.19.6: openkal-macos 0.13.2 (the cross-link probe records the three names). mcpp-safe's root names it by git tag (0.19.6 satisfies openkal-llvm-runtime's `^0.19.2`). | With the spec; PE and Mach-O loading not yet. |

## Planned

| Repository | Why | When |
|---|---|---|
| `speak-agent/openkal-linux` (thread stacks) | Thread stacks are a fixed 256 KiB (`task.cpp` `kStack`; the pthread attribute's size is ignored) with no guard page. libmc++ works around it by switching every Clang call onto its own 16 MiB stack (`mcxx_call_on_stack`). The fork would honour `pthread_attr_setstacksize`. | When the workaround gets in the way (R3/R4); not needed for MS. |

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
