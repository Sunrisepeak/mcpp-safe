# MC++ 里程碑与验收标准（v0，草案）

日期：2026-09-28
状态：待 review（2026-09-29 修订：M0.7 按架构方案第五轮的 P11–P13 调整，即内置的是 ISO 特性控制，MC++ 本身是插件系统）
配套文档：[`2026-09-28-mcxx-architecture-plan.md`](2026-09-28-mcxx-architecture-plan.md)（架构、组件、规范）。本文只讲**做到什么程度算完成、怎么验证**。

## 0. 约定

### 0.1 编号

- 里程碑：`M0`、`M1`、`M2`、`MS`（★ mcppls 完全基于 MC++）、`M3`、`M4`。
- 子阶段：`M0.3` 这样的格式，每个子阶段有明确的交付物。
- 验收项：`A0.3.2` = M0.3 的第 2 个验收项。**只有全部验收项通过，才算这个子阶段完成**。
- 验证项：`V0.1` 这样的格式，表示**需要先实测才能确定可行的事项**。验证项失败，要回头修订架构方案，不能跳过。

### 0.2 每个验收项都要写清楚

| 字段 | 含义 |
|---|---|
| 条件 | 可以用机器判断的通过条件，门槛值写明 |
| 验证方式 | 由哪个命令、哪个 fixture、哪个 CI job 产出证据 |
| 平台 | 默认只在 linux-x64 上验收；需要多平台的会特别标出 |
| 证据 | 保存在哪里（CI artifact、`conformance/` 的结果、`.agents/docs/` 中的记录） |

### 0.3 测量环境与基线

- 性能类指标默认在 GitHub `ubuntu-24.04` 4 核 runner 上测量，和已有实测的环境相同。
- 同一指标每次测 3 轮，取中位数。

| 基线（已实测） | 数值 | 来源 |
|---|---|---|
| self-mcpp 冷启动首次跳转（clangd 路径） | 87.8–126.0 s；需要 prime 174 个模块，准备窗口 158.7–228 s | mcpp-language-server#29 的 run 36415909922 和 36419266307 |
| self-mcpp 热启动首次跳转 | 55.5–78.5 s，**clangd 重建了全部 176 个 BMI**（问题记录在 mcpp-language-server#30） | 同上 |
| real-xlings 冷启动 / 热启动首次跳转 | 12.3 s / 6.7 s | run 36415909922 |
| self-mcppls 冷启动 / 热启动首次跳转 | 15.1 s / 9.4 s | 同上 |
| mcppls 的发布门槛 | 冷启动首次跳转中位数 < 12 s，热启动 < 5 s（三个平台） | mcppls `release-checks.yml` |
| 语料：mcppls | 249 个文件，2 个全局模块片段，4 处 `#include`，0 个 `#define`，0 个 `#if` | 本机统计 |
| 语料：mcpp | 204 个文件，66 个全局模块片段，17 个 `#define`，115 个 `#if*` | 本机统计 |

### 0.4 语料

| 代号 | 内容 | 用在 |
|---|---|---|
| C-mcppls | mcpp-language-server 固定在某个提交（纯模块代码） | M0 起 |
| C-mcpp | mcpp-community/mcpp 固定在某个提交（带全局模块片段） | M1 起 |
| C-xlings | openxlings/xlings 固定在某个提交 | M2 起 |
| C-fixtures | 本仓库 `conformance/fixtures/`，每个特性、每条规则的正例和反例 | M0 起 |


## 1. 生态项目与适配项（E 编号）

### 1.0 初期的生态协作规则（第三轮 review 决定，适用到另行通知为止）

| # | 规则 |
|---|---|
| R1 | 初期**不对任何工具的官方仓库**（openkal、mcpp、mcpp-index、xim-pkgindex、xlings、mcpp-plugins、mcpp-language-server）提 issue、提 PR 或做任何改动 |
| R2 | 包**优先放在本仓库内**：mcpp 包放在 `index/`（mcpp index 格式，通过 `[indices]` 的 `path` 方式接入）；xlings 包放在 `xpkgs/`（独立的 xpkg 文件，以本地索引仓库的方式接入） |
| R3 | 必须修改某个工具时，在 **speak-agent** 账号下 fork，在分支上联调，用 fork 自己的 CI 验证；**不向上游提 PR** |
| R4 | 每个 fork 和它的改动都登记在 `.agents/docs/forks.md`：fork 地址、分支、改动内容、原因、将来合回上游的条件 |
| R5 | 是否合回上游、什么时候合，等初期结束后统一决定（最早在 MS 之后，由你拍板） |
| R6 | 日常操作使用 speak-agent 账号；只有你明确要求时，才对单条命令使用 Sunrisepeak 的凭据 |

在这些规则之前已经存在的两处例外：mcpp-language-server 的草稿 PR #29 和 issue #30，都是之前按你的要求在官方 mcppls 仓库上创建的（见文末"附二：待决事项"）。

