# MC++ 架构方案与验收标准（v0，草案）

日期：2026-09-28
状态：待 review
依据：同目录下的三份文档：
[`mcpp-safe-feasibility`](2026-09-28-mcpp-safe-feasibility.md)、
[`universal-bmi-analysis`](2026-09-28-universal-bmi-analysis.md)（含 CI 实测）、
[`own-cpp-frontend-discussion`](2026-09-28-own-cpp-frontend-discussion.md)（第 8、9 节的决定）

## 0. 一句话

MC++ 是一个**以插件系统为核心、只面向 C++ 模块代码、特性可以细粒度控制的 C++ 编译器**。它有两种形态：

- `mcxx`：编译器程序，作为 mcpp 的工具链之一；
- **libmc++**：可复用的库，mcppls、mcpp-safe 以及第三方工具都可以直接嵌入。

前期以 Clang 为内核，自研前端在内部逐步接替 Clang。

## 1. 命名

| 名字 | 指什么 | 说明 |
|---|---|---|
| **MC++** | 项目，以及它定义的方言集合 | 对外名称 |
| `mcxx` | 编译器驱动程序 | 模块名里不能用 `+`，所以一律用 `mcxx` |
| **libmc++** | 可复用组件的总称，以 mcpp 包的形式发布 | 包名暂定为 `mcxx`，按 feature 选择要用的组件（仿照 `mcpp:plugins` 的做法） |
| `mcxx.*` | C++ 模块名前缀 | 例如 `mcxx.core`、`mcxx.msa` |
| `mc++.<name>` | profile 名 | 例如 `mc++.safe`、`gpu.device` |
| 仓库 | `Sunrisepeak/mcpp-safe`（私有） | 以后是否更名为 `mcxx`，见第 10 节 |

## 2. 目标与非目标

**目标**

1. **Modules**：只接受模块代码。`import` 通过模块图和 IFC 解析。
2. **特性和库的细粒度控制**：用特性注册表加 profile，可以作用到包、模块、命名空间、声明和区域。
3. **Safe 机制**：限制类特性，加上扩展类语义（生命周期注解、分析），在编译时强制执行。
4. **组件化**：每个组件都是 mcpp 包里的一个模块，可以单独复用，组合起来就是 libmc++。
5. **插件化**：编译器本身就是一个插件系统，提供规则、库控制、属性、区域/方言、分析、后端、输出七类扩展点。GPU 等能力以插件形式接入，不和核心耦合。
6. **全部走 mcpp 体系**：构建、依赖（包括 LLVM/Clang 开发库）、分发（插件就是 mcpp 包）都用 mcpp。

**非目标（当前阶段）**

- 向前兼容：头文件单元、PCH、C 模式、旧标准模式、GNU 扩展、ObjC++。
- 自有标准库 `std2`：M4 之前一律使用标准 `std`（第二轮 review 决定）。
- 自研代码生成：M4 之前由 Clang CodeGen 负责。
- MSVC ABI：Windows 先支持 `x86_64-windows-gnu`，和 mcppls 一致。

## 3. 架构原则

| # | 原则 | 如何保证 |
|---|---|---|
| P1 | **只有 `mcxx.backend.clang` 会 import Clang/LLVM**，其余组件只能看到 MSA | 分层检查（仿照 `mcppls-devtools check layers`），CI 强制 |
| P2 | 依赖严格自上而下：`base → core → {features, msa, modules} → {backend.clang, frontend} → {analysis, plugin, service} → driver` | 同上 |
| P3 | **会跨越边界的数据都要写成规范**：特性注册表、IFC 方言信息、MSA、插件 ABI、驱动契约、服务协议 | 第 5 节 MC1–MC6，规则带 id，并有可追溯的一致性测试 |
| P4 | **强制执行由真实编译完成**；辅助答案只有三种：违规、通过、不确定。"不确定"转给 Clang 后端处理 | MSA 的查询结果带 `certainty` 字段 |
| P5 | 插件边界是 **C ABI + 不透明句柄 + 版本化协议**；内部的 C++ 接口可以自由演进 | MC4；验收时要求插件用和 mcxx 不同的工具链构建 |
| P6 | **差分测试是常规手段**：自研前端和进程内的 Clang 后端对比同一批事实 | `devtools diff`；各阶段的一致率门槛 |
| P7 | 编译器和服务进程**不访问网络** | 沿用 mcppls 的原则 |
| P8 | 缺少的能力靠插件补充，而不是往核心里加特判 | 核心代码里不允许出现 `if (cuda)` 这种写法 |

