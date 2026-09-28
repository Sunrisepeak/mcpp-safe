# 开发方式：构建、测试、可观测性

目标：改一处代码后，只重建、只重测受影响的部分，在几十秒内得到结果；遇到问题时，从日志、trace 和计数器就能直接看出时间和工作花在了哪里。

## 1. 构建：只构建改动的部分

| 层 | 怎么复用 | 实测 |
|---|---|---|
| Clang/LLVM（`llvm.clang-dev`、`codegen-dev`、`clang-driver`） | **始终按版本**从本仓库的 `index/llvm` 使用。mcpp 按（版本 + 编译标志）把目标文件放进全局缓存 `~/.mcpp/build-cache`，不同项目、不同 workspace 之间共用 | 第一个使用方编译用 154 s，第二个项目只用 13.5 s |
| libmc++ 的后端（`modules/backend/clang`） | 拆成分区：`:support`、`:store`、`:unit`、`:completion`、`:index`，外加 `workspace.cpp`。改一个文件只重编它自己和依赖它的部分 | 改 `workspace.cpp` 后重建约 22 s；此前整个 `clang.cpp` 重编，还要加上 mcpp 的额外开销 |
| mcpp 本身 | 在 `.xlings.json` 中固定为 2026.9.28.2（`forks/` 下的仓库也继承）。2026.9.28.3 对每个 workspace 成员都单独走一遍解析和校验，空跑要 79 s；2026.9.28.2 只要 6 s | CI 也固定在同一个版本 |

规则：
- **不要给 Clang 库打新 tag，除非它的内容确实变了。** 新版本意味着所有使用方各重编一次（约 2.5 分钟）。同一个依赖图里只能有一个版本。
- **llvm-clang-dev 仓库内部**的 smoke 程序通过路径依赖使用各个包，每个程序在自己的目录里完整构建一次依赖，之后是增量构建。把整个仓库设成一个 mcpp workspace 并不能让成员共享依赖（实测干净构建 25 分钟、5 次构建），所以没有采用。
- 构建目录会越积越多：`mcpp clean --stale`；磁盘紧张时，删除 `forks/*/target` 下的旧指纹目录。

## 2. 测试：从快到慢分层

| 层 | 命令 | 耗时 | 覆盖内容 |
|---|---|---|---|
| 源码规则 | `python3 tools/checks/lint.py [根目录...]` | < 1 s | `clang-exposure`：Clang 只能出现在 `modules/backend/clang*`；`json-brace-init`：`Json x { expr }` 会得到 `[expr]`（编译器本身也能捕获） |
| 纯逻辑单元测试 | `mcpp test -p modules/base`、`-p modules/graph` | 几秒 | trace、扫描器、模块图 |
| LSP 层 | `mcpp test -p modules/lsp` | 约 3 s | 假后端（`FakeWorkspace`）：UTF-16 位置换算、诊断推送、跳转、悬停、引用、symbolInfo、调用层级 |
| 门禁和内置 provider | `mcpp test -p modules/features` | 几秒 | 只用事实：`mc++.iso` 的 15 个特性、profile（safe、modules、strict，多个同时使用）、Plan、作用域、豁免、未知的 id；插件覆盖内置特性、冲突、重新定义 profile、取代整个 provider |
| 插件（规则、过滤器） | `mcpp test -p plugins/std`、`-p plugins/libs` | 几秒 | 只用事实和文本：`raw-pointers`、`lib:std.vector`、`[[mcpp::cfg]]` 和 `ext:cfg`（profile portable）、json-brace-init（包括没有 nlohmann 时不问它） |
| 门禁 fixture | `mcpp build -p tools/conformance`，然后运行 `mcxx-conformance conformance/gates` | 约 10 s | 19 个特性，每个至少 5 正 5 反，精确率和召回率都要 100% |
| 后端 | `mcpp test -p modules/backend/clang` | 构建约 15 s，运行 0.3 s | 在临时目录中生成不用标准库的模块程序：解析、实体、跨模块跳转、失败根因、缓冲区覆盖、诊断 code |
| 真实工程 | `tools/probe`：`mcxx-probe --db DIR --resource DIR --cache DIR [--index] FILE [LINE:COL METHOD]...` | 秒级到分钟级 | 真实编译数据库上的完整流程 |
| 编辑器（fork） | `forks/mcpp-language-server`：`mcpp test`，以及 conformance fixture（`--core-engine mcxx`） | 分钟级 | 58 个 linux fixture |