### 1.1 涉及的仓库，以及它们在初期的落点

| 仓库 | 角色 | 初期的落点 |
|---|---|---|
| **openkal** | LLVM runtime：自带 libc、内核抽象层，是 mcppls 和 MC++ 的运行基座 | 只使用，不改动；如果有缺口，先在本仓库的构建配方中绕开，实在绕不开再到 speak-agent 的 fork 里修 |
| **mcpp-index** | mcpp 的官方库索引 | **不改动**。本仓库的 `index/` 承担它的角色 |
| **xim-pkgindex** | xlings 的官方软件和工具链索引 | **不改动**。本仓库的 `xpkgs/` 承担它的角色 |
| **xlings** | 安装器和 subos | 只使用；验证本地索引仓库的接入方式 |
| **mcpp** | 构建引擎 | **先不改 mcpp**（第三轮决定 2）。能力缺口先用现有机制绕开（见 V0.6、E-MCPP-*）；必须改的，只在 speak-agent 的 fork 里调试 |
| **mcpp-plugins** | 第一方构建插件 | 只使用（例如 `deps-cmake`、`rules-cuda`）；`tools-safe` **作为独立包放在本仓库**（第三轮决定 3） |
| **mcpp-language-server** | libmc++ 的第一个大使用方 | 所有集成工作都在 **speak-agent 的 fork** 里进行 |
| **microsoft/ifc** | IFC 规范和 SDK | 只读依赖，锁定一个 tag，打包进本仓库的 `index/` |

### 1.2 补充的验证项（和 V0.1–V0.5 一起做）

| # | 要验证的事 | 通过条件 | 失败时怎么办 |
|---|---|---|---|
| V0.6 | **不改 mcpp**，能否让 mcxx 作为工具链接入 mcpp：例如由本地 xpkg 提供一个 llvm 家族兼容的 payload（mcxx 作为兼容 clang 的驱动），或者通过现有的 toolchain 配置指向它 | `mcpp build` 能用 mcxx 编译 hello-modules | 在 speak-agent 的 mcpp fork 中加入最小改动用于调试（按 R3、R4 登记）；不向上游提交 |
| V0.7 | 本仓库的 `index/` 通过 `[indices] <名字> = { path = "index" }` 被 mcpp 使用；`xpkgs/` 以本地索引仓库的方式被 xlings 使用（项目级 `.xlings.json` 的 `index_repos`，或 `$MCPP_HOME/config.toml` 中的 `[index.repos.<名字>]`） | 两者都能解析、安装到本仓库定义的包 | xlings 走不通时，退而只用 mcpp 的 `index/`（mcxx 以 mcpp 包的形式构建和分发） |

### 1.3 适配项清单

