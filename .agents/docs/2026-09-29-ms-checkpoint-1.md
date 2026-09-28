# MS 检查点 1：mcppls fork 完全不用 clangd

日期：2026-09-29。平台：linux-x64（本机，32 核）。对应里程碑文档的 MS 节。

## 达到的状态

- fork（speak-agent/mcpp-language-server，分支 `mcxx-engine`）的默认核心引擎是 `mcxx`：libmc++ 在进程内提供 C++ 语义，底层是 Clang 23.1.0 的库，不启动也不附带 clangd。clangd 引擎代码仍在，用 `--engine clangd` 启用。
- mcppls 只依赖 `mcxx.msa`、`mcxx.backend`、`mcxx.lsp`，不接触 Clang 类型（`mcxx.backend` 是门面，`BackendInfo` 负责给出 kit 需要的 libc++ 版本）。
- payload 不再带 clangd，改为带 `mcxx/resource/include`（Clang 的 builtin headers，从 lock 里 `llvm-project-src` 的 `clang/lib/Headers` 取出，296 个文件，8 MB），`payload.json` 新增 `engines.mcxx`。

## 验收项状态

| 项 | 状态 | 依据 |
|---|---|---|
| AS.1.1 默认引擎 mcxx，clangd 保留 | 达成 | settings registry、VS Code 配置、`--engine` |
| AS.1.2 原 clangd 功能由 mcxx 提供 | 达成（linux） | 跳转、声明、类型定义、实现、引用、高亮、悬停、补全、签名帮助、诊断（推送和拉取两种）、语义高亮、文档符号、工作区符号、symbolInfo、调用层级（outgoing）都有 fixture 覆盖 |
| AS.2.1 payload 不含 clangd | 达成（linux） | `payload --verify` 通过；58 个 fixture 都用这个 payload 运行 |
| AS.2.2 payload 体积不大于切换前 | 未测 | 需要 release 构建来比较；dev 构建的 server 有 157 MB（含调试信息） |
| AS.3.1 conformance 全部通过 | linux 达成（58/58） | 11 项 clangd 专属检查或 fixture 用 `only-engine` 标出后跳过；三个平台和 self-mcpp、real-xlings、self-mcppls 还没跑 |
| AS.3.2 冷启动 < 12 s、热启动 < 5 s | 达成（linux） | timing fixture 五轮：冷启动 2.48 s，热启动 0.59 s；同机 clangd 为 3.58 s 和 0.81 s |
| AS.3.4、AS.4.x | 未开始 | |

## 过程中修掉的问题（libmc++ 侧）

- 模块失败的诊断放在 import 上，并写明根因（`module X cannot be built because Y did not compile`）；不再重复报 Clang 的 `module_not_found`。状态里用 `modules-doomed` 表示，类别是 code。
- 编辑器里未保存的模块接口缓冲会覆盖到模块构建上，importer 看到的是未保存的内容；编辑接口后，它本身和所有依赖它的模块都会失效重建。
- 驱动拒绝的编译命令进入状态（`module-scan-failed`，类别 environment）、报告和诊断。
- 诊断的 code 和 clangd 一致，取 Clang 诊断名并去掉 `err_`、`warn_`、`ext_` 前缀。
- `workspace/symbol` 支持带作用域的查询；新增 `textDocument/symbolInfo`（给出 USR）和调用层级；悬停时显示常量的值。
- 引擎把编译数据库写到缓存里，供诊断包使用；索引进度用标准 `$/progress` 上报；一个 plan 在 libmc++ 接收之前也算作正在索引，这样一次性的 CLI 查询会等它。

## 未完成

- CI：fork 的 `mcxx.yml` 需要能读取私有仓库 mcpp-safe 的 token（secret `MCPP_SAFE_TOKEN`）；llvm-clang-dev 的 CI 已经加上。
- darwin-arm64、linux-arm64、windows-gnu。
- callHierarchy/incomingCalls（mcppls 的查询用引用加外框自己算，编辑器里直接调用时还没有）。
- openkal 线程栈固定为 256 KiB，目前靠换栈绕开（见 forks.md）。
