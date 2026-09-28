# MC++ 架构方案（v0，草案）

日期：2026-09-28
状态：待 review（第三轮：记录已定事项；初期生态协作规则；本仓库自建 `index/` 和 `xpkgs/`；`mcpp-tools-safe` 作为本仓库中的独立包）
里程碑与验收标准：[`2026-09-28-mcxx-milestones-acceptance.md`](2026-09-28-mcxx-milestones-acceptance.md)（包括生态适配项 E-… 与依赖关系）
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
| **libmc++** | 可复用组件的总称 | 由 `modules/` 下的多个 mcpp 包组成，**按依赖集合拆分**（见 4.1 节），发布到 mcpp-index |
| `mcxx.*` | C++ 模块名前缀 | 例如 `mcxx.core`、`mcxx.msa` |
| `mc++.<name>` | profile 名 | 例如 `mc++.safe`、`gpu.device` |
| 仓库 | `Sunrisepeak/mcpp-safe`（私有） | 名字保持不变（第三轮已定） |

## 2. 目标与非目标

**目标**

1. **Modules**：只接受模块代码。`import` 通过模块图和 IFC 解析。
2. **特性和库的细粒度控制**：用特性注册表加 profile，可以作用到包、模块、命名空间、声明和区域。
3. **Safe 机制**：限制类特性，加上扩展类语义（生命周期注解、分析），在编译时强制执行。
4. **组件化**：每个组件都是 `modules/` 下的一个 mcpp 包，可以单独复用，组合起来就是 libmc++。
5. **插件化**：编译器本身就是一个插件系统，提供规则、库控制、属性、区域/方言、分析、后端、输出七类扩展点。GPU 等能力以插件形式接入，不和核心耦合。
6. **全部走 mcpp 体系，基于 openkal 开发**：构建、依赖（包括 LLVM/Clang 开发库）、分发（插件就是 mcpp 包）都用 mcpp；运行基座是 openkal（和 mcppls 一样，从一台 Linux 主机交叉构建 4 个平台）。

**非目标（当前阶段）**

- 向前兼容：头文件单元、PCH、C 模式、旧标准模式、GNU 扩展、ObjC++。
- 自有标准库 `std2`：M4 之前一律使用标准 `std`（第二轮 review 决定）。
- 自研代码生成：M4 之前由 Clang CodeGen 负责。
- MSVC ABI：Windows 先支持 `x86_64-windows-gnu`，和 mcppls 一致。


## 3. 架构原则

