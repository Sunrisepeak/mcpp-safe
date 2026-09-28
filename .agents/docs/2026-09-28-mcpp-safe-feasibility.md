# mcpp-safe 可行性调研：只做 AST 够不够？和 mcppls 能不能复用？

日期：2026-09-28
输入：`README.md` 的问题；只读仓库 mcpp、mcpp-plugins、mcpp-language-server（mcppls）

## 0. 问题拆解

README 里的原问题实际包含三件事：

1. 想做的东西：
   - 自己的 C++ 语言服务器，尤其是 module 支持；
   - 一个 mcpp 构建插件，在编译期做强制检查：禁用某些语言特性或类型（例如禁止指针、禁止 `vector`），做部分生命周期检查，检查代码风格；
   - 规则由项目配置，并且真正拦住构建，而不是只写在文档里。
2. 核心疑问：是不是**只要实现 AST** 就够了？
3. 复用疑问：mcppls 和 mcpp-safe 两个项目能不能复用？

## 1. 结论先行

> **修订（review 后）**：第 2 行"最难的部分"和第 3 节"Clang 与 GCC 看到的代码不一致"的判断已被推翻。
> 规则可以作为 GCC/Clang 插件直接跑在真实编译中（已实测），不需要给 Clang 重建 BMI；通用抽象层放在语义模型层，
> 它的按模块序列化文件（UBMI）同时供 mcppls 使用。详见 [`2026-09-28-universal-bmi-analysis.md`](2026-09-28-universal-bmi-analysis.md)。

| 问题 | 结论 |
|---|---|
| 只做 AST 够吗 | **不够，而且 AST 不应该自己实现。** 需要的是一个带完整语义（Sema）的 AST，只能来自一个真正的 C++ 前端，也就是 Clang。只要复用 Clang，AST 本身就是现成的。真正要做的是另外四层：①给模块工程拿到"正确的 Clang AST"（构建模型 + 参数归一化 + clang 自己的 BMI + `std`）；②在 AST 之上做分析（匹配器只能处理禁用类规则，生命周期要 CFG/数据流）；③规则策略（配置、作用域、豁免、基线）；④在构建里强制执行 |
| 最难的部分在哪 | 不在 AST，而在第①层：**一个用 GCC/MSVC 构建的 C++ 模块工程，怎么让 Clang 正确解析**。mcppls 花了大部分工程量解决的正是这个问题 |
| 能复用吗 | **能，但复用点不在 AST。** mcppls 自己的引擎是纯词法的，没有 AST；它的 C++ 语义全部来自外部 clangd 进程。可复用的是它的"构建模型 → 归一化 → 语义套件 → 模块预构建"这条链路，以及 S1 构建数据库规范。规则引擎应作为独立的共享核心，由 mcpp（强制）和 mcppls（编辑器提示）两边同时使用 |
| 强制执行挂在哪 | mcpp 已经有现成机制：`mcpp::action` 设 `role = check`、`blocking = true`，包内所有编译边都会等它跑完；它不通过，构建就失败。文档里给的示例正是 clang-tidy，占位符 `${mcpp.compile_db}` 也已提供。**mcpp 引擎本身不需要改就能挂上检查** |

## 2. 三个仓库里和本问题相关的事实

### 2.1 mcppls：没有自己的前端，AST 来自 clangd

- 设计文档原话："It has no compiler front end of its own and never builds the project."（`mcpp-language-server/.agents/docs/design.md` §1）
- 两个引擎（`src/engine/engine.cppm`）：
  - **native 引擎**：`src/engine/native/*`，自研词法器加一个小型递归下降解析，**不做预处理、不建 AST**。只回答模块层面的问题：模块名跳转、`import` 补全、模块依赖图、未解析/有歧义/跨模块分区的诊断，以及导出声明的抽取（`exports.cpp`，给 review 的语义 diff 用）。
  - **clangd 引擎**：`src/engine/clangd.cpp`（约 3700 行），驱动一个固定版本（23.1）、裁剪过的 clangd **外部进程**，C++ 语义都由它提供。
- 引擎抽象支持 `answer / fallback / merge` 三种角色，诊断由 workspace 合并后发布。**再挂一个"规则引擎"在结构上是被支持的。**
- mcppls 已经解决、且对 mcpp-safe 直接有用的部分：
  - **L1–L4 构建模型**：mcpp 工程通过离线 `mcpp emit build-database` 得到每个编译单元的角色、参数和 `std`；也支持 CMake、单独的 `compile_commands.json`、只有源码的情况。
  - **归一化**（`src/normalize/{gnu,msvc,plan}.cpp`）：把 GCC/MSVC 的参数转成 Clang 系引擎能接受的参数。
  - **语义套件 S4**：打包好的 libc++ 头文件和 `std` 模块源码，机器上没有编译器也能 `import std`。
  - **模块预构建**：替 clangd 按依赖顺序构建 BMI；某个模块构建失败时用占位单元代替（`C2`）。
  - **外部程序 runner**：给每次运行独立的进程单元、设截止时间、限制读取量、留下记录。
