# 自建 C++ 前端：先作辅助，为 mcpp 工具链做准备（讨论）

日期：2026-09-28
前置：[`2026-09-28-mcpp-safe-feasibility.md`](2026-09-28-mcpp-safe-feasibility.md)、[`2026-09-28-universal-bmi-analysis.md`](2026-09-28-universal-bmi-analysis.md)（包括第 8.6、8.7 节的实测）
议题：基于 LLVM、参考 Clang 和 Zig 的做法，自建一个 C++ 前端。先作为 mcppls 和 mcpp 构建检查的辅助，将来成为 mcpp 的工具链之一。

## 1. 规模基准（实测）

| 对象 | 行数 | 来源 |
|---|---|---|
| mcpp（src + modules） | 约 12.2 万 | 本机统计 |
| mcppls | 约 4.7 万 | 本机统计 |
| **GCC 16 的 C++ 前端**（`cp/` 31.6 万 + `c-family/` 5.6 万 + `libcpp/` 3.6 万） | **约 40.7 万**，不含代码生成 | 本机 GCC 16.1 源码 |
| 其中：`parser.cc` / `pt.cc`（模板）/ `decl.cc` / `call.cc`（重载）/ `constexpr.cc` / `module.cc` / `name-lookup.cc` | 5.8 万 / 3.4 万 / 2.1 万 / 1.5 万 / 1.3 万 / 2.4 万 / 1.0 万 | 同上 |

按"只要比 mcpp 复杂度低就不算难"的标准：**完整的 C++ 前端约为 mcpp 的 3.3 倍，超出了这条线**。所以"一次写全"不可取，可取的是下面这种分阶段、限定范围、每一步单独就能交付价值的做法。

## 2. 参考对象能告诉我们什么

| 参考 | 做法 | 对我们的启示 |
|---|---|---|
| **Zig 的 `zig cc` / `zig c++`** | 把 Clang/LLVM、libc 源码、libc++ 一起打包，提供开箱即用的密闭交叉工具链。**它的 C++ 前端就是 Clang，不是自己写的** | "mcpp 自己的工具链"和"自己写前端"是两件事。前者 mcpp 已经具备了大半：openkal-llvm-runtime、xlings、mcppls 本身就是在 openkal 上交叉构建的 |
| **Zig 的 Aro**（用 Zig 写的 C 前端） | 只对 C 这种范围可控的语言自己写前端，用来替代 Clang 的部分用途（translate-c） | 自研前端要选范围可控的子集；C++ 全集不属于这种范围 |
| **Zig 与 LLVM 解耦** | 社区有让主程序不再依赖 LLVM/Clang 库的提案；据我了解，Zig 的 LLVM 后端已经改为自己直接写出 bitcode，不再通过 libLLVM 的 IRBuilder（具体版本待核实） | 到了代码生成阶段，可以输出 LLVM IR 或 bitcode，交给 openkal 的 clang/llc 处理，**前端本身不链接 LLVM** |
| **Circle**（Sean Baxter） | 一个人从零写的 C++ 前端加 LLVM 后端，在上面实现了 Safe C++ 借用检查（P3390） | 证明"自有前端 + 安全子集 + 编译期强制"这条路走得通；代价是一个人投入多年，而且闭源 |
| **cppfront**（Herb Sutter） | 新语法先翻译成 C++，再交给现有编译器编译 | 另一种思路：不做完整前端，只做翻译层 |

## 3. 让范围可控的四个杠杆（关键）

1. **只接受模块输入，import 的内容通过 IFC 读取。** mcpp 以模块为主，`import std;` 是常态。前端遇到 import 时，直接读取真实编译器产出的 IFC：来自 GCC/Clang 插件（生成侧），或者 MSVC 原生产出。这样**不需要解析 libstdc++/libc++ 的头文件**，而 C++ 一致性问题最集中的地方恰恰是这些头文件（标准库的实现几乎用到了语言的每一个特性）。前端只需要处理用户代码。
2. **定位为辅助，允许"不知道"。** 硬性强制执行仍然由编译器插件完成（已实测可行）。自研前端的结论只有三种：违规、通过、不确定。遇到"不确定"就交给编译器插件处理。所以它不需要一开始就完全正确。
3. **只完整支持安全子集。** mcpp-safe 本来就是在限制 C++ 的特性。前端只需要完整理解 mcpp-safe 允许的那部分语言；超出子集的写法，要么本身就是违规，要么交给插件。"检查器定义它所强制的那门语言"，这和 Circle、Safe C++ 的思路一致。
4. **有现成的标准答案。** 对同一份代码，GCC/Clang 插件导出的语义事实（T1/T2 层）可以作为参考答案，和自研前端的输出做差分测试。正确性可以持续度量，不靠猜。