| # | 原则 | 如何保证 |
|---|---|---|
| P1 | **只有 `mcxx-backend-clang` 依赖 Clang/LLVM，只有 `mcxx-platform` import openkal**，其余组件只能看到 MSA 和 base、core 提供的接口 | 分层检查 `mcxx-devtools check layers`（仿照 mcppls），在 CI 中强制执行 |
| P2 | **按依赖集合拆包**（沿用 mcppls 的做法："Split by DEPENDENCY SET, not by taste"）。不需要 Clang 的使用方，不会链接进任何 Clang 的代码 | 用 `nm` 检查示例程序（A0.2.3） |
| P3 | **会跨越边界的数据都要写成规范**：特性注册表、IFC 方言信息、MSA、插件协议、驱动契约、服务协议 | 第 5 节的 MC1–MC6，规则带 id，并有可追溯的一致性测试 |
| P4 | **强制执行由真实编译完成**；辅助答案只有三种：违规、通过、不确定。"不确定"转给 Clang 后端处理 | MSA 的查询结果带 `certainty` 字段 |
| P5 | **插件有两种方式，但扩展点的语义只有一套**：①**静态组合**（主路径）：插件是 mcpp 包，和 libmc++ 用同一个 openkal 工具链链接成本项目的 mcxx（由 `mcxx compose` 生成临时 workspace 交给 mcpp 构建，不改 mcpp），直接使用 C++ 模块 API；②**进程外协议**：其他工具链或其他语言写的插件，通过版本化协议在子进程中运行。**不依赖 `dlopen`**，因为 openkal 构建的是静态可执行文件（mcppls 实测是 "statically linked"），能否 `dlopen` 由 V0.5 确认 | MC4；A0.6.1–A0.6.2 |
| P6 | **差分测试是常规手段**：自研前端和进程内的 Clang 后端对比同一批事实 | `mcxx-devtools diff`；各阶段的一致率门槛 |
| P7 | 编译器和服务进程**不访问网络** | 沿用 mcppls 的原则 |
| P8 | 缺少的能力靠插件补充，而不是往核心里加特判 | 核心代码里不允许出现 `if (cuda)` 这种写法 |
| P9 | 平台差异只通过 `modules/os/*` 中的常量表达，不使用 `#ifdef` | 沿用 mcppls 的做法；分层检查 |
| P10 | **同一个单元无论从哪条路径进入，交给后端的参数都逐字节相同** | MC5；A0.5.2（吸取 mcpp-language-server#30 的教训） |
| P11 | **MC++ 本身就是一个插件系统**（第五轮）：<br>- 内置功能也是 SDK 上的 provider（`mc++.iso`），不走特殊通道；<br>- 插件可以控制（任意类别的特性）、扩展（源码过滤器）、覆盖（替换一个特性、整个 provider 或一个 profile）；<br>- 同一个 id 有两个 provider 而都不声明替换时算冲突，要报告，不能悄悄换掉 | `modules/features/tests/test_override.cpp`；`mcxx features` 列出全部 provider、特性、profile 和冲突 |
| P12 | **内置的是 ISO C++ 特性的明确控制，而且只做减法**（第五轮）：<br>- 每一项都用标准的 stable name 标明，去掉之后仍然是 ISO C++；<br>- MC++ 专有的东西只能来自插件，而且分类：policy、library、pitfall、extension；<br>- 只有 extension 会让代码离不开 MC++，profile `portable` 按类别整体禁止它们 | MC1 的类别；`Feature::standard` |
| P13 | **可控、可扩展、可覆盖，但性能不能丢**（第五轮）：<br>- 注册解析一次（Catalog），配置解析一次（Plan，有缓存）；<br>- 规则只被问到不是 `allow` 的特性；<br>- 只收集被问到的特性需要的事实种类（`fact::Kinds`）；<br>- 文件里不可能出现的特性跳过（`requires_declaration`）；<br>- 什么都不用问时不遍历 AST | `MCXX_LOG=gates=debug` 给出每一段的耗时；实测记在 `development.md` |

## 4. 仓库和 workspace 结构（mcpp workspace，基于 openkal）