## 4. 仓库和 workspace 结构（mcpp workspace）

```
mcpp-safe/                         mcpp workspace 根（虚拟 workspace：只有 [workspace]）
├── mcpp.toml                      [workspace] members；[workspace.package] 统一标准为 c++23 及以上；统一依赖版本
├── libs/                          ── libmc++ ──（一个包 `mcxx`，按 feature 选组件；或者每个组件一个包，见第 10 节）
│   ├── base/        mcxx.base          错误、文本、路径、arena、sha256；无依赖
│   ├── core/        mcxx.core          会话、选项、诊断（含 SARIF 输出）、源码管理
│   ├── features/    mcxx.features      特性注册表、profile、门禁引擎、作用域、配置解析（MC1）
│   ├── msa/         mcxx.msa           MC++ 语义 API：接口和值类型（MC3）
│   ├── modules/     mcxx.modules       模块图、IFC 读写（依赖 ifc-sdk）、方言信息（MC2）
│   ├── backend-clang/ mcxx.backend.clang  基于 Clang 23.1 实现 MSA；**唯一** import Clang 的地方
│   ├── frontend/    mcxx.frontend      自研前端：lex / parse（M1）→ decl（M2）→ expr（M3）
│   ├── analysis/    mcxx.analysis.*    CFG 和数据流分析（生命周期等）
│   ├── plugin/      mcxx.plugin        插件宿主和加载器（MC4）
│   └── service/     mcxx.service       面向编辑器和 Agent 的查询与诊断服务（MC6）
├── sdk/plugin/      mcxx.plugin.sdk    插件作者使用的 `import mcxx.plugin;` 封装（只依赖 C ABI 头层）
├── apps/mcxx/       mcxx               驱动：compile / check / emit-ifc / serve（MC5）
├── plugins/                        第一方插件（每个都是独立的 mcpp 包，走和第三方一样的路径）
│   ├── safe/        mc++.safe          首个 profile 和规则集
│   └── gpu/         gpu.*              M3 之后的原型
├── specs/                          MC1–MC6，各自独立版本化；schema、示例、可追溯性
├── conformance/                    一致性测试 fixture 和 runner（仿照 mcppls 的 conformance）
├── tools/devtools/                 分层检查、规范校验、差分测试 runner、语料统计
└── .agents/docs/                   设计记录（本目录）
```

### 4.1 外部依赖（全部通过 mcpp 引入）

| 依赖 | 版本 | 现状（本机实测） | 方案 |
|---|---|---|---|
| **LLVM/Clang 开发库**（头文件，以及 clangAST、Sema、Frontend、Serialization、Analysis、Tooling、CodeGen 和 LLVM Support/Core 等库） | **23.1.x**，与 mcppls 的 clangd 23.1.0 对齐 | **缺失**：xlings/mcpp 的 `xim-x-llvm` 22.1.8 只有二进制、libc++ 头文件和 resource dir；`xim-x-libllvm` 20.1.7 只有 `libLLVM.so`，没有头文件，也没有 Clang 库 | 在 mcpp index 中新增包，例如 `llvm-x-clang-dev@23.1.x`：**用 openkal 工具链（clang + libc++）构建**，这样才能和 mcxx 的 C++ ABI 一致（上游预编译包链接的是 libstdc++，不能用），覆盖 4 个平台。过渡方案：用 `mcpp.deps.cmake` 把 llvm-project 作为 prepare action 从源码构建，并缓存结果 |
| IFC SDK（`microsoft/ifc`） | 锁定一个 tag | 不在 index 中 | 打包为 mcpp 包 `ifc-sdk`（Apache-2.0 WITH LLVM-exception，C++20） |
| nlohmann.json、cmdline | 与 mcppls 相同 | 已在 index 中 | 直接使用 |