- mcppls 的 review 层也有"规则"（`src/ai/review/rules.cpp`），但那是**针对变更的审查规则**，例如 `module/export-removed-in-use`、`build/toolchain-divergence`，不是语言特性约束，不能直接当作 mcpp-safe 的规则引擎用。

### 2.2 mcpp：强制检查的挂点已经存在

`mcpp/docs/30-build-mcpp.md` 把 `build.mcpp` 能做的工作分为三类，其中"Verification（验证）"就是为检查准备的：

| role | 行为 |
|---|---|
| `check` | 与编译并行运行，mcpp 写一个 stamp 文件；**`blocking = true` 时，所在包的所有编译边都会等它完成** |
| `source` / `object` / `artifact` / `prepare` | 生成源码、参与链接、链接后处理、准备前缀 |

- 动作的命令是 argv（不经过 shell），可用 `${mcpp.compile_db}` 拿到 `compile_commands.json`，原文注明"what clang-tidy's `-p` wants"。
- 动作有 input/output 指纹，**天然增量**：源码不变就不会重跑检查。
- `mcpp emit build-database` 与 `mcpp build --configure-only` 算出的是**同一个计划**：编辑器看到的和构建看到的是同一份事实。
- mcpp-plugins 的分族里，`mcpp.tools.<x>` 的定义是"what the build program does itself"，mcpp-safe 作为插件放在这一族（例如 `mcpp.tools.safe`）最自然。

### 2.3 mcpp-plugins：插件形态已经是范式

一个成员 = 一个模块文件 + `mcpp.toml` 中的一个 feature + `tests/` 下一个 CI 会构建的消费者工程。消费方在 `build.mcpp` 里 `import mcpp.tools.safe;`，配置并提交 action 即可。**分发、版本、引擎版本下限等问题这套机制都已经解决。**

## 3. 为什么"只做 AST"不够

### 3.1 AST 不能自己写

"禁止 `vector`""禁止指针"看起来是语法问题，实际都是**语义**问题：

- `auto v = make();`、`using Buf = std::vector<int>;`、`template<class C> void f(C&)`，这些写法在词法上都看不到 `vector`，要经过类型推导、别名展开、模板实例化才能知道。
- 指针也会隐式出现：数组退化、`this`、字符串字面量、lambda 捕获、标准库内部实现。要判断"用户**自己写的**指针"，就需要 Clang 的 `SourceManager` 区分文件和宏展开位置。
- 生命周期要知道一个引用指向哪个对象、这个对象什么时候析构，这需要重载决议、临时对象物化、隐式转换之后的完整语义。

自己实现到这个程度，等于重写一个 C++ 前端（模板、重载、constexpr、concepts、模块）。**唯一现实的路线是复用 Clang**：libTooling / AST Matchers / clang-tidy 检查框架 / Clang Analysis（CFG、数据流）。mcppls 的 native 词法器适合模块层面的快速回答，**不适合**承担 mcpp-safe 的规则判定。

### 3.2 有了 Clang AST 之后还要做的四层

```
① 构建模型   compile 参数 + 模块图 + clang 自己的 BMI + std       ← 最难，mcppls 已有
② 分析       AST Matcher（禁用类） / PPCallbacks（宏、include）
             / CFG + 数据流（生命周期） / token 级（风格）
③ 策略       规则集、作用域（只管本包、不管依赖和 std）、豁免、基线
④ 执行       mcpp blocking check（强制）/ mcppls 诊断（提示）/ CLI（CI）
```

**① 构建模型（最容易被低估）。** 模块工程下，Clang 要解析一个 TU，必须先有它 import 的每个模块的 **Clang BMI**。BMI 各编译器互不兼容：工程用 GCC 构建，GCC 的 `.gcm` 对 Clang 没用，检查器得自己按依赖顺序再构建一遍 Clang BMI。`compile_commands.json` 里的 GCC 参数（如 `-fmodules-ts`、`-fmodule-mapper`）Clang 不接受，需要归一化。`std` 从哪来，也要看是 libstdc++、libc++ 还是 MSVC STL。**这一层 mcppls 已经完整做过**（2.1 节），这是复用价值最大的地方。