修一个缺陷时，先在对应层写一个会失败的测试，再修。

fixture 的工作目录（`--workspace-dir`、`--cache-dir`）要放在 mcpp-safe 目录树之外，例如 `~/.cache/mcxx-fixtures`：xlings 会沿用上层目录的 `.xlings.json`（mcpp-safe 的只声明了 mcpp），fixture 里调用的宿主 `c++` 就会解析不到。CI（本仓库 `ci.yml`、fork 的 `mcxx.yml`、llvm-clang-dev 的 `ci.yml`）是复核，不是主要的验证手段。

## 3. 门禁的开销

测量方法：
- 文件：一个 4000 个函数的主文件，共 1.6 万个声明，没有违规；
- 构建：dev 构建（未优化）；
- 命令：`mcxx check`；
- 取值：5 次里最好的一次；
- 门禁各段的耗时用 `MCXX_LOG=gates=debug` 查看。

| 包的配置 | 总时间 | 事实收集 |
|---|---|---|
| 没有配置（json-brace-init 默认开启，但文件里没有 nlohmann） | 0.15 s | 不遍历 |
| 只开 `goto` | 0.15 s | 0.003 s |
| 只开 `c-array`（需要声明） | 0.20 s | 0.053 s |
| `strict`（13 个特性） | 0.24 s | 0.089 s |

优化前，`strict` 的事实收集要 0.097 s，而且不论配置如何，每个文件都要遍历一次。做过的三处优化：

| 优化 | 原因 |
|---|---|
| 名字 token 的范围不再逐个规范化文件路径 | 每个声明都要做一次 |
| 类型文本只在需要时计算 | 单独作为 `declaration_types` 一类事实；被标记的声明（C 数组、裸指针）仍然带类型文本 |
| 按 `requires_declaration` 跳过特性 | 例如文件里没有 nlohmann 时不问 json-brace-init |

规则本身的耗时在 1 ms 以内。

真实代码里，主文件的声明只占解析工作的一小部分，大部分时间花在头文件和导入的模块上。所以上面这个全部代码都在主文件里的例子是最坏情况。

## 4. 可观测性：`mcxx.base.trace`

| 用途 | 做法 |
|---|---|
| 看更多日志 | `MCXX_LOG=info`；`MCXX_LOG=debug`；`MCXX_LOG=info,modules=debug,parse=debug`（先写默认级别，再按类别覆盖） |
| 类别 | `modules`（BMI 构建和复用）、`parse`、`index`、`complete`、`workspace`、`lsp`（每个请求）、`gates`（`gates.facts`、`gates.rules`、计数 `gates.idle`；mcxx 驱动的编译也适用） |
| 时间线 | `MCXX_TRACE=/tmp/mcxx.json`：每个区间写成一个 Chrome trace 事件，可用 Perfetto（ui.perfetto.dev）或 chrome://tracing 打开，按线程看模块构建、索引、解析在哪里等待 |
| 计数器 | `msa::Status::counters`，例如 `modules.built`、`modules.reused`、`modules.failed`、`parse.parse`、`index.unit`、`lsp.textDocument/hover`。mcppls 的 `mcppls report` 里在 `engines[mcxx].details.counters` 下 |
| 慢操作 | 区间超过阈值（默认 2 s；补全 500 ms；LSP 请求 1 s）会自动升到 info 级别，不需要打开 debug 也能看到 |
| 库不占用标准错误 | 日志交给宿主的回调（`msa::Workspace::Options::log`，带级别和类别）；Clang 自己的输出（例如 "N errors generated."）也关掉了 |

新代码的约定：一个会花时间的操作就包一个 `trace::Span`，失败用 info 级别并写明原因，其余用 debug 级别。