ABI 不稳定的代价（第二轮 review 已接受）：升级 Clang 版本 = 重新构建开发库 + 只修改 `backend.clang`。P1 保证影响不会扩散。

### 4.2 libmc++ 的复用方式

| 使用方 | 用哪些组件 | 方式 |
|---|---|---|
| mcpp（构建） | `mcxx` 驱动 | 作为工具链（`[toolchain]` 选择 mcxx）；对其他工具链（GCC 等），使用 `mcxx check` 作为 blocking check action（通过插件 `mcpp.tools.safe`） |
| mcppls | `core`、`features`、`msa`、`modules`、`service`，加上 `backend.clang` 或 `frontend` | **两种方式都支持**：进程内链接 libmc++，或者像 clangd 一样起一个 `mcxx serve` 子进程。先做子进程方式（隔离性好，崩溃不会拖垮 mcppls，和 mcppls 现有的引擎抽象一致） |
| 第三方工具 | 按需选择 | mcpp 包依赖，按 feature 选择组件 |
| 插件作者 | 只用 `mcxx.plugin.sdk` | 以 `[build-dependencies]` + `host-module = true` 的方式分发 |

## 5. 规范（MC1–MC6，仿照 mcppls 的 S1–S5：独立版本号、规则 id、可追溯性）

| 规范 | 内容 | 首个版本出现在 |
|---|---|---|
| **MC1 特性注册表与 profile** | 特性 id 命名规则；类别（限制类 / 扩展类）；检查层（语法 / 声明 / 表达式 / 控制流）；级别（allow / warn / deny）；作用域（包 / 模块 / 命名空间 / 声明 / 区域）；profile 文件格式；在 `mcpp.toml` 中如何配置；逃生口 `[[mcpp::allow("id")]]` 与审计输出；库控制（`lib:<模块或包>.<符号>`） | M0 |
| **MC2 IFC 方言信息** | 锁定的 IFC 版本；`VendorExtension` 的编码约定（模块的特性集、profile、跨方言边界标注、GCC/Clang 特有构造的编码） | M1 |
| **MC3 MSA** | 实体（声明、类型、表达式、区域、位置）和句柄；查询集（按阶段逐步增加）；`certainty` 语义；版本化 | M0（只含声明层和表达式层的查询） |
| **MC4 插件 ABI** | C ABI 入口与生命周期；不透明句柄；协议号；七类扩展点的回调契约；插件作为 mcpp 包分发和加载的方式；插件失败时的隔离 | M0（v1：规则和库控制两类扩展点）；M1 起加入属性和区域 |
| **MC5 驱动与工具链契约** | `mcxx` 的命令行（兼容 clang 的一个参数子集，用规范列出）；mcpp 如何调用它（toolchain-model）；输出物（对象文件、`.ifc`、`.pcm`、facts、SARIF）；和 `mcpp emit build-database`（mcppls 的 S1）的衔接；**参数规范化**（吸取 8.7 节参数漂移的教训：同一个单元无论经过哪条路径，参数都逐字节相同） | M0 |
| **MC6 服务协议** | `mcxx serve` 面向 mcppls 的协议：LSP 子集，加上事实和门禁查询的扩展；和 mcppls S3/S5 对齐 | M1 |

## 6. 里程碑与验收标准

每个里程碑都有**可以机器检查的验收项**（A 编号）。没有全部通过，就不进入下一阶段。测量环境默认是 GitHub `ubuntu-24.04` 4 核（和 8.6、8.7 节实测相同）；其他平台单独列出。

### M0 基座：Clang 内核 + 门禁 + 工具链

