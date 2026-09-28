# backend：后端，以及 Clang 的边界

libmc++ 里所有和具体编译器前端相关的代码都在这个目录下。它分两部分：**门面**（使用方依赖的接口，不涉及任何后端）和**实现**（今天是 Clang 23.1）。

| 目录 | 包 / 模块 | 谁使用 | 含 Clang |
|---|---|---|---|
| [`semantic`](semantic/README.md) | `mcxx-backend` / `mcxx.backend` | mcppls、服务、工具：要一个 `msa::Workspace` | 否 |
| [`compiler`](compiler/README.md) | `mcxx-backend-compiler` / `mcxx.backend.compiler` | mcxx 驱动：执行一条编译命令 | 否 |
| [`clang`](clang/README.md) | `mcxx-backend-clang` / `mcxx.backend.clang` | `semantic` 门面 | **是** |
| [`clang-compiler`](clang-compiler/README.md) | `mcxx-backend-clang-compiler` / `mcxx.backend.clang.compiler` | `compiler` 门面 | **是** |

## 边界规则

- **只有 `clang/` 和 `clang-compiler/` 可以包含 Clang/LLVM 头文件、依赖 `llvm.*` 包。** `tools/checks/lint.py` 的 `clang-exposure` 规则检查整个仓库，CI 中强制执行。
- **门面只转发，不暴露后端类型。** `mcxx.backend` 的接口只有 `msa::` 类型；`mcxx.backend.compiler` 的接口只有命令行和退出码。
- **语义和编译分成两个包**：mcpp 会把一个包的全部目标文件链接进使用方。mcppls 只需要语义，不应该带上代码生成器（`llvm.codegen-dev`、`llvm.clang-driver`）。实测 release 版 mcppls 为 53 MB，mcxx 驱动（dev）为 154 MB。
- **Clang 库按版本从本仓库的 `index/llvm` 取**（`llvm.clang-dev`、`codegen-dev`、`clang-driver`，版本 23.1.0.2，来自 speak-agent/llvm-clang-dev），由 mcpp 的全局缓存复用。

## 以后

MC++ 自研前端（M1.6 起）作为第三个实现放在这里，例如 `frontend/`，并在门面后面和 Clang 并存：先回答能确定的查询，不确定的交给 Clang（MSA 的 `Certainty`，计划 P4）。使用方不需要改动。