## 4. 它能提供而 clangd 和插件做不到的（为什么值得做）

- **mcppls**：不重建 BMI、也不依赖 clangd，直接给出跨模块的语义答案。实测中 mcpp 仓库的冷启动要构建 174 个 BMI，4 核机器上 1–4 分钟；另外还有热启动因参数漂移而全量重建的问题（8.7 节）。自研前端读取 IFC，可以把"首次可用"缩短到秒级，而且对编译器中立。
- **mcpp 构建**：在调用编译器之前先做一遍秒级的预检查（语法级和声明级规则），提前、快速地失败。
- **长期**：由我们自己掌握语言子集的语义，可以加入编译器没有的东西，例如被强制执行的生命周期注解、`[[mcpp::...]]` 语义属性。这是区别于 clangd 和插件的**真正差异点**，也是它将来成为工具链的理由。

## 5. 分阶段路线（每个阶段都能单独交付，并设门槛）

| 阶段 | 内容 | 规模估计（参照 GCC 对应部分） | 交付给谁 | 进入下一阶段的门槛 |
|---|---|---|---|---|
| **F0**（现在，不写前端） | 编译器插件做硬性强制；生成侧写出 IFC；修复 mcppls 的参数漂移（8.7 节） | 见前两份文档 | mcpp-safe、mcppls | IFC 生成侧可用 |
| **F1 语法层** | 词法分析 + 预处理（只需覆盖模块代码实际用到的部分）+ 完整的 C++ 语法树，带错误恢复，达到 IDE 级别 | 3 万–5 万（GCC 的 parser 5.8 万里夹杂着语义动作，单纯的语法解析会小一些） | mcppls native 引擎升级（大纲、符号、模块特性）；**语法级规则**：`goto`、`new` 表达式、声明里写出的 `*` 声明符、`reinterpret_cast`、宏定义 | 在 mcpp、xlings、mcppls 三个仓库上零解析失败 |
| **F2 声明层** | 名字查找、声明、类型解析；读取 IFC 形式的 import；类模板**只实例化声明**，不实例化函数体 | 3 万–5 万 | mcppls 在没有 clangd 的情况下提供跳转、悬停、补全；mcpp-safe 的 T1 接口规则 | 与插件导出的 T1 事实差分一致率达标（例如 99%） |
| **F3 表达式层** | 表达式类型、重载决议、`auto` 和模板实参推导、常量求值的子集；仅覆盖安全子集 | 5 万–9 万（参照 `call.cc` + `typeck.cc` + `pt.cc` + `semantics.cc` + `constexpr.cc` 的一部分） | mcpp-safe 的 T2 规则在编辑器和预检查中秒级运行 | 与插件导出的 T2 事实差分一致；把 `auto v = make()` 这类用例纳入回归 |
| **F4 代码生成**（工具链） | 为安全子集生成 LLVM IR 或 bitcode，交给 openkal 的 clang/llc；遵循 Itanium ABI（名字修饰、布局、虚表、异常、RTTI），可以和 clang 编译的代码互相链接 | 大，需要单独评估 | 为 mcpp 提供"一种工具链" | 见下方难点 |

F1 到 F3 合计约 11 万–19 万行，**和 mcpp 在同一个量级**，而且每一步单独就能交付价值。

**F4 的难点要事先说清楚**：用户代码里一旦写了 `std::vector<int>`，要为它生成代码，就得实例化标准库模板的**函数体**。这等于要求前端能处理 libc++/libstdc++ 的实现代码，也就回到了接近完整 C++ 的一致性要求，杠杆 1 在这里失效。所以 F4 是否启动，要看 F3 的实际一致性数据再决定，而不是现在就承诺。在此之前，"mcpp 自己的工具链"可以先按 Zig 的方式定义：**密闭打包的 Clang（openkal）+ 内置 mcpp-safe 插件 + 生成 IFC**。这件事现在就能做，而且对用户的价值立刻可见。

## 6. 技术选型建议