| # | 验收项 |
|---|---|
| A0.1 | workspace 在 linux-x64 上可以用 `mcpp build` 构建，`llvm-x-clang-dev@23.1` 由 mcpp 解析；CI 缓存命中时的构建时间有预算 |
| A0.2 | 分层检查通过：除 `backend.clang` 外，任何组件都不 import Clang/LLVM（P1、P2） |
| A0.3 | `mcxx check --syntax-only` 在 **mcppls 语料**（249 个文件，纯模块）上零错误，诊断和 clang 23.1 的 `-fsyntax-only` 一致 |
| A0.4 | `mc++.safe` v0：5–8 个限制类特性，加上 `lib:std.vector`。每个特性至少各有 5 个正例和反例 fixture。**跨模块 `auto v = make()`、别名 `Buf` 这两个用例必须检出**（与 GCC 插件探针的结果一致） |
| A0.5 | 用 `mcxx` 作为 mcpp 工具链构建 **mcppls**（代码生成由 Clang CodeGen 负责），产物通过 mcppls 自己的单元测试 |
| A0.6 | `mc++.safe` 以**插件**形式实现（不是内置），并且用**和 mcxx 不同的工具链**（GCC 16 + libstdc++）构建后仍能加载运行（证明 P5 成立） |
| A0.7 | MC1、MC3（第一版子集）、MC4 v1、MC5 v0 发布为草案，schema 校验和规则 id 可追溯性通过 |

### M1 libmc++ 的首批使用方

| # | 验收项 |
|---|---|
| A1.1 | **编辑器和构建看到的门禁结果相同**：mcppls（通过 `mcxx serve`）和 `mcxx check` 对同一批 fixture 给出**完全相同**的门禁诊断集合 |
| A1.2 | 用 GCC 构建的工程（mcpp 仓库），通过 `mcpp.tools.safe` 的 blocking check 执行 `mcxx check`：违规时构建失败，改正后构建通过 |
| A1.3 | 生成 IFC（T1 层）并附带方言信息（MC2 v1）：mcppls 语料的每个接口都产出 `.ifc`；`ifc-printer` 全部能读；把 IFC 读回 MSA 后，得到的 T1 事实和写出前**逐项相等** |
| A1.4 | 自研前端语法层（F1）：mcppls 和 mcpp 两套语料**零解析失败**；声明范围和 Clang 的差分一致率 ≥ 99.9% |
| A1.5 | mcppls 的 native 引擎（纯词法）换成 `mcxx.frontend` 的语法层之后，mcppls 现有的 conformance fixture 全部照旧通过 |
| A1.6 | 插件 ABI 扩展到属性和区域两类扩展点；一个示例属性插件通过测试 |

### M2 语义服务不再依赖 BMI 重建

| # | 验收项 |
|---|---|
| A2.1 | 自研前端声明层（F2）+ IFC import：T1 事实和 Clang 后端的差分一致率 ≥ 99% |
| A2.2 | **self-mcpp 冷启动首次跳转 ≤ 10 s**（实测基线：clangd 路径 88–126 s，要构建 174 个 BMI）。跳转、悬停、补全的声明部分由 libmc++ 回答，**不构建任何 BMI** |
| A2.3 | 热启动不会重建任何东西：日志中 `Built`/`rebuilt` 类事件数为 0（吸取 8.7 节的教训，作为回归检查） |
| A2.4 | 转给 Clang 的"不确定"查询在 mcppls 语料上的比例被统计出来，并公布 |

### ★ 里程碑 MS：mcppls 完全基于 MC++

- 定义：mcppls **不再附带、也不再启动 clangd**，C++ 语义全部来自 libmc++。
- mcppls **自身由 `mcxx` 构建**。

| # | 验收项 |
|---|---|
| AS.1 | mcppls 的引擎配置里没有 clangd；payload 中不包含 clangd |
| AS.2 | mcppls 全部 conformance fixture 通过（包括 `self-mcpp`、`real-xlings`、`self-mcppls` 这几个真实工程） |
| AS.3 | 满足 mcppls 现有的发布门槛：冷启动首次跳转中位数 < 12 s，热启动 < 5 s，这两项在三个平台上都要满足；并且 self-mcpp 冷启动首次跳转 ≤ 10 s（A2.2） |
| AS.4 | mcppls 的 CI 和发布构建使用 `mcxx` 工具链（自举） |
| AS.5 | payload 体积不大于当前（当前主要由 clangd 23.1 和 kit 构成） |

MS 可以在 M2 之后达成，**不依赖 M3**：这时 C++ 语义可以由进程内的 Clang 后端提供，自研前端负责模块和声明部分。

### M3 自研前端覆盖 `mc++.safe`