**② 分析的难度差别很大。**

| 规则类别 | 例子 | 技术 | 难度 |
|---|---|---|---|
| 禁用类型/声明 | 禁止 `std::vector`、禁止 `new`/`delete`、禁止 `reinterpret_cast`、禁止异常 | AST Matcher | 低 |
| 禁用特性 | 禁止裸指针类型、指针算术、C 数组、`goto`、宏 | AST Matcher + SourceLocation 过滤 | 低到中 |
| 预处理 | 禁止 `#include`（强制模块化）、禁止宏定义 | PPCallbacks | 低 |
| 风格 | 命名、布局 | clang-format + readability 类检查 | 低（大多现成） |
| 生命周期（局部） | 返回局部变量的引用、悬垂 `string_view`/span、临时对象被引用 | CFG + 过程内数据流 + `[[clang::lifetimebound]]` | **高** |
| 生命周期（完整） | 类 Rust 借用检查 | 过程间分析 + 注解体系 | **非常高，不建议作为目标** |

**③ 策略层决定能不能真正落地。** 需要：规则可配置（放在 `mcpp.toml` 或独立配置里）；只检查本包源码，不检查依赖和 `std`；逐处豁免（例如 `[[mcpp::allow("raw-pointer")]]` 或注释）；存量代码用基线，只拦新增违规；诊断稳定且带规则 ID。

**④ 执行。** 强制执行放在构建里（mcpp blocking check），编辑器里只是同一套规则的提前提示。

### 3.3 "真正强制"的边界要先说清楚

- 检查器用 Clang 解析，工程可能用 GCC 编译：GCC 特有扩展、libstdc++ 与 libc++ 的差异可能导致"Clang 解析失败"或"两边看到的代码不同"。
- 只能看到当前配置下的代码：`#if` 另一分支、别的平台的代码、未实例化的模板体，都检查不到（或只能部分检查）。
- 因此"强制"的准确含义是：**在 mcpp 为该目标计算出的这份计划下强制**。多平台工程需要每个 target 各跑一次。

## 4. 复用方案

### 4.1 推荐的分层

```
                  ┌──────────── 共享：mcpp-safe core（库） ────────────┐
                  │  规则定义 · 规则配置 · Clang 分析（Matcher/CFG）     │
                  │  输入：S1 构建数据库   输出：带规则 ID 的诊断       │
                  └──────────────┬───────────────────┬─────────────────┘
                                 │                   │
      mcpp 插件 mcpp.tools.safe  │                   │  mcppls
      build.mcpp 提交 blocking   │                   │  新引擎（merge 诊断）
      check action → 失败即失败  │                   │  或链接进自建 clangd
          （强制）               │                   │  （编辑器提示）
                                 │                   │
                       CLI：mcpp-safe check（CI / Agent）
```

复用关系逐项对照：

| 组件 | 来源 | mcpp-safe 怎么用 |
|---|---|---|
| S1 构建数据库 / `mcpp emit build-database` | mcpp + mcppls 规范 | 直接作为输入 |
| 归一化（GCC/MSVC 参数 → Clang） | mcppls `src/normalize` | **复用或抽成库**，最关键 |
| 语义套件 S4（libc++、std 模块） | mcppls | 直接复用，解决"Clang 的 std 从哪来" |
| 模块预构建 / 占位单元 | mcppls clangd 引擎 | 复用思路；检查器也要按依赖顺序构建 Clang BMI |
| runner / 超时 / 事件记录 | mcppls | 可复用 |
| native 词法引擎 | mcppls | **不复用于规则判定**，最多做快速预筛 |
| review 规则 | mcppls `src/ai/review` | 不同性质，不复用 |
| check action / 增量指纹 / 插件分发 | mcpp、mcpp-plugins | 直接使用 |

### 4.2 mcppls 侧有两种接法

| 方案 | 做法 | 优点 | 缺点 |
|---|---|---|---|
| A. 链接进 clangd | mcpp-safe 规则写成 clang-tidy 模块，编进 mcppls 自己发布的那份 clangd（clangd 在进程内运行 clang-tidy 检查） | 复用 clangd 已经建好的 AST 和 BMI，零额外解析；编辑时实时出结果 | 必须自己构建 clangd（mcppls 目前是"裁剪并固定 hash 的 clangd 23.1"，要评估改成自建的代价）；规则升级跟着 payload 发版 |
| B. 独立引擎 | mcppls 新增一个 engine，调用 mcpp-safe CLI（或在进程内调用），诊断以 `merge` 角色合并 | 与 clangd 解耦，规则可以独立升级 | 同一个 TU 要解析两次；BMI 需要共享或重建 |