- 实现语言：C++23 模块，用 mcpp 构建（和 mcppls 一样自举，也是在给 mcpp 做实战验证）。
- 依赖：F1–F3 **不依赖 LLVM**，保持轻量，才能直接嵌入 mcppls 和 mcpp。F4 输出 IR 或 bitcode，不链接 libLLVM（参照 Zig 的解耦方向）。
- 数据模型：直接使用 IFC（第 8 节已决定）。自研前端既读 IFC（import），也写 IFC（自己编译的模块接口），和插件、MSVC 处在同一个生态里。
- "参考 Clang"的方式：参考它的分层（Lex → Parse → Sema → AST）和它的测试用例（例如 `clang/test` 中的 SemaCXX 语料可以作为一致性测试的来源），**不 fork Clang 的代码**；否则就又回到了"用 Clang"，没必要自己写。

## 7. 需要拍板的问题

1. 是否接受"辅助定位 + 允许不确定 + 硬性强制仍由编译器插件完成"这一前提？它是控制风险的核心。
2. 输入限定为"模块代码 + IFC 形式的 import"，全局模块片段里的 `#include` 怎么处理？（选项：规则直接禁止；或者交给插件处理。）
3. F1 的第一个交付物，是否就定为"替换 mcppls native 引擎的词法器，加上语法级规则"？
4. F4 放到 F3 的数据出来之后再决定，这期间"mcpp 工具链"先按 Zig 的方式做 Clang 打包，是否同意？

## 8. 修订：MC++（review 后）

review 给出的约束：

- 不考虑老的向前兼容，核心只有 **module** 和 **safe**；
- 能编译完全基于 C++ 模块的代码即可；
- 前期可以依赖 libclang，复用其中的功能；
- 核心特点是 **C++ 特性可以细粒度控制和配置**，不再只是 `-std=c++23` 这一个开关；
- 很早就能让 mcppls 和 mcpp-safe 用上，逐步稳定之后，自然成长为独立的 MC++ 编译器。

**结论：可行。** 做法是让 MC++ **从第一天起就是一个编译器（内部基于 Clang），然后在内部逐步替换成自研前端**。这是 Zig 的 stage1（C++ 实现）过渡到 stage2（自举）、以及 Aro 逐步接替 Clang 做 translate-c 时走过的路。

### 8.1 "只做纯模块代码"是现实的（实测）

| 代码库 | 源文件 | 全局模块片段 | 含 `#include` | `#define` | `#if*` |
|---|---|---|---|---|---|
| mcppls | 249 | 2 | 4 | **0** | **0** |
| mcpp | 204 | 66 | 66 | 17 | 115 |

- **mcppls 就是一个现成的纯模块语料**（约 4.7 万行），可以作为 MC++ 的第一套验收语料。
- mcpp 使用 `#include` 主要是为了 C API（`<cstdio>` 40 处、`<cstdlib>` 37 处，可以改用 `import std.compat`）和平台头文件（`unistd.h`、`windows.h`）。这正是边界层该处理的事：mcppls 的 `modules/platform` 就是唯一接触 openkal 的地方。

可以直接砍掉的老包袱：C 模式、头文件单元、PCH、宏（交给特性开关，默认禁止）、旧标准模式、K&R 语法、GNU 扩展、ObjC++、三字符组。
**砍不掉的核心**：模板、重载决议、名字查找（ADL 可以限制）、constexpr、concepts、lambda。协程和异常可以通过特性开关禁用。

### 8.2 "前期依赖 libclang"要改成"依赖 Clang 的 C++ 库"

- libclang（C API）是稳定的只读游标 AST：不能挂 Sema、不能注册自定义属性、没有 CFG 分析。
- 应该链接 Clang 的 C++ 库：clangAST、Sema、Frontend、Tooling、Analysis，并固定到和 mcppls 相同的版本 23.1。

用 Clang C++ 库可以做到：
- 限制类特性：用 AST Matcher 实现；
- 宏和 `#include` 的门禁：用 PPCallbacks 实现；
- 扩展语义：通过 Clang 的属性插件注册 `[[mcpp::...]]`；
- 生命周期检查：基于 Clang Analysis 的 CFG 实现。

代价：Clang C++ 库不保证 ABI 稳定，必须绑定版本。要么由 xlings 和 mcpp 提供 LLVM/Clang 的开发库，要么自己构建。这是一项需要落实的集成工作。

### 8.3 特性的细粒度控制：整个方案的主轴