| # | 验收项 |
|---|---|
| A3.1 | 表达式层（F3）：`mc++.safe` 允许的特性全部由自研前端实现；在 mcppls 语料上，"不确定"的比例 ≤ 5% |
| A3.2 | 门禁优先在自研前端上运行，和 Clang 后端的结果差分一致率 ≥ 99.5% |
| A3.3 | 生命周期分析 v0（过程内分析 + `lifetimebound` 类注解）在 fixture 上的精确率和召回率被公布 |
| A3.4 | GPU 区域插件原型：`[[gpu::kernel]]` 所在区域套用 `gpu.device` profile，抽取出内核后交给 `rules-cuda` 或 `rules-sycl` 编译，并运行通过 |

### M4 独立编译器（届时再细化）

- MC++ 方言的模块由自研前端生成代码（LLVM IR 或 bitcode，不链接 libLLVM），classic 模块仍交给 Clang，两者混合构建。
- 届时依据库控制插件的**实测数据**（MC++ 代码实际用到了 `std` 的哪些部分），决定 `std` 与 `std2` 的取舍。

## 7. mcppls 这一侧的前置工作（不在本仓库，但影响 MS）

1. **修复参数漂移**（8.7 节）：模型缓存和 producer 两条路径必须给出逐字节相同的参数，并加入"热启动 `Built module` 数为 0"的回归检查。这件事能立刻省下 55–78 s，而且是 A2.3 的前提。
2. 引擎抽象里新增 `mcxx` 引擎（和 clangd 并列），为 MS 过渡做准备。
3. 临时 PR #29（计时插桩）保留，作为 A2.2 和 AS.3 的测量工具原型。

## 8. 风险与对策

| 风险 | 对策 |
|---|---|
| 为 4 个平台构建 Clang 开发库很耗时（LLVM 完整构建约 1 小时以上） | 做成预编译的 mcpp 包，CI 只拉取不构建；只在升级版本时重新构建 |
| Clang API 随版本变化 | P1 把影响限制在 `backend.clang`；固定版本；升级有单独的检查清单 |
| MSA 设计过度膨胀 | **按需扩展**：只有当某个使用方（mcppls、mcpp-safe、插件）真正需要时才增加查询，并且先写进 MC3 |
| 插件 C ABI 的性能开销 | 批量查询、按区域回调、句柄缓存；在基准测试中给出预算 |
| IFC 规范是草案 | 锁定版本；MC2 独立版本化 |
| 自研前端迟迟追不上（轨 1 太好用，轨 2 就没人推） | 每个里程碑都有差分门槛；A2.2、AS 这类**只有自研前端才能达到的指标**作为推动力 |
| Windows MSVC ABI | 当前不做；先支持 `x86_64-windows-gnu` |

## 9. 近期的具体行动（M0 起步）

1. 仓库：`Sunrisepeak/mcpp-safe`（私有）已创建并与本地关联。
2. 调研并落实 `llvm-x-clang-dev@23.1`：在 openkal 上构建 Clang 库的配方，先在 linux-x64 上完成。
3. 搭建 workspace 骨架（第 4 节的目录）和 devtools 的分层检查。
4. 起草 MC1、MC3 子集、MC4 v1、MC5 v0 规范。
5. `mc++.safe` v0 的特性清单和 fixture（复用 GCC 插件探针中的用例）。

## 10. 需要 review 决定的问题

1. **命名**：MC++ / `mcxx` / libmc++ / 模块前缀 `mcxx.` / 仓库名暂用 `mcpp-safe`（将来是否改为 `mcxx`）。
2. **libmc++ 的包粒度**：一个 `mcxx` 包按 feature 选组件（和 `mcpp:plugins` 相同的做法），还是每个组件一个包？
3. **配置写在哪里**：先写在 `mcpp.toml` 的 `[package.metadata.mcxx]` 下（不需要改 mcpp），还是推动 mcpp 支持一级的 `[language]` 或 `[features]` 表？
4. **mcppls 的接入方式**：先用 `mcxx serve` 子进程方式（本方案的建议），还是直接进程内链接？
5. **平台顺序**：先 linux-x64，然后是 macOS arm64、linux arm64、windows-gnu？
6. **里程碑 MS 的验收项**（AS.1–AS.5）是否符合你对"mcppls 完全基于 MC++"的定义？
