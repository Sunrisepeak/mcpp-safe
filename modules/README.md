# modules：libmc++

libmc++ 是 MC++ 的可复用部分。每个目录都是一个独立的 mcpp 包，**按依赖集合拆分**：一个包只依赖它确实需要的东西，使用方只链接它确实用到的包。mcpp 会把一个包的全部目标文件链接进使用方，所以包的边界同时也是体积和依赖的边界。

## 分层

```
            ┌──────────────── 使用方 ────────────────┐
            │ mcxx 驱动（src/）   mcppls（fork）   插件（plugins/）│
            └────────────────────────────────────────┘
                     │                 │                 │
   backend/compiler ─┘   backend/semantic   lsp          │
          │                    │              │          │
          │           ┌── features ── plugin/sdk ────────┘
          │           │        │
 backend/clang-compiler   backend/clang ── graph
          │                    │
          └──── llvm.*（index/llvm）   msa ── base / os / arch
```

| 层 | 包 | 允许依赖 |
|---|---|---|
| 基础 | `base`、`os/*`、`arch/*`、`testing` | 只有 openkal-llvm-runtime |
| 语义 API | `msa` | 只有 std |
| 模块事实 | `graph` | std |
| 插件核心 | `plugin/sdk` | `msa` |
| 插件的进程外格式与两端 | `plugin/wire`、`plugin/remote`、`plugin/host` | `msa`、`plugin/sdk`、nlohmann.json；`host` 另外依赖 `features`、`base` |
| 驱动 | `driver` | 上面各层和两个门面（不含 Clang 头文件） |
| 门禁与内置 provider | `features`（门禁、`mc++.iso`） | `base`、`msa`、`plugin/sdk` |
| 服务 | `lsp` | `base`、`msa`、nlohmann.json |
| 自己的前端 | `frontend` | 只有 `msa`（不依赖 Clang/LLVM） |
| 门面 | `backend/semantic`、`backend/compiler` | 各自的后端实现（不含 Clang 头文件） |
| **后端（Clang）** | `backend/clang`、`backend/clang-compiler` | 上面各层，加上 `llvm.*`。**只有这两个包可以出现 Clang** |

规则：
- **Clang 的暴露面收在 `backend/clang*` 里。** 其他地方包含 `<clang/…>`、`<llvm/…>` 头文件，或者依赖 `llvm.*` 包，都会被 `tools/checks/lint.py` 的 `clang-exposure` 规则报错（CI 强制）。使用方通过 MSA（`msa`）、事实（`msa::fact`）和两个门面接触后端；以后 MC++ 自研前端接进来时，这些使用方都不用改。
- **插件核心和插件实现分开。** 插件核心在 `plugin/`，门禁引擎在 `features/`；插件实现在仓库根目录的 `plugins/`，和第三方插件走同一条路。
- **MC++ 自己也是 provider。** 内置的 ISO 特性控制 `mc++.iso` 在 `features/` 里，同样针对 SDK 编写，用 `Origin::builtin` 注册；插件可以替换其中任何一项（`plugin/README.md` 的"覆盖"）。
- **平台差异只用常量表达**（`os/*`、`arch/*`，用 `if constexpr`），不写 `#ifdef`。

## 各包一览

| 目录 | 模块 | 作用 | 对应里程碑 |
|---|---|---|---|
| [`base`](base/README.md) | `mcxx.base.*` | 错误、路径、文本、URI、sha256、日志、trace、TOML | — |
| [`os`](os/README.md)、[`arch`](arch/README.md) | `mcxx.os`、`mcxx.arch` | 平台常量 | P9 |
| [`testing`](testing/README.md) | `mcxx.testing` | 测试框架 | — |
| [`msa`](msa/README.md) | `mcxx.msa` | MC++ 语义 API：位置、诊断、实体、Unit/Workspace 接口、事实（MC3） | M0.4 |
| [`graph`](graph/README.md) | `mcxx.graph` | 按词法扫描模块声明，建立模块图 | — |
| [`plugin`](plugin/README.md) | `mcxx.plugin`、`.wire`、`.remote`、`.host` | 插件 SDK（MC4）与进程外插件：provider、Catalog、协议 1、故障隔离 | M0.6 |
| [`serve`](serve/README.md) | `mcxx.serve` | `mcxx serve`（MC6）：LSP 基础协议之上的语义服务，以及带崩溃重启的客户端 | M1.3 |
| [`frontend`](frontend/README.md) | `mcxx.frontend` | MC++ 自己的前端 F1：和 Clang 逐 token 一致的词法、模块单元的预处理、由此得到的 MC3 事实；不依赖 Clang | M1.6 |
| [`driver`](driver/README.md) | `mcxx.driver` | mcxx 驱动（MC5）：命令、`mcxx features`、`mcxx compose` | M0.5、M0.6 |
| [`features`](features/README.md) | `mcxx.features`、`mcxx.features.iso` | 特性门禁（MC1）：配置、profile、作用域、级别、豁免、审计、Plan；内置 provider `mc++.iso`（ISO 特性控制） | M0.3、M0.7 |
| [`lsp`](lsp/README.md) | `mcxx.lsp` | 基于 MSA 的 LSP 形态服务（MC6），不含传输层 | M1.3 |
| [`backend`](backend/README.md) | `mcxx.backend*` | 后端：两个门面，以及基于 Clang 23.1 的两个实现 | M0.4、M0.5 |

## 构建与测试

```
mcpp test -p modules/<包>          # 每个包各自测试；见 ../.agents/docs/development.md 的分层表
python3 tools/checks/lint.py       # 源码规则：clang-exposure、json-brace-init
```