| 编号 | 落点 | 内容 | 需要在哪个阶段完成 | 验收条件 | 被这些验收项依赖 |
|---|---|---|---|---|---|
| **E-OK-1** | 本仓库的构建配方（必要时 speak-agent/openkal fork） | 构建 LLVM/Clang 23.1 开发库时遇到的 openkal 缺口被绕开或修复 | M0（V0.1） | 最小的 libTooling 程序能在 openkal 上链接并运行 | V0.1、A0.1.1 |
| **E-OK-2** | 本仓库 `.agents/docs/` | 静态进程能否 `dlopen` 的书面结论 | M0（V0.5） | 有实测记录 | V0.5、A0.6.x |
| **E-IDX-1** | 本仓库 `index/` | 包 `llvm.clang-dev@23.1.x`：用 openkal 工具链构建的 Clang/LLVM 静态库和头文件（先 linux-x64） | M0 | 通过 `[indices]` 能解析；本仓库 CI 构建通过 | A0.1.1 |
| **E-IDX-2** | 本仓库 `index/` | IFC SDK 包，锁定 tag | M0 | 同上 | A0.1.2、V0.3 |
| **E-IDX-3** | 本仓库 `index/` | `llvm.clang-dev` 补齐其余平台 | MS 之前 | 各平台 CI 通过 | AS.3.x |
| **E-IDX-4** | 本仓库 `index/` | libmc++ 各组件包（按依赖集合拆分）和插件 SDK | M1 | fork 中的 mcppls 能通过 `[indices]` 引入并构建 | A1.4.x、A1.5.1 |
| **E-IDX-5** | 本仓库 `index/` | 插件包（`mcxx-plugins-std`、`mcxx-plugins-libs`），以及 `mcpp-tools-safe` 规则包 | M1 | 同上 | A1.5.x |
| **E-IDX-6** | 本仓库 `index/` | GPU 区域插件原型包 | M3 | 同上 | A3.4 |
| **E-XIM-1** | 本仓库 `xpkgs/` | `mcxx` 工具链的 xpkg 文件（先 linux-x64） | M0 | V0.7 通过；`xlings install mcxx` 能用 | A0.5.3、A1.5.x |
| **E-XIM-2** | 本仓库 `xpkgs/` | （仅当 V0.1 走退路时）glibc 形式的 `llvm-dev@23.1` xpkg 文件 | M0（有条件） | 同上 | V0.1 的退路 |
| **E-XIM-3** | 本仓库 `xpkgs/` | `mcxx` 补齐其余平台 | MS 之前 | 各平台 CI 通过 | AS.4.x |
| **E-XL-1** | 验证即可 | 通过本地索引仓库执行 `xlings install mcxx`、`xlings use mcxx` | M1 | 冒烟脚本 | A1.5.x |
| **E-MCPP-1** | 不改 mcpp（V0.6）；退路是 speak-agent/mcpp fork | mcxx 作为工具链接入 mcpp | M0 | `mcpp build` 用 mcxx 编译 hello-modules 和 C-mcppls | A0.5.3 |
| **E-MCPP-2** | 本仓库（不改 mcpp） | **按项目静态组合插件**：由 `mcxx compose` 生成一个临时 workspace（libmc++ + 项目声明的插件包），交给 mcpp 构建出本项目的 mcxx，并按插件集合的哈希缓存 | M0 | 插件集合不变时不重新链接 | A0.6.1 |
| **E-MCPP-3** | 本仓库（不改 mcpp） | 方言和 profile 信息：mcxx 和 mcppls 直接读取各包 `mcpp.toml` 中的 `[package.metadata.mcxx]`（mcpp 对这张表原样保留，不做解释；`build.mcpp` 也能通过 `graph_file()` 读到它） | M1 | fork 中的 mcppls 能拿到各模块的方言信息 | A1.4.2、A1.2.x |
| ~~E-MCPP-4~~ | — | 已取消：配置先放在 `[package.metadata.mcxx]`，不改 mcpp（第三轮决定 2） | — | — | — |
| **E-PLG-1** | 本仓库 `plugins/mcpp-tools-safe/`（独立包） | 对非 mcxx 工具链，以 blocking check action 运行 `mcxx check`，保持增量；参考 mcpp-index 中 `clangtidy` 规则包的写法 | M1 | 包内 `tests/` 中的消费者工程在本仓库 CI 中通过 | A1.5.1–A1.5.3 |
| **E-PLG-2** | 只使用官方 mcpp-plugins 的 `deps-cmake` | （过渡用）从源码构建 `llvm.clang-dev` | M0（有条件） | 构建出的库和 E-IDX-1 的产物等价 | V0.1 |
| **E-PLG-3** | 使用官方 `rules-cuda`、`rules-sycl`；需要改动时用 speak-agent/mcpp-plugins fork | 接收 GPU 区域插件抽取出来的内核编译单元 | M3 | 本仓库 GPU 插件的消费者工程通过 | A3.4 |
| **E-LS-1** | speak-agent/mcpp-language-server fork | 修复参数漂移（即官方 #30 描述的问题），并加入"热启动 `Built module` = 0"的回归检查 | M1 之前 | fork 的 CI | A1.4.4、A2.3.4 |
| **E-LS-2** | 同上 | 新增 `mcxx` 引擎（和 clangd 并列，默认关闭） | M1 | fork 的 CI | A1.4.1–A1.4.3 |
| **E-LS-3** | 同上 | 用 `mcxx.frontend` 的语法层替换 native 引擎 | M1 | fork 的 CI | A1.8.x |
| **E-LS-4** | 同上 | 跳转、悬停、补全的声明部分切换到 libmc++ | M2 | fork 的性能 job | A2.3.x |
| **E-LS-5** | 同上 | 默认引擎改为 mcxx；去掉 clangd；CI 和发布构建改用 mcxx | MS | fork 的 release-checks | AS.x |
| **E-LS-6** | 同上 | 把 #29 的计时插桩整理成可选的测量功能（在 fork 中） | M2 之前 | fork 的性能 job 可以复用 | A2.3.2 |

### 1.4 各阶段的生态前置条件（汇总）

| 阶段 | 必须先完成的适配项 | 同期要完成的适配项 | 需要的 fork |
|---|---|---|---|
| M0 | E-OK-1、E-OK-2、E-IDX-1、E-IDX-2、V0.6、V0.7（有条件时换成 E-PLG-2 或 E-XIM-2） | E-MCPP-1、E-MCPP-2、E-XIM-1 | 只在 V0.6 失败时才需要 speak-agent/mcpp |
| M1 | E-LS-1、E-XIM-1、E-IDX-4、E-IDX-5 | E-MCPP-3、E-PLG-1、E-XL-1、E-LS-2、E-LS-3 | speak-agent/mcpp-language-server |
| M2 | E-LS-6 | E-LS-4 | 同上 |
| MS | E-IDX-3、E-XIM-3 | E-LS-5 | 同上 |
| M3 | — | E-IDX-6、E-PLG-3 | 视情况 speak-agent/mcpp-plugins |

---