- **特性注册表**：每个特性有 id（例如 `raw-pointer`、`pointer-arithmetic`、`new-delete`、`reinterpret-cast`、`c-array`、`union`、`goto`、`macros`、`exceptions`、`rtti`、`coroutines`、`adl`、`lib:std.vector`……）。每个特性标明：
  - 类别：限制类（从 C++ 里去掉能力），或扩展类（加入新语义，例如生命周期注解、借用检查）；
  - 检查层：语法 / 声明 / 表达式 / 控制流。
- **Profile**：特性的集合，例如 `c++23`（全部允许）、`mc++.safe`，以及用户自定义的组合。每个特性可以设为 `allow`、`warn`、`deny` 三档，类似 Rust 的 lint 级别和 edition。
- **粒度**：包 → 模块 → 命名空间/声明，用 `[[mcpp::allow("x")]]` 做逃生口，并且留有审计记录。**模块是方言的基本单位。**
- **写进 IFC**：模块生效的特性集写在 `VendorExtension` 里（8.5 节"厂商扩展约定"就此有了具体内容）。导入方据此做**跨模块的边界检查**，例如一个 `mc++.safe` 模块导入了一个接口里暴露裸指针的模块时，必须显式标注。
- **关键的协同效应**：**profile 禁止的特性，自研前端就不需要实现。** 所以 profile 就是自研前端的里程碑：前端实现了某个 profile 允许的全部特性，就说明它支持这个 profile。"细粒度特性控制"和"前端能不能写完"在这里是同一件事。

### 8.4 路线：MC++ 双轨

| 阶段 | 轨 1：Clang 内核（立即可用） | 轨 2：自研前端（逐步替换） | 给 mcppls / mcpp-safe 的交付 |
|---|---|---|---|
| **M0** | `mcxx` v0 = Clang 驱动 + 特性注册表和门禁 + 生成 IFC（含方言信息）；接入 mcpp，作为一个工具链；mcppls 的 clangd 从源码构建，把门禁检查编进去 | - | 门禁在编辑器和构建里同源；编译即强制执行 |
| **M1** | 同上 | 语法层（词法、有限的预处理、完整语法树）；在进程内与 Clang 做差分测试 | mcppls native 引擎升级；秒级的语法级门禁 |
| **M2** | 同上 | 声明层 + 读取 IFC 形式的 import | mcppls 不依赖 clangd 也能给出语义答案（绕开 1–4 分钟的 BMI 重建） |
| **M3** | 同上 | 表达式层，覆盖 `mc++.safe` profile | mcpp-safe 的表达式层规则秒级出结果 |
| **M4** | Clang 继续负责 classic 方言的模块 | 为 MC++ 方言的模块生成代码（LLVM IR/bitcode）；混合构建 | **独立的 MC++ 编译器** |

- 每一阶段都用语料验收：先是 mcppls（纯模块），再是 mcpp（带全局模块片段）。
- 验收指标：与轨 1（进程内的 Clang）差分一致率。
- 规模：轨 1 自己的代码约 1.5 万–3 万行；轨 2 的 M1–M3 约 10 万–15 万行（砍掉老包袱之后按前面估算的下限算）；M4 需要另外评估。每一步都不超过 mcpp 的体量；全部合计多年下来约为 mcpp 的 1.5–2.5 倍。

### 8.5 必须提前面对的一个分叉：`std`

现代标准库大量使用推导返回类型（`auto` 返回值、ranges 的各种适配器、`std::format`）。**即使只是给用户代码定类型，也需要实例化标准库模板的函数体**，也就是要能处理 libc++/libstdc++ 的实现代码。这会把自研前端拉回接近完整 C++ 的一致性要求。有三条路：

| 选项 | 做法 | 代价 |
|---|---|---|
| A. 忠实兼容 `std` | 自研前端能处理 libc++ 的实现代码 | 接近完整 C++，和"只做 module + safe"相矛盾 |
| B. 自有标准库 `std2` | 用 MC++ 子集写一套安全标准库（Circle 为 Safe C++ 做过 std2）；与老 C++ 通过边界互通（mcpp-plugins 的 `tools-island` 已经有生成 `extern "C"` 边界的机制） | 需要另起一个库生态；已有代码要迁移 |
| **C. 混合（建议）** | 按模块区分方言：classic 模块（包括使用标准 `std` 的代码）交给轨 1 的 Clang；MC++ 方言的模块交给自研前端，**逐步以 std2 为主**；两者之间通过 IFC 互通 | 要定义跨方言的规则：MC++ 模块只能使用 classic 模块中非模板的接口、显式实例化的接口，或者通过 std2 使用 |