```
mcpp-safe/                              workspace 根；根包就是 mcxx 驱动
├── mcpp.toml                           [workspace] members；[indices]（本仓库 index/）
├── src/                                mcxx 驱动（MC5）：只做命令分发，不包含 Clang 头文件
├── modules/                            ── libmc++：每个目录都是一个独立的 mcpp 包，按依赖集合拆分 ──
│   ├── base/            mcxx.base            错误、文本、路径、sha256、日志、trace、TOML
│   ├── os/{linux,macos,windows}, arch/{x86_64,aarch64}   平台常量包，通过 [target.'cfg(...)'.dependencies] 选择
│   ├── testing/         mcxx.testing         最小的具名模块测试框架
│   ├── msa/             mcxx.msa             MC++ 语义 API 与事实（MC3）
│   ├── graph/           mcxx.graph           模块声明的词法扫描、模块图（方案里原名 modules/）
│   ├── plugin/          插件核心：sdk/（mcxx.plugin，MC4）；以后还有 host/（mcxx compose、进程外宿主）、remote/
│   ├── features/        mcxx.features        特性门禁（MC1）：配置、作用域、级别、豁免、审计
│   ├── lsp/             mcxx.lsp             面向编辑器的 LSP 形态服务（MC6；方案里原名 service/）
│   └── backend/         后端。**Clang 只能出现在这里的两个 clang* 包里**
│       ├── semantic/        mcxx.backend               语义门面（mcppls 链接它）
│       ├── compiler/        mcxx.backend.compiler      编译门面（mcxx 驱动链接它）
│       ├── clang/           mcxx.backend.clang         基于 Clang 23.1 的 MSA、事实、Clang 内的插件机制
│       └── clang-compiler/  mcxx.backend.clang.compiler  进程内的 clang（driver、cc1、代码生成）
├── plugins/                            插件实现，和插件核心分开；一个目录是一个包，可放多个插件模块
│   ├── std/             mcxx.plugins.policy（mc++.policy：raw-pointers、lib:std.vector）、mcxx.plugins.cfg（[[mcpp::cfg]]，扩展 ext:cfg）
│   └── libs/            mcxx.plugins.json（nlohmann::json 的 json-brace-init）
├── index/                              本仓库自建的 mcpp 包索引：llvm.*（clang-dev、codegen-dev、clang-driver）、microsoft.*（gsl、ifc-sdk）
├── xpkgs/                              （计划）本仓库自建的 xlings xpkg：mcxx 工具链包
├── specs/                              MC1、MC3、MC4、MC5 的正文、schema、示例（MC2、MC6 在 M1）
├── conformance/                        门禁 fixture（gates/）与规范条目的可追溯性（traceability.json）
├── tools/               probe/（服务的开发驱动）、checks/lint.py（clang-exposure、json-brace-init）；以后还有 devtools/
├── forks/                              被忽略：speak-agent 下各 fork 的本地检出
└── .agents/docs/
```

和第一版方案相比的调整（第四轮，按实现情况）：
- **后端集中到 `modules/backend/`**：两个门面加两个 Clang 实现。语义和编译分成两个包，因为 mcpp 会把一个包的全部目标文件链接进使用方，mcppls 不应该带上代码生成器。mcppls 在进程内链接语义门面，不再通过 `mcxx serve` 子进程；那是早期的设想，实测进程内更简单也更快。
- **插件核心和实现分开**：核心在 `modules/plugin/`、`modules/features/`，实现在 `plugins/`；一个实现目录按主题放多个插件模块。
- **Clang 边界靠工具强制**：`tools/checks/lint.py` 的 `clang-exposure` 规则，CI 中执行。
- `platform/`、`core/` 暂时没有拆出：目前没有这部分需求，mcxx.base 已经够用。

第五轮调整（2026-09-29，原则 P11–P13）：
- **mc++.safe 不再是插件，而是内置的 profile。** 原先插件里的 7 个 ISO 特性移入核心（`modules/features/src/iso.cppm`，provider `mc++.iso`），并补齐到 15 个，每个都标明 stable name。
- **profile `safe` 专门针对编译器不检查的未定义行为来源**，共 10 项：
  - 指针运算、new/delete；
  - `reinterpret_cast`、C 风格转换、`const_cast`；
  - 联合体、C 数组、C 可变参数；
  - 未初始化的局部变量、asm。

  `goto` 和宏不是未定义行为来源，移到 `strict`。`modules` 要求所有依赖都通过 import。`portable` 禁止一切扩展。
- **插件只提供 ISO 以外的东西。** `plugins/std` 的 `mc++.policy` 提供 `raw-pointers`（"能不能用指针"这一编译器配置的例子）和 `lib:std.vector`；`[[mcpp::cfg]]` 以 `ext:cfg` 的名义报告每一处使用。
- **SDK 开放全部类别，并支持覆盖**：`Feature::replaces`、`Provider::replaces()`、`Profile::replaces`。
- **性能相关的机制**：`Catalog`、`Plan`、`Context::wants`、`Feature::needs`（`fact::Kinds`，其中 `declaration_types` 单独一类）、`Feature::requires_declaration`。

