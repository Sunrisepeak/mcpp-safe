# mcpp-safe：MC++

MC++ 是一个以 C++ 模块为中心、特性可控、支持插件的 C++ 编译器与语义工具集。它先以 Clang 23.1 作为后端起步，逐步由自研前端替换。仓库里有三部分：

- **mcxx 驱动**（[`src/`](src/README.md)）：`mcxx c++ …` 就是进程内的 clang，同时在每次编译中运行 MC++ 的插件；`mcxx check …` 只做检查。
- **libmc++**（[`modules/`](modules/README.md)）：MC++ 的可复用部分，每个组件是一个 mcpp 包。mcppls（speak-agent/mcpp-language-server 的 `mcxx-engine` 分支）在进程内使用它，已经不需要 clangd。
- **插件**（[`plugins/`](plugins/README.md)）：`mc++.policy`（例如能不能用裸指针）、`[[mcpp::cfg(...)]]`（扩展）、nlohmann::json 的 `json-brace-init`。它们和第三方插件走同一个 SDK。

**MC++ 本身就是一个插件系统**（[`modules/plugin`](modules/plugin/README.md)）：
- 内置的 ISO C++ 特性控制（`mc++.iso`：每一项都用标准的 stable name 标明，都是减法，去掉后仍然是 ISO C++）也是 SDK 上的一个 provider。
- 插件可以控制、扩展，也可以覆盖内置的实现。
- profile `safe` 针对编译器不检查的未定义行为来源，`modules` 要求所有依赖都通过 import，`portable` 禁止一切 MC++ 专有扩展。
- `mcxx features` 列出程序里的全部 provider、特性和 profile。

## 目录

```
mcpp-safe/
├── src/                       mcxx 驱动（根包）：不包含 Clang 头文件
├── modules/                   libmc++
│   ├── base/ os/ arch/ testing/
│   ├── msa/                   MC++ 语义 API 与事实（MC3）
│   ├── graph/                 模块扫描与模块图
│   ├── plugin/sdk/            插件 SDK（MC4）：provider、规则、源码过滤器、profile、Catalog
│   ├── features/              特性门禁（MC1），以及内置 provider mc++.iso（ISO 特性控制）
│   ├── lsp/                   LSP 形态的语义服务（MC6）
│   └── backend/               后端：semantic/、compiler/（两个门面），clang/、clang-compiler/（只有这两个包含 Clang）
├── plugins/                   插件实现：std/（policy、cfg），libs/（json）
├── specs/                     规范 MC1、MC3、MC4、MC5：正文、JSON Schema、示例
├── conformance/               门禁 fixture（gates/），规范条目的可追溯性（traceability.json）
├── tools/                     probe、conformance（fixture runner）、checks（源码规则、规范检查）
├── index/                     本仓库自建的 mcpp 包索引（llvm.*、microsoft.*）
├── forks/                     被 .gitignore 忽略：speak-agent 下各 fork 的检出（llvm-clang-dev、mcpp-language-server）
└── .agents/docs/              方案、里程碑、进度、开发方式、检查点
```

每个目录都有自己的 README.md。约定：Clang 只能出现在 `modules/backend/clang*`（`tools/checks/lint.py`，CI 强制）；插件核心在 `modules/`，插件实现在 `plugins/`。

## 构建与测试

```
mcpp build                                   # mcxx 驱动（首次会下载并构建 Clang 库，之后走全局缓存）
mcpp test -p modules/lsp                     # 各包分别测试；分层说明见 .agents/docs/development.md
python3 tools/checks/lint.py                 # 源码规则
python3 tools/checks/specs.py --mcxx <mcxx>  # 规范：schema、示例、要求 id 与可追溯性
<mcxx> features                              # 这个 mcxx 能门禁什么：provider、特性、profile
MCXX_LOG=info MCXX_TRACE=/tmp/t.json …       # 日志与时间线
```

mcpp 版本固定在 `.xlings.json`（2026.9.28.2）。计划与进度：`.agents/docs/2026-09-28-mcxx-architecture-plan.md`、`2026-09-28-mcxx-milestones-acceptance.md`、`progress.md`。

## 缘起

最初的问题（原文保留）：

如果要自己实现 cpp ls 特别是 module 的支持，以及 要做 mcpp 构建插件 编译期检测 和 C++ 语言特性 / 标准 更细节的 控制 和 自定义 (例如禁止用指针 静止vector) 以及 类似 部分的 生命周期检查器等 自定义 哪些C++ 特性可以使用 代码风格等 或 不能使用 变成一种真实强制的 编译期检查 而不是 只是文档规定，是不是只要 实现 AST 就可以了 （核心） 而且 mcppls 和 mcpp safe 两个项目可以复用？

只读仓库:

- /home/speak/workspace/github/mcpp-community/mcpp
- /home/speak/workspace/github/mcpp-community/mcpp-plugins
- /home/speak/workspace/github/mcpp-language-server
