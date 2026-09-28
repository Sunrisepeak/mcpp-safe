# Forks and derived repositories (early stage)

Under the early-stage rules (milestones doc §1.0), related repositories are forked under the
`speak-agent` account. Nothing is filed or pushed to the official repositories. This file lists
every fork, what it carries, and what it would take to merge back (a decision for after MS).

| Repository | Upstream | Branch | What it carries | Merge back |
|---|---|---|---|---|
| `speak-agent/llvm-clang-dev` | llvm/llvm-project 23.1.0 (a subset kept verbatim under `llvm/`, see `UPSTREAM-REV`) | `main` | Clang/LLVM frontend libraries built by mcpp on openkal (package `llvm.clang-dev` 23.1.0): generated config and TableGen output in `llvm-generated/`, generators in `tools/`, three link stubs in `port/`. Upstream code is unpatched. | Not an upstream fork in the usual sense: a build of it. It reaches mcpp's index as E-IDX-1. |
| `speak-agent/mcpp-language-server` | Sunrisepeak/mcpp-language-server (377d222) | `mcxx-engine` | The `mcxx` engine: libmc++ in process (`mcxx.backend` + `mcxx.lsp`) as the default core engine, with clangd kept as `--engine clangd`. Other changes: a payload without clangd (`mcxx/resource`, `engines.mcxx`); kit matching by the backend's libc++ release; the conformance runner's `--core-engine` and `only-engine`; clangd-only fixtures and checks marked. | E-LS-1…E-LS-5. After MS, upstream's decision (R5). |

## Planned

| Repository | Why | When |
|---|---|---|
| `speak-agent/openkal-linux` | Thread stacks are a fixed 256 KiB (`task.cpp` `kStack`; the pthread attribute's size is ignored) with no guard page. libmc++ works around it by switching every Clang call onto its own 16 MiB stack (`mcxx_call_on_stack`). The fork would honour `pthread_attr_setstacksize`. | When the workaround gets in the way (R3/R4); not needed for MS. |

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

The fork's `mcpp.toml` depends on libmc++ by path (`../../modules/{msa,backend,lsp}`).
`modules/clang` depends on `../../forks/llvm-clang-dev`. A package consumed from outside its own
workspace must state its dependencies' versions, because mcpp resolves `x.workspace = true` only
for members of the consumer's workspace. So libmc++'s members state `openkal-llvm-runtime` and
`nlohmann.json` versions explicitly.

A fresh checkout: `git clone Sunrisepeak/mcpp-safe`, then clone the two forks into `forks/`, then
`mcpp build` in `forks/mcpp-language-server`.