### 4.1 包和依赖集合

| 包 | 依赖 | 谁会用到 |
|---|---|---|
| `mcxx-base`、`mcxx-os-*` | 无 | 所有组件 |
| `mcxx-platform` | openkal-llvm-runtime | 需要和操作系统打交道的组件 |
| `mcxx-core`、`mcxx-features`、`mcxx-msa` | base、platform | mcppls（轻量接入）、插件、驱动 |
| `mcxx-modules` | 上一行的包 + IFC SDK | 驱动、服务、自研前端 |
| **`mcxx-backend-clang`** | 上面这些 + **`llvm.clang-dev`**（体积大） | 只有驱动和服务会用到。**mcppls 通过 `mcxx serve` 子进程使用它，自己的二进制里不链接 Clang** |
| `mcxx-frontend`、`mcxx-analysis` | core、msa、modules | 驱动、服务、mcppls（进程内使用语法层） |
| `mcxx-plugin`（SDK） | base、msa 的接口 | 插件作者 |
| `mcxx-plugin-remote` | base（不含 openkal） | 进程外插件（任何工具链） |

### 4.2 外部依赖（全部通过 mcpp 引入，基于 openkal）

| 依赖 | 版本 | 现状（本机实测） | 方案（生态适配项见里程碑文档第 1 节） |
|---|---|---|---|
| openkal-llvm-runtime | 与 mcppls 相同的版本线（当前 0.15.x） | 已在官方 mcpp-index 中 | 直接使用（只使用，不改动） |
| **LLVM/Clang 开发库** | **23.1.x**（与 mcppls 的 clangd 23.1.0 对齐） | **缺失**：mcpp-index 只有 `llvm.libcxx` 和 `llvm.compiler-rt-builtins`；xim-pkgindex 的 `llvm` 22.1.8 只有工具链；`libllvm` 20.1.7 只有 `libLLVM.so`；`llvm-dev` 20.1.7.1 是用 gcc/glibc 构建、给 mesa 用的，ABI 不适用 | **E-IDX-1**：在**本仓库 `index/`** 中新增 `llvm.clang-dev@23.1.x`，**用 openkal 工具链构建**。过渡方案：E-PLG-2（用官方 `deps-cmake` 从源码构建）或 E-XIM-2（本仓库 `xpkgs/` 中的 glibc 形式，仅 linux-x64） |
| IFC SDK（`microsoft/ifc`） | 锁定一个 tag | 不在任何索引中 | **E-IDX-2**（本仓库 `index/`） |
| nlohmann.json、cmdline | 与 mcppls 相同 | 已在 mcpp-index 中 | 直接使用 |

ABI 不稳定的代价（已接受）：升级 Clang 版本 = 重新构建 `llvm.clang-dev` + 只修改 `mcxx-backend-clang`。

### 4.3 libmc++ 的复用方式

| 使用方 | 用哪些组件 | 方式 |
|---|---|---|
| mcpp（构建） | `mcxx` 驱动 | 作为工具链（E-MCPP-1，先验证不改 mcpp 能否做到：V0.6）；对 GCC 等其他工具链，由本仓库的规则包 `mcpp-tools-safe` 以 blocking check 的方式运行 `mcxx check`（E-PLG-1） |
| mcppls | 进程内：`core`、`features`、`msa`、`modules`、`frontend`（语法层和声明层）；子进程：`mcxx serve`（Clang 后端） | 轻量部分放在进程内，快速响应；重量部分放在子进程里隔离，和 mcppls 现有的引擎抽象一致 |
| 第三方工具 | 按依赖集合选择包 | 初期从本仓库的 `index/` 引入 |
| 插件作者 | `mcxx-plugin` 或 `mcxx-plugin-remote` | 以 mcpp 包的形式分发 |

## 5. 规范（MC1–MC6，仿照 mcppls 的 S1–S5：独立版本号、规则 id、可追溯性）