建议：M0–M1 期间不用做这个决定，因为轨 1 什么都能处理。**但是 IFC 里的方言信息（8.3 节）现在就要设计进去。** 到 M2 时，再根据差分数据在 C 方案的细节上拍板。

## 9. 第二轮 review 的决定与架构（MC++ = 以插件系统为核心的编译器）

### 9.1 已定

1. 双轨方案（8.4 节）：同意。
2. **全部走 mcpp 体系**：MC++ 用 mcpp 构建，LLVM/Clang 的开发库也通过 mcpp 引入。Clang C++ 库 ABI 不稳定没有关系，**MC++ 提供自己的一层，Clang 只是前期依赖**。
3. 核心支柱：**Modules + 特性/库控制 + Safe 机制 + 组件化 + 插件化**。编译器本身就是一个插件系统，可以扩展；例如很容易在代码里嵌入 GPU C++，而且不和 MC++ 核心耦合。
4. M0 = `mcxx` v0 + 首个 profile `mc++.safe`：同意。
5. 先使用标准 `std`，等成为独立编译器时再考虑 `std2`（理由见 9.6 节）。

### 9.2 第 2 点和第 3 点的交汇：MC++ 语义 API（下称 MSA）

- MC++ 定义自己的一套语义 API（MSA），包括节点、声明、类型、表达式、查询、诊断、源码位置。
- **规则、分析、插件、mcppls 都只依赖 MSA，永远看不到 Clang 的类型。**
- 前期 MSA 由 `mcxx.backend.clang` 基于 Clang AST 实现；自研前端成熟之后，由它来实现 MSA。**写在 MSA 之上的东西，在这次切换中不需要修改。**
- `mcxx.backend.clang` 是**唯一** import Clang 的组件，这和 mcppls 里 `modules/platform` 是唯一 import openkal 的地方同一个道理。用分层检查来保证这一点（参照 `mcppls-devtools check layers`）。
- **切换期的好处**：在自研前端算不出某个结果时（例如需要实例化 `std` 的推导返回类型），可以通过 MSA 把这个查询转给 Clang 后端。所以 M1–M3 阶段自研前端可以"只做一部分，其余转给 Clang"，一直到 M4 都不必为 `std` 做完整的一致性。**这就是第 5 点可以推迟决定的技术前提。**

### 9.3 组件化（每个组件都是一个 mcpp 包或模块）

```
mcxx.core          会话、诊断、源码管理、字符串与内存
mcxx.modules       模块图、IFC 读写（锁定一个 IFC 版本）、方言信息（VendorExtension）
mcxx.features      特性注册表、profile、门禁引擎（包 / 模块 / 命名空间 / 声明 / 区域）
mcxx.msa           MC++ 语义 API（稳定层）
mcxx.backend.clang MSA 基于 Clang 的实现（唯一 import Clang 的地方）
mcxx.frontend      自研前端：语法 / 声明 / 表达式（M1–M3），逐步实现 MSA
mcxx.analysis.*    生命周期等基于 CFG 的分析（依赖 MSA）
mcxx.plugin        插件 ABI 与加载器
mcxx.driver        命令行（兼容 clang 的一个参数子集）；作为 mcpp 的工具链之一
```

使用方：
- mcpp：把 `mcxx.driver` 当作工具链；
- mcppls：在进程内链接 `core`、`modules`、`features`、`msa`，或者像 clangd 一样起一个 `mcxx serve` 进程；
- mcpp-safe：就是 `mcxx.features` 里的一组 profile，加上一组规则插件。

### 9.4 插件化：扩展点

| 扩展点 | 插件能做什么 | 例子 |
|---|---|---|
| 特性 / 规则 | 注册特性 id，在 MSA 上实现检查 | `mc++.safe` 的规则集、团队代码风格 |
| 库控制 | 按"声明归属于哪个模块或包"来允许或禁止使用 | `lib:std.vector`、`lib:std.regex`、`lib:<第三方包>.<符号>`（结合 mcpp 的包图） |
| 属性 / 注解 | 认领 `[[ns::attr]]`，拿到对应的 MSA 子树 | `[[mcpp::lifetime(...)]]`、`[[gpu::kernel]]` |
| 区域 / 方言 | 认领一个代码区域（函数、命名空间、模块），给它套用自己的 profile，并负责区域的降级 | GPU 设备代码、嵌入式中断处理函数 |
| 分析 pass | 基于 MSA 的 CFG 和数据流 | 生命周期、借用检查 |
| 后端 | 接收降级后的 IR | 主机端 LLVM IR；设备端 NVPTX / SPIR-V |
| 输出 | 导出事实、诊断、接口 | IFC 写出、SARIF、给 mcppls 的事实流 |