## M0 基座：Clang 内核 + 特性门禁 + 工具链

**目标**：`mcxx` v0 能作为 mcpp 的工具链编译 mcppls；`mc++.safe` v0（内置 profile `safe`）在编译时强制执行，MC++ 的内置控制和插件走同一个 SDK。

### 验证项（先做，决定 M0 的做法）

| # | 要验证的事 | 通过条件 | 失败时怎么办 |
|---|---|---|---|
| V0.1 | 能在 openkal 工具链上构建 LLVM/Clang 23.1 的开发库（静态库 + 头文件） | 在 linux-x64 上产出 clangAST、Sema、Frontend、Serialization、Analysis、CodeGen、Lex、Basic，以及 LLVM Support、Core 等库；一个最小的 libTooling 程序能在 openkal 上链接并运行 | 找出 openkal 缺少的能力，向 openkal 提需求；过渡期允许 mcxx 用 glibc 动态构建（仅 linux-x64） |
| V0.2 | mcxx 静态链接 Clang 后的体积和链接耗时 | 记录这两个数；按项目静态组合插件时，链接耗时 ≤ 60 s（4 核，可按组合结果缓存） | 链接耗时超标时，改为预链接的 `libmcxx` 单体对象，或者把进程外协议作为主路径 |
| V0.3 | IFC SDK（`microsoft/ifc`）能在 openkal 上用 mcpp 构建 | `ifc-printer` 能构建出来并正常运行 | 自己实现 IFC 读写的子集（规模小，只覆盖 T1 层） |
| V0.4 | 在 mcxx 中调用 Clang CodeGen 产出的目标文件，能和 mcpp 其余部分正常链接 | 用 mcxx 编译、再由 mcpp 链接的 hello-modules 能运行 | 退一步：mcxx 只做前端检查，代码生成仍调用 clang 二进制 |
| V0.5 | openkal 上的静态 mcxx 能否 `dlopen` | 记录结论（预期：不能）。结论决定 MC4 是否需要动态加载这种方式 | 能的话，把动态加载作为可选方式写进 MC4 |

### M0.1 依赖与构建

| # | 条件 | 验证方式 |
|---|---|---|
| A0.1.1 | 本仓库 `index/` 中有 `llvm.clang-dev@23.1.x`（openkal 构建，linux-x64），并在 workspace 中锁定（依赖 **E-IDX-1**、E-OK-1） | `mcpp build` 能解析到它；`mcpp.lock` 中有记录 |
| A0.1.2 | IFC SDK 以 mcpp 包的形式可用，并锁定版本（依赖 **E-IDX-2**） | 同上 |
| A0.1.3 | 缓存命中时，CI 上的冷构建 ≤ 15 分钟 | CI job 耗时 |

### M0.2 workspace 骨架与分层

| # | 条件 | 验证方式 |
|---|---|---|
| A0.2.1 | `modules/` 下每个组件都是一个独立的 mcpp 包，按**依赖集合**拆分（见架构方案 4.1 节），根包是 `mcxx` 驱动 | `mcpp build --workspace` 通过 |
| A0.2.2 | **只有 `mcxx-backend-clang` 依赖 Clang/LLVM，只有 `mcxx-platform` import openkal** | `mcxx-devtools check layers` 通过，并在 CI 中强制执行 |
| A0.2.3 | 不需要 Clang 的使用方（例如只依赖 features、msa、modules 的程序）构建出来的产物里**不包含任何 Clang 符号** | 用 `nm` 检查示例程序 |
| A0.2.4 | 平台差异只通过 `modules/os/*` 中的常量表达，不使用 `#ifdef` | 分层检查中包含这条规则 |

### M0.3 特性注册表与 profile（MC1 v0）

| # | 条件 | 验证方式 |
|---|---|---|
| A0.3.1 | MC1 v0 草案发布：特性 id、类别、检查层、三档级别、五种作用域、profile 格式、配置位置、逃生口、审计输出 | schema 校验和示例校验通过 |
| A0.3.2 | 从 `mcpp.toml` 读取的配置和规范示例逐项相等 | 单元测试 |
| A0.3.3 | 作用域优先级（声明 > 命名空间 > 模块 > 包 > profile）按规范生效 | 每个组合至少有 1 个 fixture |
| A0.3.4 | 每次使用逃生口 `[[mcpp::allow("id")]]` 都会出现在审计输出中，而且不能静默放行（该特性必须允许被豁免） | fixture |

### M0.4 MSA v0 与 Clang 后端

| # | 条件 | 验证方式 |
|---|---|---|
| A0.4.1 | MC3 v0：只包含 T1（声明、类型、导出）和 T2（变量类型、表达式类别、cast 种类、new/delete、指针运算）所需的查询；每个查询都带 `certainty` | schema 校验和示例校验 |
| A0.4.2 | `mcxx-backend-clang` 实现 MC3 v0 的全部查询 | MSA 一致性测试 100% 通过 |
| A0.4.3 | 在 C-mcppls 上，通过 MSA 导出的 T1 事实数量和 Clang AST 直接统计的数量一致 | `mcxx-devtools diff --corpus C-mcppls` |