| 规范 | 内容 | 首个版本出现在 |
|---|---|---|
| **MC1 特性注册表与 profile** | 特性 id 命名规则；类别（第五轮：`iso`（内置，带 ISO stable name，只做减法）/ `policy` / `library` / `pitfall` / `extension`，全部对插件开放）；内置 profile（`safe`：未定义行为来源；`modules`；`strict`；`portable`：禁止扩展），多个 profile 同时使用时取最严格的级别；检查层（语法 / 声明 / 表达式 / 控制流）；级别（allow / warn / deny）；作用域（包 / 模块 / 命名空间 / 声明 / 区域）；profile 文件格式；配置写在 `mcpp.toml` 的 `[package.metadata.mcxx]` 中（不改 mcpp，第三轮已定）；逃生口 `[[mcpp::allow("id")]]` 与审计输出；库控制（`lib:<模块或包>.<符号>`） | M0 |
| **MC2 IFC 方言信息** | 锁定的 IFC 版本；`VendorExtension` 的编码约定（模块的特性集、profile、跨方言边界标注、GCC/Clang 特有构造的编码） | M1 |
| **MC3 MSA** | 实体和句柄；查询集（按阶段逐步增加）；`certainty` 语义；版本化 | M0（只含声明层和表达式层的查询） |
| **MC4 插件** | **静态组合**（mcpp 包、注册方式、按插件集合的哈希缓存）和**进程外协议**（消息格式、协议号、版本协商）；两种方式共用同一套扩展点语义；故障隔离 | M0（v0：规则、源码过滤器、profile 三类扩展点，以及对特性、provider、profile 的覆盖）；M1 起加入属性和区域 |
| **MC5 驱动与工具链契约** | `mcxx` 命令行（兼容 clang 的一个参数子集）；mcpp 如何调用它；输出物（对象文件、`.ifc`、`.pcm`、facts、SARIF）；和 `mcpp emit build-database` 的衔接；**参数规范化**（P10） | M0 |
| **MC6 服务协议** | `mcxx serve`：LSP 子集，加上事实和门禁查询的扩展；和 mcppls S3/S5 对齐 | M1 |

## 6. 里程碑与验收标准

已拆成独立文档：**[`2026-09-28-mcxx-milestones-acceptance.md`](2026-09-28-mcxx-milestones-acceptance.md)**，内容包括：

- 验证项 V0.1–V0.5（openkal 上构建 Clang 开发库、静态链接的体积和耗时、IFC SDK、CodeGen 产物的链接、`dlopen`）；
- M0（8 个子阶段）、M1（9 个）、M2（3 个）、**★ MS：mcppls 完全基于 MC++**（4 个）、M3、M4，每个验收项都有编号、通过条件和验证方式；
- **生态适配项 E-…**：openkal、mcpp-index、xim-pkgindex、xlings、mcpp、mcpp-plugins、mcppls 在每个阶段要完成的事，以及它们和验收项之间的依赖关系。

## 7. 风险与对策

| 风险 | 对策 |
|---|---|
| openkal 上构建 Clang 开发库存在缺口（V0.1） | E-OK-1 由 openkal 补齐；过渡方案是 E-PLG-2 或 E-XIM-2 |
| 为 4 个平台构建 Clang 开发库很耗时 | 做成预编译的 mcpp-index 包（E-IDX-1、E-IDX-3），CI 只拉取；只在升级版本时重新构建 |
| 静态组合插件时，每个项目都要重新链接 mcxx，链接耗时长（V0.2） | 按插件集合的哈希缓存；必要时预链接出单体的 libmcxx 对象，或者把进程外协议作为主路径 |
| Clang API 随版本变化 | P1 把影响限制在 `mcxx-backend-clang`；固定版本；升级有单独的检查清单 |
| MSA 设计过度膨胀 | 按需扩展：只有当某个使用方真正需要时才增加查询，并且先写进 MC3 |
| IFC 规范是草案 | 锁定版本；MC2 独立版本化 |
| 自研前端迟迟追不上 | 每个里程碑都有差分门槛；A2.3.2、MS 这类只有自研前端才能达到的指标作为推动力 |
| 生态项目的节奏对不上 | 适配项有编号，并被验收项引用；关键路径（E-OK-1 → E-IDX-1）最先启动 |