**插件的 ABI**：插件由项目的工具链构建（可能是 GCC + libstdc++），而 mcxx 由 openkal 构建（clang + libc++）。**两者之间不能直接传 C++ 对象**（例如两边的 `std::string` 布局不同）。所以：
- 进程内边界采用 **C ABI + 不透明句柄 + 版本化协议号**；
- 插件那一侧用一个 C++ 模块做类型安全的封装（`import mcxx.plugin;`）。

这和 mcpp 的构建插件是同一种设计：`import mcpp;` 是带类型的表面 API，底下是版本化的协议。插件以 mcpp 包的形式分发，用 `[build-dependencies]`、`host-module = true` 引入，和 `mcpp.rules.*` 的机制一致。

### 9.5 嵌入 GPU C++ 而不和核心耦合（以区域插件为例）

```cpp
export module app;
import std;
import mcxx.gpu;                       // GPU 插件提供的模块表面

[[gpu::kernel]] void saxpy(gpu::span<float> y, gpu::span<const float> x, float a) {
    auto i = gpu::thread_index();
    if (i < y.size()) y[i] = a * x[i] + y[i];
}

int main() { gpu::launch(saxpy, gpu::grid { 1024 }, y, x, 2.0f); }
```

1. MC++ 核心只做三件事：解析；看到 `[[gpu::kernel]]` 已被 GPU 插件认领；把这个函数作为一个**区域**交给插件。**核心不知道 CUDA 是什么。**
2. 插件给这个区域套用自己的 profile `gpu.device`（禁止异常、RTTI、虚调用、递归、主机内存分配……）。这复用的正是 Safe 用的那套门禁引擎：**"Safe" 和 "GPU 设备子集"是同一种机制，只是作用在不同的区域上**。
3. 降级分两个阶段：
   - 前期：插件把内核抽取成独立的 `.cu` 或 `.sycl` 编译单元，通过 mcpp-plugins 现有的 `rules-cuda`、`rules-sycl`、`rules-hip` 去编译，并生成主机端的启动桩；
   - 后期：插件直接在 MC++ 的 IR 上生成 NVPTX 或 SPIR-V。
4. **两层插件各管一层，互相补充**：mcpp 的构建插件（`mcpp.rules.*`）管文件和动作这一层，即"哪个编译器编译哪个文件"；MC++ 的编译器插件管编译单元内部的语义这一层，即"这段代码属于谁、按什么规则处理"。一个 GPU 包可以同时提供这两种插件。

### 9.6 关于第 5 点：先用 `std`

- 同意。理由有三：
  1. 轨 1（Clang）本来就能完整处理 `std`；
  2. 9.2 节的查询转发，让自研前端在 M1–M3 阶段可以把 `std` 相关的查询交给 Clang；
  3. `std2` 只在 M4（独立生成代码）时才会成为硬性门槛。
- 现在要做的准备只有两件，成本都很低：
  1. IFC 里预留方言信息；
  2. **库控制插件顺带统计 MC++ 代码实际用到了 `std` 的哪些部分**。等到 M4 需要决定 `std2` 的范围时，就有真实数据可依，不用凭空设计。

### 9.7 下一步（M0 的具体范围）

1. 在 mcpp 体系中引入 LLVM/Clang 23.1 的开发库（与 mcppls 的 clangd 版本一致），先确认 xlings 和 mcpp 能提供哪些，缺什么补什么。
2. 搭好组件骨架：`core`、`features`、`msa`（首批只覆盖 T1/T2 需要的查询）、`backend.clang`、`plugin`（C ABI v1）、`driver`。
3. `mc++.safe` profile v0：选 5–8 个限制类特性（例如 `raw-pointer-arithmetic`、`new-delete`、`reinterpret-cast`、`c-array`、`goto`、`macros`，以及 `lib:std.vector` 作为库控制的样例）。
4. 验收：用 `mcxx` 编译 mcppls 的语料（纯模块），结果与 clang 一致；门禁规则的检出结果与 `2026-09-28-universal-bmi-analysis.md` 第 4 节 GCC 插件探针的结果一致；输出的 IFC 能被 `ifc-printer` 正确读取。