### M0.5 驱动与 mcpp 工具链（MC5 v0）

| # | 条件 | 验证方式 |
|---|---|---|
| A0.5.1 | `mcxx check --syntax-only` 在 C-mcppls 上零错误；诊断集合和 clang 23.1 `-fsyntax-only` 的一致 | CI job `corpus-check` |
| A0.5.2 | **参数规范化**：同一个单元无论从 build database、缓存还是命令行进入，最终交给后端的参数**逐字节相同** | 专门的 fixture（吸取 mcpp-language-server#30 的教训） |
| A0.5.3 | 以 `mcxx` 作为 mcpp 工具链构建 C-mcppls，产物通过 mcppls 自己的单元测试（`mcpp test`）（依赖 **E-MCPP-1**、E-XIM-1） | CI job `self-host-mcppls` |
| A0.5.4 | MC5 v0 草案：命令行子集、输出物、和 mcpp toolchain-model 的对接方式 | schema 校验 |

### M0.6 插件（MC4 v0）

| # | 条件 | 验证方式 |
|---|---|---|
| A0.6.1 | **静态组合**：插件是 mcpp 包，通过 `import mcxx.plugin;` 注册；`mcxx compose` 生成一个临时 workspace，由 mcpp 把项目声明的插件集合和 libmc++ 一起链接成本项目的 mcxx，并按插件集合的哈希缓存（依赖 **E-MCPP-2**；不改 mcpp） | 示例项目：插件集合不变时第二次构建不重新链接 |
| A0.6.2 | **进程外协议**：用其他工具链（GCC 16 + libstdc++）构建的插件，通过进程外协议完成同样的规则检查，结果和静态组合方式相同 | 同一批 fixture，两种方式输出同一个诊断集合 |
| A0.6.3 | 插件崩溃或超时不会拖垮编译：报告会写明是哪个插件、在哪个单元出的问题 | 故障注入 fixture |
| A0.6.4 | MC4 v0 草案：两种方式共用同一套扩展点语义（v0：规则、源码过滤器、profile，以及覆盖）、协议号、版本协商 | schema 校验 |

### M0.7 `mc++.safe` v0（内置 profile `safe`）

| # | 条件 | 验证方式 |
|---|---|---|
| A0.7.1 | 内置的 ISO 特性控制（`mc++.iso`），每项都带 stable name；profile `safe` 覆盖编译器不检查的未定义行为来源（至少包括指针运算、new/delete、`reinterpret_cast`、C 风格转换、`const_cast`、联合体、C 数组、C 可变参数、未初始化的局部变量、asm）；另有插件样例：policy `raw-pointers`、库控制 `lib:std.vector`、extension `ext:cfg` | `mcxx features --json`；特性清单写进 MC1 的示例 |
| A0.7.2 | 每个特性至少各有 5 个正例和反例 fixture，**精确率和召回率都是 100%** | `conformance run --suite safe` |
| A0.7.3 | 必须检出的用例：跨模块 `auto v = make()`，以及导出别名 `Buf`（和 GCC 插件探针的结果一致） | fixture `cross-module-deduction` |
| A0.7.4 | 内置控制和插件走同一个 SDK：`mc++.iso` 只 import `mcxx.plugin` 和 `mcxx.msa`，不接触后端；插件能替换其中任何一个特性，或者替换整个 provider；冲突要报告 | `modules/features/tests/test_override.cpp`；`clang-exposure` 分层检查 |
| A0.7.5 | 违规时编译失败，诊断包含特性 id、位置、豁免方法 | fixture |

### M0.8 工程基础设施

| # | 条件 | 验证方式 |
|---|---|---|
| A0.8.1 | `specs/` 下的 MC1、MC3、MC4、MC5 都有 schema、示例和规则 id；可追溯性文件覆盖全部规则 | `mcxx-devtools check specs` |
| A0.8.2 | `conformance/` 的 runner 能输出 JSON 格式的计时和结果（仿照 mcppls 的 `--measure`） | CI artifact |
| A0.8.3 | CI 至少包含这些 job：构建、分层检查、规范检查、一致性测试、语料检查、自举（self-host） | workflow 列表 |

**M0 的退出条件**：V0.1–V0.5 都有结论；A0.x 全部通过；只要求 linux-x64。

---

## M1 libmc++ 的首批使用方

**目标**：mcppls 和 mcpp 开始用上 libmc++；IFC 能正确写出；自研前端的语法层可用。

### M1.1 生成 IFC（MC2 v1）