## 8. 初期的生态协作规则（第三轮决定）

详见里程碑文档 1.0 节，要点如下：

1. **不对任何工具的官方仓库**（openkal、mcpp、mcpp-index、xim-pkgindex、xlings、mcpp-plugins、mcpp-language-server）提 issue、提 PR 或做任何改动。
2. 包优先放在本仓库：mcpp 包放在 `index/`，xlings 包放在 `xpkgs/`。
3. 必须修改某个工具时，在 **speak-agent** 下 fork，在分支上联调，用 fork 的 CI 验证；fork 及其改动登记在 `.agents/docs/forks.md`。
4. 是否合回上游、什么时候合，初期结束后（最早在 MS 之后）再统一决定。
5. 日常操作使用 speak-agent 账号；只有你明确要求时，才对单条命令使用 Sunrisepeak 的凭据。

## 9. 近期的具体行动

1. **验证接入方式**：V0.7（本仓库 `index/` 通过 `[indices]` 接入；`xpkgs/` 以本地索引仓库的方式接入）、V0.6（不改 mcpp 能否把 mcxx 当作工具链）。
2. **关键路径**：E-OK-1 → E-IDX-1（在 openkal 上构建 `llvm.clang-dev@23.1`，放进本仓库的 `index/`）；E-IDX-2（IFC SDK）。
3. **本仓库**：搭建 workspace 骨架（第 4 节，包括 `index/`、`xpkgs/`、`plugins/`）和分层检查；起草 MC1、MC3 子集、MC4 v0、MC5 v0。
4. 做 V0.1–V0.5 的实测，结论写进 `.agents/docs/`。
5. 进入 M1 之前：把 mcpp-language-server fork 到 speak-agent（E-LS-*），并建立 `.agents/docs/forks.md`。

## 10. 已定事项（第三轮）

| # | 事项 | 决定 |
|---|---|---|
| 1 | 命名 | MC++ / `mcxx` / libmc++ / 模块前缀 `mcxx.`；仓库名保持 `mcpp-safe` |
| 2 | 配置位置 | 先写在 `[package.metadata.mcxx]` 下，**不改 mcpp** |
| 3 | `tools-safe` 放在哪里 | 作为独立包 `plugins/mcpp-tools-safe` 放在本仓库 |
| 4 | 平台顺序 | linux-x64 → darwin-arm64 → linux-arm64 → windows-gnu |
| 5 | MS 的定义 | 按里程碑文档 AS.1–AS.4（初期在 speak-agent 的 mcppls fork 上达成） |
| 6 | 生态协作 | 第 8 节的规则 |

待决事项：官方 mcppls 上已有的草稿 PR #29 和 issue #30 如何处理（见里程碑文档"附二"）。

### 已定事项（第五轮，2026-09-29）

| # | 事项 | 决定 |
|---|---|---|
| 7 | 内置和插件的边界 | 内置的是 ISO C++ 特性控制（只做减法，带 stable name）；MC++ 专有的只能来自插件，并按 policy、library、pitfall、extension 分类 |
| 8 | 插件能力 | MC++ 本身就是插件系统：插件可以控制、扩展，也可以覆盖内置的实现；冲突要报告 |
| 9 | safe 的含义 | profile `safe` 针对未定义行为来源；风格类（goto、宏）归入 `strict` |
| 10 | 性能 | 默认配置下，门禁对不相关的代码没有开销（不遍历 AST）；开销和包要求检查的内容成正比 |