建议：**第一阶段选 B**（落地快，和 CLI 共用一条路径），规则稳定后再评估 A。

### 4.3 构建侧的优化

toolchain 本身是 Clang 时，可以把规则做成 Clang 插件（`-fplugin=`）挂在真实编译上，省掉第二次解析。toolchain 是 GCC/MSVC 时，走独立 check action。两条路径共用同一个 core。

## 5. 不写代码就能先做到的（值得先验证）

以下都是 Clang/clang-tidy 现成的能力，建议先组合起来，验证"强制执行"这条链路是否走得通，再决定哪些规则需要自研：

- `-Wunsafe-buffer-usage`（Clang Safe Buffers）：禁止指针算术和裸数组下标，基本就是"禁止裸指针运算"。
- clang-tidy 的 `cppcoreguidelines-pro-*`（类型转换、指针算术、union 等）、`cppcoreguidelines-owning-memory`、`cppcoreguidelines-no-malloc`、`modernize-*`、`readability-identifier-naming`（风格）。
- `-Wdangling*` 系列与 `[[clang::lifetimebound]]`：局部生命周期检查。较新版本的 Clang 还在做实验性的 lifetime safety 分析，**具体开关和版本需要按 mcppls 固定的 clangd 23.1 实测确认**。
- libc++ hardening（`_LIBCPP_HARDENING_MODE`）：运行期兜底，补上静态检查覆盖不到的部分。
- 类似"禁止某个类型"的规则可以写成 clang-query 的 matcher。较新的 clang-tidy 有基于 query 的自定义检查（实验特性），**是否可用需要实测确认**；可用的话，"禁止 `vector`"这类规则只需配置、不需要写 C++。

最小验证方式：在一个 mcpp 模块工程的 `build.mcpp` 里，提交一个 `role = check, blocking = true` 的 action，命令为 `clang-tidy -p ${mcpp.compile_db} ...`。重点看**模块工程下 clang-tidy 能不能正确拿到 BMI**。这一步卡住的地方，正好就是 mcpp-safe 必须自己解决（或从 mcppls 复用）的第①层。

## 6. 建议的路线

| 阶段 | 内容 | 产出 |
|---|---|---|
| P0 | 按第 5 节做最小验证：mcpp 模块工程 + blocking check + clang-tidy | 确认模块工程下 Clang 工具链路的实际缺口 |
| P1 | 把 mcppls 的归一化、kit、模块预构建抽成可共享的库（或 mcpp-safe 通过 mcppls CLI 拿到"可给 Clang 用的计划"） | "给任意 mcpp 工程拿到正确 Clang AST"的能力 |
| P2 | mcpp-safe core：规则配置格式、作用域、豁免、基线；先做禁用类规则（Matcher） | `mcpp-safe check` CLI |
| P3 | 插件 `mcpp.tools.safe`（mcpp-plugins 成员）：一个 feature，`build.mcpp` 中提交 blocking check | 构建期强制执行 |
| P4 | mcppls 接入（方案 B 引擎），编辑器显示同样的诊断 | 编辑器侧提示 |
| P5 | 生命周期：先做过程内 + `lifetimebound` 注解，再评估更深的分析 | 局部生命周期检查 |

## 7. 待决问题（需要你决定）

1. ~~工程是否限定为 Clang 工具链？~~ 已由通用 BMI 分析回答：GCC 与 Clang 都用插件方式，不需要限定。原文：**工程是否限定为 Clang 工具链？** 限定后，可以直接用 `-fplugin` 挂在真实编译上，不需要第二次解析，也没有"Clang 与 GCC 看到的代码不一致"的问题，复杂度大幅下降。不限定，就必须走 mcppls 那一套归一化和 Clang BMI 重建。
2. **生命周期检查的目标深度？** 局部检查（悬垂引用、返回局部引用）是可达的；类 Rust 借用检查不建议作为目标。
3. **规则配置放在哪？** `mcpp.toml` 的 `[package.metadata.safe]`（mcpp 不解释该表，插件自己读）还是独立文件。
4. **mcppls 接入方式？** 方案 A（编进 clangd）还是 B（独立引擎），这决定 mcppls 是否要改成自建 clangd。
5. **mcpp-safe 与 mcppls 的代码关系？** 是把 normalize/kit 抽成公共库两边依赖，还是 mcpp-safe 只调用 mcppls 的 CLI、把它当作黑盒"计划提供者"。