| # | 条件 | 验证方式 |
|---|---|---|
| A1.1.1 | C-mcppls 中每个模块接口单元都产出 `.ifc`（T1 层） | 统计 |
| A1.1.2 | `ifc-printer` 能读取全部 `.ifc`，零错误 | CI |
| A1.1.3 | 把 `.ifc` 读回 MSA 后得到的 T1 事实，和写出前**逐项相等** | `mcxx-devtools ifc-roundtrip` |
| A1.1.4 | 方言信息（模块的特性集、profile）按 MC2 v1 写入 `VendorExtension`，读取方能还原 | fixture |
| A1.1.5 | MC2 v1 草案：锁定的 IFC 版本号、厂商扩展的编码表 | schema 校验 |

### M1.2 跨模块的方言边界

| # | 条件 | 验证方式 |
|---|---|---|
| A1.2.1 | `mc++.safe` 模块导入一个接口里暴露了受限特性（例如裸指针）的模块时，必须显式标注，否则报错 | fixture |
| A1.2.2 | 这个判断**只依赖对方的 `.ifc`**，不读对方的源码 | fixture 里删掉对方源码后结果不变 |

### M1.3 服务与 `mcxx serve`（MC6 v1）

| # | 条件 | 验证方式 |
|---|---|---|
| A1.3.1 | `mcxx serve` 实现 MC6 v1：诊断、门禁查询、T1 事实查询 | 协议一致性测试 |
| A1.3.2 | 进程级隔离：`mcxx serve` 崩溃后能被重启，不会拖垮宿主 | 故障注入 |

### M1.4 接入 mcppls（在 speak-agent 的 mcppls fork 中完成）

| # | 条件 | 验证方式 |
|---|---|---|
| A1.4.1 | mcppls 增加 `mcxx` 引擎，和 clangd 并列，默认关闭，通过设置开启（**E-LS-2**；方言信息依赖 E-MCPP-3） | mcppls CI |
| A1.4.2 | **编辑器和构建的门禁结果一致**：对同一批 fixture，mcppls（经由 mcxx 引擎）和 `mcxx check` 给出的门禁诊断集合**完全相同** | 对照测试 |
| A1.4.3 | 开启 mcxx 引擎后，mcppls 现有的 conformance fixture 全部照旧通过 | mcppls CI |
| A1.4.4 | 前置条件：参数漂移（官方 #30 描述的问题）已在 fork 中修复，并且"热启动 `Built module` = 0"的回归检查已在 fork 的 CI 中生效（**E-LS-1**） | 引用 mcppls 的 CI 结果 |

### M1.5 对非 mcxx 工具链的强制执行

| # | 条件 | 验证方式 |
|---|---|---|
| A1.5.1 | 规则包 `mcpp-tools-safe`（本仓库中的独立包）：对使用 GCC 的工程，以 blocking check action 的形式运行 `mcxx check`（依赖 **E-PLG-1**、E-IDX-5、E-XIM-1、E-XL-1） | 示例工程 |
| A1.5.2 | C-mcpp 用 GCC 构建时，注入一处违规会让构建失败，去掉后构建通过 | CI |
| A1.5.3 | check action 是增量的：源码不变时不会重新运行 | 第二次构建的日志 |

### M1.6 自研前端 F1：词法与预处理

| # | 条件 | 验证方式 |
|---|---|---|
| A1.6.1 | 在 C-mcppls 和 C-mcpp 上，token 序列和 Clang 的一致率 100%（忽略注释） | `mcxx-devtools diff --layer lex` |
| A1.6.2 | 预处理只覆盖语料中实际用到的指令（`#include` 只允许出现在全局模块片段里、条件编译），**宏受 `macros` 特性开关控制** | fixture |
| A1.6.3 | 不依赖 Clang/LLVM（分层检查） | `check layers` |

### M1.7 自研前端 F1：语法树

| # | 条件 | 验证方式 |
|---|---|---|
| A1.7.1 | C-mcppls 和 C-mcpp **零解析失败** | 语料统计 |
| A1.7.2 | 声明范围（名字和整体范围）和 Clang 的差分一致率 ≥ 99.9% | `diff --layer syntax` |
| A1.7.3 | 错误恢复：对语料逐个文件做 1000 次随机截断或插入，不崩溃，并且仍能产出语法树 | 模糊测试 job |
| A1.7.4 | 解析速度：C-mcppls 整个语料冷启动解析 ≤ 2 s（4 核） | 计时 |

### M1.8 替换 mcppls 的 native 引擎

| # | 条件 | 验证方式 |
|---|---|---|
| A1.8.1 | mcppls 的大纲、文档符号、工作区符号、模块特性，改由 `mcxx.frontend` 的语法层提供（**E-LS-3**） | mcppls CI |
| A1.8.2 | mcppls 的 conformance fixture 全部照旧通过 | mcppls CI |
| A1.8.3 | 语法级门禁（`goto`、`new`、写出的 `*` 声明符、`reinterpret_cast`、宏）在编辑器中 ≤ 100 ms 给出结果 | 计时 |

### M1.9 插件扩展点：属性和区域

| # | 条件 | 验证方式 |
|---|---|---|
| A1.9.1 | 属性插件：认领 `[[ns::attr]]` 并拿到对应的 MSA 子树 | 示例插件和 fixture |
| A1.9.2 | 区域插件：给一个区域套用自己的 profile，门禁引擎在区域范围内生效 | 示例插件和 fixture |
| A1.9.3 | MC4 v1：7 类扩展点中已完成 4 类（规则、库控制、属性、区域） | schema 校验 |

**M1 的退出条件**：A1.x 全部通过；A1.4 和 A1.8 在 mcppls 的三个平台上通过；其余只要求 linux-x64。

---

## M2 语义服务不再依赖 BMI 重建

**目标**：跨模块的声明级语义由自研前端加上 IFC 提供，冷启动不需要构建 BMI。

### M2.1 名字查找与声明

| # | 条件 | 验证方式 |
|---|---|---|
| A2.1.1 | 名字查找（限定、非限定，以及受 `adl` 特性开关控制的 ADL）、using、命名空间、类成员：T1 事实和 Clang 后端的差分一致率 ≥ 99% | `diff --layer decl --corpus C-mcppls,C-mcpp` |
| A2.1.2 | 类型解析（别名、cv 限定、引用、函数类型、类模板特化的**声明**） | 同上 |

### M2.2 通过 IFC 处理 import

| # | 条件 | 验证方式 |
|---|---|---|
| A2.2.1 | 自研前端处理 `import M;` 时只读 `M.ifc`，不读 M 的源码，也不构建 BMI | fixture：删掉 M 的源码，查询结果不变 |
| A2.2.2 | `import std;` 通过 IFC 解析（`std` 的 `.ifc` 由 M1.1 生成），声明层查询的正确率 ≥ 99%（和 Clang 后端对比） | diff |
| A2.2.3 | 需要实例化函数体的查询（例如推导返回类型）返回"不确定"，并转给 Clang 后端 | fixture |

### M2.3 服务切换到自研前端

| # | 条件 | 验证方式 |
|---|---|---|
| A2.3.1 | mcppls 中跳转、悬停、补全的**声明部分**由 libmc++ 回答（**E-LS-4**；测量依赖 E-LS-6） | mcppls CI |
| A2.3.2 | **self-mcpp 冷启动首次跳转 ≤ 10 s**（基线 87.8–126 s） | 性能 job，3 轮取中位数 |
| A2.3.3 | real-xlings、self-mcppls 冷启动首次跳转不劣于基线（12.3 s、15.1 s） | 同上 |
| A2.3.4 | 热启动不重建任何东西：日志中重建类事件数 = 0 | 回归检查 |
| A2.3.5 | "不确定"比例在三套语料上被统计并公布 | 报告 |

**M2 的退出条件**：A2.x 全部通过；A2.3.2–A2.3.4 在三个平台上通过。

---

## ★ MS：mcppls 完全基于 MC++

**定义**（初期在 speak-agent 的 mcppls fork 上达成和验收）：mcppls **既不附带也不启动 clangd**，C++ 语义全部来自 libmc++（M2 阶段由自研前端加进程内的 Clang 后端共同提供）；并且 **mcppls 本身由 `mcxx` 构建**。MS 不依赖 M3。

### MS.1 引擎切换

| # | 条件 | 验证方式 |
|---|---|---|
| AS.1.1 | mcppls 的默认引擎是 mcxx；clangd 引擎代码保留一个版本，作为退路 | mcppls 配置 |
| AS.1.2 | 原本由 clangd 提供的 LSP 功能（跳转、声明、引用、悬停、补全、诊断、语义高亮、文档符号、签名帮助）全部由 mcxx 引擎提供 | mcppls 能力对照表，每项至少一个 fixture |

### MS.2 去掉 clangd

| # | 条件 | 验证方式 |
|---|---|---|
| AS.2.1 | payload 中不再包含 clangd（**E-LS-5**） | `payload --verify` 的文件清单 |
| AS.2.2 | payload 体积不大于切换前 | 体积对比 |

### MS.3 质量门槛

| # | 条件 | 验证方式 |
|---|---|---|
| AS.3.1 | mcppls 全部 conformance fixture 通过，包括 `self-mcpp`、`real-xlings`、`self-mcppls` | mcppls CI（三个平台） |
| AS.3.2 | 满足 mcppls 的发布门槛：冷启动首次跳转中位数 < 12 s，热启动 < 5 s（三个平台） | mcppls `release-checks` |
| AS.3.3 | self-mcpp 冷启动首次跳转 ≤ 10 s（沿用 A2.3.2） | 性能 job |
| AS.3.4 | 稳定性：mcppls 的 stability 检查连续 3 轮通过 | mcppls `release-checks` |

### MS.4 自举

| # | 条件 | 验证方式 |
|---|---|---|
| AS.4.1 | mcppls 的 CI 和发布构建使用 `mcxx` 工具链（依赖 **E-XIM-3**、E-IDX-3） | mcppls workflow |
| AS.4.2 | 用 mcxx 构建出的 mcppls 通过自己的全部测试和 conformance | mcppls CI |

**MS 的退出条件**：AS.x 全部通过，并且是在三个平台上（linux-x64、darwin-arm64、win32-x64；linux-arm64 按 mcppls 当时的平台表）。

---

## M3 自研前端覆盖 `mc++.safe`（先定方向，细则在 M2 结束时补充）

| # | 条件（初值） |
|---|---|
| A3.1 | F3 表达式层：`mc++.safe` 允许的特性全部由自研前端实现；C-mcppls 上"不确定"比例 ≤ 5% |
| A3.2 | 门禁优先在自研前端上运行，和 Clang 后端的差分一致率 ≥ 99.5% |
| A3.3 | 生命周期分析 v0（过程内分析 + `lifetimebound` 类注解）：在 fixture 上公布精确率和召回率，精确率 ≥ 95% |
| A3.4 | GPU 区域插件原型：`[[gpu::kernel]]` 区域套用 `gpu.device` profile，抽取后交给 `rules-cuda` 或 `rules-sycl` 编译，能运行（依赖 **E-PLG-3**、E-IDX-6） |
| A3.5 | 库控制插件统计出 C-mcppls、C-mcpp 对 `std` 的实际使用分布（为 M4 的 `std` 决策提供数据） |

## M4 独立编译器（先定方向）

| # | 条件（初值） |
|---|---|
| A4.1 | MC++ 方言的模块由自研前端生成代码（LLVM IR 或 bitcode，不链接 libLLVM），能和 classic 模块（Clang 编译）混合链接 |
| A4.2 | 依据 A3.5 的数据，出一份 `std` / `std2` 决策记录 |
| A4.3 | 至少有一个真实工程，其 MC++ 方言模块完全不经过 Clang 编译，并通过测试 |

---

## 附：依赖关系（包含生态适配项）

```
                ┌──────── 生态（初期：本仓库 index/、xpkgs/，或 speak-agent 的 fork）────────┐   ┌─ 本仓库 ─┐
openkal（只使用）  E-OK-1 ─┬─> E-IDX-1 (index/: llvm.clang-dev) ─┬─> V0.1 ─> M0.1
                  E-OK-2 ─┼──────────────────────────────────────┼─> V0.5 ─> M0.6
mcpp-plugins（只使用） E-PLG-2 ┘ (E-IDX-1 就绪前的过渡)           │
                  E-IDX-2 (index/: IFC SDK) ───────────────────────┴─> V0.3 ─> M0.1
                  V0.7 (本地 index 和 xpkgs 能被接入) ─┬─> E-XIM-1 (xpkgs/: mcxx) ─┐
mcpp（不改）       V0.6 ─> E-MCPP-1 ───────────────────┴─────────────────────────────┴─> M0.5
                  E-MCPP-2 (mcxx compose，本仓库) ─────────────────────────────────────> M0.6

M0 ──┬─ M1.1 ── M1.2 <── E-MCPP-3 ([package.metadata.mcxx]，本仓库)
     ├─ M1.3 ── M1.4 <── E-LS-1、E-LS-2 (speak-agent/mcppls fork)、E-IDX-4
     ├─ M1.5 <── E-PLG-1 (plugins/mcpp-tools-safe)、E-IDX-5、E-XL-1
     ├─ M1.6 ── M1.7 ── M1.8 <── E-LS-3 (fork)
     └─ M1.9
M1 ── M2.1 ── M2.2 ── M2.3 <── E-LS-4、E-LS-6 (fork)
                        ├─ MS <── E-LS-5 (fork)、E-IDX-3、E-XIM-3
                        └─ M3 <── E-PLG-3、E-IDX-6 ── M4
```

### 生态适配的推进顺序（按关键路径）

1. **现在就可以开始**：V0.7（本地 index 和 xpkgs 的接入方式）、V0.6（不改 mcpp 能否接入 mcxx）、E-OK-1 与 E-IDX-1（关键路径的起点）、E-IDX-2。
2. **M0 期间**：E-MCPP-1、E-MCPP-2、E-XIM-1。
3. **M1 期间**：fork mcppls 到 speak-agent；E-LS-1、E-LS-2、E-LS-3；E-PLG-1、E-MCPP-3、E-IDX-4、E-IDX-5、E-XL-1。
4. **M2 至 MS 期间**：E-LS-4、E-LS-6、E-IDX-3、E-XIM-3、E-LS-5。

## 附二：待决事项

1. 官方 mcppls 仓库上已有的草稿 PR #29（计时插桩）和 issue #30（参数漂移），是在 R1 之前按你的要求创建的。按 R1，建议：保留原样、不再更新；后续的计时和修复工作都转到 speak-agent 的 fork 中进行。或者由你决定关闭它们。
