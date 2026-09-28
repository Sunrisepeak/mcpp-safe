# 通用 BMI 抽象层分析

日期：2026-09-28
起因：对《mcpp-safe 可行性调研》（`2026-09-28-mcpp-safe-feasibility.md`）的 review 意见：

> BMI 各编译器之间不兼容 → 直接分析各个编译器的 BMI 格式和实现，自定义抽象层、通用 BMI（只要比 mcpp 现在复杂度低都不要说难）

复杂度基准，按本机源码统计的 `.cpp/.cppm` 行数：**mcpp 约 12.2 万行**，mcppls 约 4.7 万行。下文所说的"难/不难"都以此为参照，并给出行数估算，不用形容词代替。

## 1. 结论

1. **通用抽象层这个方向是对的，复杂度也远低于 mcpp，可以做。** 但抽象层不应放在"BMI 二进制格式"这一层，而应放在"语义模型"这一层。原因见第 3 节。
2. **不需要自己解析 BMI 二进制。** 每个编译器本来就会读自己的 BMI。GCC 插件和 Clang 插件运行在编译器进程内部，拿到的就是编译器已经从 BMI 里读出来的完整语义。MSVC 的 IFC 有官方开源 SDK。三家都是让编译器自己读，我们只负责把结果转成通用模型。
3. **已实测**：GCC 16.1（mcpp 使用的 xlings 工具链）上，一个约 40 行的插件在真实编译中生效：
   - 隔着 `import` 识别出 `auto v = make();` 推导出的 `std::vector<int>`；
   - 识别出导出别名 `Buf@lib`；
   - 报 error，编译失败。

   全程没有重建任何 BMI，也没有第二次解析。
4. 这推翻了上一份报告中的两处判断：
   - "最难的是给 Clang 重建 BMI"。对 GCC 和 Clang 工具链来说，这一步可以完全去掉，它只在 MSVC 的兜底路径上还需要。
   - "Clang 与 GCC 看到的代码不一致"。规则直接跑在真实编译器里，这个问题不存在了。
5. 通用 BMI 仍然有价值，但它的身份变了：它是通用语义模型按模块序列化后的文件（下文称 **UBMI**）。mcpp-safe 用它做跨模块规则，**mcppls 可以直接读它获得模块语义**，不用等 clangd 预构建模块。这一点对 mcppls 的冷启动收益最大。

## 2. 三家 BMI 格式实测

| | GCC `.gcm` | Clang `.pcm` | MSVC `.ifc` |
|---|---|---|---|
| 容器 | ELF32 节区（"Encapsulated Lazy Records Of Numbered Declarations"），可以用 `readelf` 查看 | LLVM bitstream，魔数 `CPCH`，可以用 `llvm-bcanalyzer` 查看 | 公开规范定义的二进制格式（按 partition 组织的表） |
| 内容 | **GCC tree 结构体字段的顺序流**：`trees_out::core_vals` 逐字段写出 `t->decl_minimal.name`、`t->type_common.context` 等。每个节区是一个声明强连通分量（SCC），声明和定义不拆开 | **与 Clang AST 类一一对应的记录**：`DECL_FUNCTION`、`DECL_NAMESPACE`、`TYPE_FUNCTION_PROTO`、`TYPE_RECORD` 等；`LANGUAGE_OPTIONS` 是 400 多个字段的原样转储 | 规范原话："formally define a binary format for describing the *semantics* of C++ programs"，设计原则之一是 "Compiler neutrality" |
| 版本锁 | `MODULE_VERSION = major*10000+minor`，不一致直接报错拒绝（`module.cc:20727`；只有 `-fmodule-version-ignore` 能强行放过） | `METADATA` 里有 major 版本号（实测为 35）加完整的 git 修订字符串，不一致即拒绝 | 规范仍是草案 |
| 公开程度 | 只有源码：`gcc/cp/module.cc` 共 24,090 行，其中读取侧约 6,400 行；tree code 共 346 种（`tree.def` + `cp-tree.def`） | 只有源码（`clang/lib/Serialization`） | 规范 CC-BY-4.0；SDK `microsoft/ifc` 以 Apache-2.0 WITH LLVM-exception 开源，带读写库和 `ifc-printer`。已知限制："non-inline functions or methods will not be generated into the IFC" |
| 实测样本 | `huxerui.gcm`：28 MB，9,564 个节区（`import std` 带入了整个 std） | `greet.cppm.pcm`：18 KB，模块 `hello.greet` | 本机没有 MSVC 样本 |

**关键观察：gcm 和 pcm 本质上不是"格式"，而是编译器内存对象的序列化。** 它们的布局就是编译器内部 AST 的布局，随每个版本变化，编译器自己也拒绝读取其他版本写出的文件。只有 IFC 是按"编译器中立的公开格式"设计的。

## 3. "通用 BMI"有三种做法，成本和收益各不相同

### 做法 A：自己解析二进制，读进通用模型

- GCC：重写 `trees_in` 的读取逻辑（约 6,400 行），外加一份 346 种 tree code 的镜像数据结构。因为声明和定义在同一个 SCC 流里、不带长度前缀，即使只要声明也必须能解码完整的 tree 流。
- Clang：重写 `ASTReader` 的一个子集。
- **每个编译器版本都要跟一遍**：两家都把格式和具体版本绑死了。
- 规模：首版约 1.5–3 万行，之后每个 GCC/Clang 版本都要持续维护。低于 mcpp 的体量，但它是**做法 B 的严格劣化版**，没有理由选。

### 做法 B：让编译器自己读，导出到通用模型（推荐）

| 编译器 | 适配方式 | 版本问题 |
|---|---|---|
| GCC | 插件（`-fplugin`），在 `PLUGIN_PRE_GENERICIZE` / `PLUGIN_FINISH_UNIT` 中遍历 tree。import 进来的声明已经由 GCC 自己从 gcm 读出 | 插件按 GCC 版本重新编译即可，**格式细节由 GCC 负责** |
| Clang | 插件（`-fplugin`）或 libTooling。pcm 由 Clang 自己读 | 同上 |
| MSVC | 用 `microsoft/ifc` SDK 读 `.ifc` 得到接口层；函数体用 clang-cl 重新解析兜底（MSVC 没有公开的前端插件 API） | IFC 是公开格式 |

通用模型分三层，按需实现：

| 层 | 内容 | 用途 |
|---|---|---|
| T1 声明/类型/导出 | 模块、导出实体、完整类型、签名 | 接口规则（例如"导出 API 不得暴露裸指针"）；**mcppls 的模块语义** |
| T2 使用点事实 | 函数体内每个变量、表达式的类型，cast、指针运算、new/delete、异常 | 禁用类规则（禁止 vector、禁止指针等） |
| T3 控制流/数据流 | CFG、定义-使用、对象生命周期事件 | 生命周期检查 |

### 做法 C：把 gcm/ifc 翻译成 Clang AST，让 Clang Sema 直接吃

- 通过 Clang 的 `ExternalASTSource`，把 GCC tree 转成 Clang Decl/Type/Stmt，包括模板体和 libstdc++ 里用到的 GCC 内建函数。
- 规模：约数万行，仍低于 mcpp。
- **不推荐，原因是收益而不是难度。** 被检查的 TU 本身还是由 Clang 解析，"Clang 与 GCC 看到的代码不一致"只消除了一半。它唯一省下的是"用 Clang 重编接口单元"这一步，而这一步 mcppls 已经实现了。

## 4. 实测：在 GCC 真实编译中强制执行"禁止 vector"

环境：GCC 16.1.0（`~/.xlings/data/xpkgs/xim-x-gcc/16.1.0`，自带 `plugin/include`）。

```cpp
// lib.cppm
module;
#include <vector>
export module lib;
export using Buf = std::vector<int>;
export Buf make() { return {1, 2, 3}; }

// main.cpp
import lib;
int main() {
  auto v = make();   // std::vector 只能通过跨模块推导得知
  Buf w;             // 通过导出别名
  int ok = 0;
  return (int)v.size() + (int)w.size() + ok;
}
```

插件的核心逻辑（完整约 40 行）：在 `PLUGIN_PRE_GENERICIZE` 中遍历每个函数体，遇到 `VAR_DECL` 就判断它的类型是否为 `std` 命名空间下 `vector` 模板的特化（`CLASSTYPE_TI_TEMPLATE` + `decl_in_std_namespace_p`），是就调用 `error_at`。

```
$ g++ -std=c++23 -fmodules -fplugin=./ban.so -c main.cpp
main.cpp:3:8: error: [mcpp-safe/no-vector] 'v' has type 'std::vector<int>'
main.cpp:4:7: error: [mcpp-safe/no-vector] 'w' has type 'Buf@lib' {aka 'std::vector<int>'}
exit=1
```

- `ok` 没有被误报；隔着模块边界的推导和别名都被识别出来；报 error 后编译失败，**本身就是强制执行**。
- 每处报了两次，是因为探针同时遍历了 `DECL_EXPR` 和其中的 `VAR_DECL`，属于探针代码的写法问题，与方案无关。
- 探针源码在会话 scratchpad 的 `gccprobe/` 目录中，没有放进仓库。

实测中遇到的三个环境问题，都是**打包问题，不是方案问题**：

1. `plugin/include/system.h` 需要 `gmp.h`，xlings 的 GCC 包没带。临时借用了系统里的那份。
2. 在 xlings 的 loader 下，插件找不到 `libstdc++.so.6`。改为 `-static-libstdc++ -static-libgcc` 后解决。
3. `plugin_default_version_check` 失败：插件头文件的 `configuration_arguments` 和运行中的 cc1plus 不一致（xlings 的 GCC 包里，头文件与编译器来自不同的构建配置）。把检查放宽到 `basever` 一致之后可以正常运行。**更稳妥的做法是在 xlings 的 GCC 包里修正。**

## 5. 接入 mcpp

- 插件参数：`build.mcpp` 输出 `mcpp:cxxflag=-fplugin=<path>`（2026.9.17.1+），或在 `mcpp.toml` 的编译参数里加上。
- 插件本身：用项目自己的工具链作为宿主工具构建（mcpp 已支持从依赖构建宿主工具）。插件和编译器同源，版本自然一致。
- 规则失败就是编译失败，**这种情况下不需要单独的 check action**。check action 仍保留给 MSVC 兜底路径和"只报告、不拦截"模式使用。
- UBMI：插件在编译模块接口时，顺带输出 `<module>.ubmi`（T1 层），和 `.gcm` 放在一起；mcppls 通过 `mcpp emit build-database` 找到它们。

## 6. 复杂度估算（对照 mcpp 约 12.2 万行）

| 组件 | 估算行数 | 备注 |
|---|---|---|
| 通用语义模型 + UBMI 序列化 | 3k–5k | T1 + T2 |
| GCC 适配（插件，T1+T2） | 3k–6k | 本次探针已打通关键路径 |
| Clang 适配（插件，T1+T2） | 3k–6k | Clang 插件和 AST 接口是成熟公开 API |
| MSVC 适配（IFC SDK 读 T1；T2 走 clang-cl） | 2k–4k | T2 复用 Clang 适配 |
| 规则引擎 + 配置/作用域/豁免/基线 | 5k–10k | 规则只写一份，面向模型 |
| mcpp 插件（`mcpp.tools.safe`） | 1k 以内 | 构建插件 + 注入参数 |
| mcppls 读取 UBMI | 1k–2k | 接入 native 引擎 |
| **合计（T1+T2）** | **约 1.8 万–3.4 万** | **约为 mcpp 的 15%–28%，按你的标准不算难** |
| T3 生命周期 | 另算 | 取决于深度。GCC 已有 `-Wdangling-pointer`、`-Wdangling-reference`、`-Wuse-after-free`，Clang 有 `-Wdangling*` 和 `lifetimebound`，可以先接这些现成能力 |

## 7. 需要确认的约束

1. **许可证**：GCC 插件必须声明 `plugin_is_GPL_compatible`。GCC 适配部分要采用与 GPLv3 兼容的许可证（Apache-2.0 满足）；分发时是否把它单独成包，需要你来定。
2. **每个编译器版本一份插件**：插件依赖 GCC/Clang 内部的 ABI，每个版本都要重新构建（不需要改代码，除非内部 API 变了）。由 mcpp 按项目工具链现场构建可以覆盖这一点。
3. **MSVC 是弱项**：没有公开的前端插件 API，T2 只能通过 clang-cl 重新解析，这条路径上"两套编译器看到的代码不一致"的问题依然存在。
4. **两个适配器要做一致性测试**：同一批测试用例，GCC 和 Clang 两边输出的模型事实必须相同。可以沿用 mcppls conformance 的做法。
5. **xlings GCC 包需要补齐**：`gmp.h`，以及与编译器一致的插件头文件配置。

## 8. 讨论：以 IFC 作为 UBMI，还是自定义格式？（review 第二轮）

### 8.1 IFC 的实际能力（`microsoft/ifc` 的 `abstract-sgraph.hxx`）

| 维度 | 内容 |
|---|---|
| 覆盖面 | `DeclSort` 27+、`TypeSort` 21+、`ExprSort` 50+、`StmtSort` 19+、`SyntaxSort` 100+、`NameSort`、`ChartSort`（模板参数表）、`AttrSort`、`DirSort`（pragma、using 等） |
| 扩展点 | 每个主要 sort 的第一个枚举值都是 `VendorExtension`，GCC/Clang 特有的东西有地方放 |
| 模板 | 参数表用 Chart 表示；**模板定义用 Syntax 树表示**（语法层），不是语义树 |
| MSVC 色彩 | `VendorTraits`（`__forceinline`、`dllexport`、`vtordisp`……）、SEH、调用约定 |
| 版本 | 头文件里没有格式版本常量；SDK 的说法是适配 "a current MSVC"，版本跟随 MSVC 演进 |
| 其他编译器 | 没有发现 GCC/Clang 读写 IFC 的实现；P2581 只把"Clang 读 IFC"作为假设用例提过 |

### 8.2 结论：UBMI 直接采用 IFC，不自己设计格式

- 我们设计的 T1（声明、类型、导出）是 IFC 的子集，自己设计等于把 IFC 已经做过的设计工作重做一遍。
- MSVC 原生产出 IFC，MSVC 这一侧不需要写生成代码。
- SDK 里的读写库、`ifc-printer` 和 viewer 可以直接拿来用。
- **同一份源码，MSVC 产出的 IFC 就是参考答案**：GCC/Clang 适配器产出的 IFC 可以直接和它 diff，一致性测试有了现成的标准答案。
- 用法约定：锁定一个 IFC 规范版本；GCC/Clang 特有的内容统一放进 `VendorExtension`，并制定一份 mcpp 自己的厂商扩展约定。

**修正第 3 节**：T2 使用点事实**不进 BMI**。它是单个 TU 的分析结果，不是接口的一部分；插件在编译器进程内直接消费，不需要序列化。UBMI 只包含 T1，以及 IFC 本身就会携带的 inline/constexpr 函数体。

### 8.3 统一实现放在哪一侧

| 位置 | 做法 | 代码关系 | 用途 | 建议 |
|---|---|---|---|---|
| **生成侧** | GCC/Clang 插件在编译模块接口时，同时写出 `.ifc`，和 `.gcm`/`.pcm` 放在一起 | 适配器本体 | mcpp 自己构建的所有模块 | **主路径，第一阶段** |
| **转换侧** | 离线把 `.gcm`/`.pcm` 转成 `.ifc`。读 gcm 仍由 GCC 自己完成：用挂着同一个插件的 cc1plus 编译一个只写了 `import M;` 的桩 TU，插件遍历导入的声明并写出 IFC；pcm 用 Clang 库加载 | **和生成侧是同一套代码**，只是触发方式不同 | 不是我们构建的 BMI（预构建包、第三方）；必须使用与 BMI 完全一致的编译器版本（mcpp 掌握工具链，可以满足） | 第二阶段，顺带实现 |
| **消费侧** | 让 GCC/Clang 直接 `import` 一个 IFC，把它当 BMI 用 | 需要修改编译器：在读取 BMI 的位置换成"IFC → 编译器内部 AST"的构建器 | 真正的跨编译器 BMI。唯一有实际价值的消费者是 **mcppls 的 clangd**：直接吃 GCC 构建出的模块，不用再为 Clang 重建 BMI | 暂不做，见下 |

消费侧的成本和价值：

- 成本：每个编译器约 2 万–5 万行（Clang 可以走 `ExternalASTSource`；GCC 要替换 `module.cc` 的读取侧），低于 mcpp 的 12.2 万行。此外要长期维护对编译器的修改；xlings 本来就自己打包 GCC，mcppls 本来就自己发布 clangd，所以运维上可行。
- 价值有限：
  - 构建时混用两种编译器本来就少见，ABI（libstdc++ 与 libc++、Itanium 与 MSVC 的名字修饰）通常也不允许。
  - 真正的收益集中在 mcppls 的冷启动上。
  - IFC 的模板定义是语法树，而 GCC 保存的模板是半语义的 tree，双向转换都要重新做一遍语义分析。
- 建议：生成侧上线之后，先量化 mcppls 冷启动中"为 Clang 重建 BMI"占多少时间，再决定要不要做。

### 8.4 B 方案的成本清单

| 项 | 性质 | 量级 |
|---|---|---|
| GCC、Clang 两个适配器 | 一次性 | 各 3k–6k 行，产出 IFC（T1）并在进程内提供 T2 事实 |
| MSVC 生成侧 | 没有 | MSVC 原生产出 |
| 编译器版本跟进 | **主要的持续成本** | GCC 插件 API 就是 GCC 的全部内部结构，**没有兼容性承诺**；GCC 每年发一个大版本，预计每次要小改。Clang 的 AST API 相对稳定。量级和 mcppls 维护 clangd workaround 注册表相当 |
| 插件构建 | 自动 | 由 mcpp 用项目的工具链作为宿主工具现场构建，版本天然一致 |
| 打包 | 一次性 | 第 4 节发现的三个 xlings GCC 包问题 |
| 一致性 | 持续，但有标准答案 | 同一份源码的 IFC：MSVC 产出的作参考，与 GCC、Clang 适配器的产出做 diff |

结论：**B 方案不麻烦**。一次性工作约 1 万行，持续成本集中在每年一次的 GCC 大版本跟进。

### 8.5 review 第三轮的决定与澄清

1. **IFC 版本**：锁定一个规范版本（已定）。
2. **"厂商扩展约定"指什么**：它是**数据约定**，不是代码机制，和 mcpp 的构建插件框架属于两个不同的层次。
   - 构建插件框架（`mcpp.tools.safe`）回答"代码在构建的哪一步运行"：它负责构建 GCC/Clang 插件、注入 `-fplugin`、提交 check action。
   - 厂商扩展约定回答"`.ifc` 文件里，IFC 标准没有定义的那部分字节怎么写"。IFC 规定：遇到 `VendorExtension` 时，内容由厂商自己定义。GCC 适配器和 Clang 适配器遇到同一个标准外的构造，必须写出同样的编码；mcppls 和 mcpp-safe 的规则读取时才能看懂。
   - 实际需要的量预计很小：`[[gnu::xxx]]` 这类属性可以直接放进 IFC 标准的 `AttrSort::Scoped`；真正需要扩展的只有 IFC 表达不了的少数构造（例如 GCC 的向量类型、语句表达式 `({...})`、`_BitInt(N)`），要在实现适配器时逐项确认。
   - 落地形式：mcpp-safe 仓库中的一页规范（带版本号），加一个两边适配器和读取方共用的编解码模块。不需要像 mcppls 的 S1–S5 那样单独立一套规范。
3. **"消费侧等数据再决定"指什么**：
   - 现状：mcppls 打开一个用 GCC 构建的 mcpp 工程时，clangd 读不了 `.gcm`，只能用 Clang 把所有模块（包括 `std`）的 BMI 再构建一遍。这是冷启动慢的主要来源之一。已有实测（`mcpp-language-server/.agents/docs/2026-09-22-real-project-experience.md`）：xlings 约 110 个模块，29 s 就绪，CPU 221 s，峰值内存 5.7 GB；mcpp 仓库 20 s 就绪，CPU 122 s，峰值内存 4.0 GB。发布门槛是冷启动首次导航的中位数低于 12 s。
   - 消费侧要做的是改 clangd，让它能直接读取我们 GCC 插件已经写好的 `.ifc`，从而省掉这次重建。
   - 这几个数字里混着 BMI 构建、索引和其他开销，**BMI 重建究竟占多少还没有拆分过**。先在 mcppls 里把这部分单独计时：占比高就做消费侧，占比低就不做。

### 8.6 实测：mcppls 冷启动里 BMI 重建占多少（2026-09-28）

**方法**：临时 draft PR [Sunrisepeak/mcpp-language-server#29](https://github.com/Sunrisepeak/mcpp-language-server/pull/29)（`[DO NOT MERGE]`，分支 `tmp/bmi-rebuild-timing`）。

- 插桩：在 clangd 引擎中记录每个模块 prime 的时长（从 `import M;` 的 didOpen 到 clangd 发回首次诊断，基本就是 clangd 为 M 构建 BMI 的时间）、整个准备阶段的时间窗口，以及 clangd 在窗口两端的 CPU 秒数。
- conformance runner 等准备阶段结束后再取数。
- CI 环境：GitHub `ubuntu-24.04`，4 核，因此 primer 并发上限为 1，**模块是串行构建的**。
- 每个工程在同一个 workspace 和缓存上各跑一次冷启动（缓存为空）和一次热启动。
- 原始数据：run 36415909922 的 artifact `bmi-timing`。同一 PR 上原有的 CI 全部通过。

| 工程 | 启动 | prime 模块数 | 准备窗口 | prime 时长合计 | 平均/模块 | 最慢 | 首次跳转 | 窗口内 clangd CPU（占当时总量） |
|---|---|---|---|---|---|---|---|---|
| self-mcpp（mcpp 仓库，GCC 16） | 冷 | 174 | **228 s**（1.1 → 229.3 s） | 228 s | 1.3 s | `std` 5.7 s | **126.0 s** | 727 s（99.9%） |
| | 热 | 0 | - | - | - | - | **78.5 s** | 运行期间共 81 s |
| real-xlings | 冷 | 101 | 184 s | 175 s | 1.7 s | `xlings.core.xvm.errors` 12.9 s，`std` 6.3 s | 12.3 s | 542 s（99%） |
| | 热 | 0 | - | - | - | - | 6.7 s | 272 s |
| self-mcppls | 冷 | 54 | 190 s | 121 s | 2.2 s | `mcppls.project.provider` 18.9 s | 15.1 s | 435 s（99%） |
| | 热 | 0 | - | - | - | - | 9.4 s | 393 s |

说明：

- self-mcpp 是最干净的一组。real-xlings 和 self-mcppls 的 STRESS1 检查（随机开文件、发请求）与准备阶段在时间上重叠，prime 时长被抢占拉长。real-xlings 的两次失败都是 STRESS1 的 p90 延迟预算（3.39 s 和 5.0 s，预算 3 s），和计时无关。
- 准备窗口里 clangd 的 CPU/墙钟约为 3.2，而 prime 是串行的，说明窗口里还有 **background index**（mcppls 默认开启 `--background-index`）在并行消耗 CPU。所以"窗口内 CPU"是 BMI 构建与后台索引之和。
- clang 构建一个 BMI 基本是单线程的，因此 prime 时长合计（228 / 175 / 121 s）可以看作 BMI 重建 CPU 的上界；其余约 500 / 370 / 310 CPU 秒属于后台索引。

**结论**：

1. **在缓存为空的冷启动中，BMI 重建是墙钟时间的主体。** 以 mcpp 仓库为例，4 核机器上串行构建了 228 s，"全部诊断就绪"（C1-main 在 230.8 s 完成）几乎完全被它卡住；首次跳转在冷启动下要 126 s，热启动下 78.5 s，BMI 重建至少直接贡献了其中 47.5 s（约 38%）。核数更多时 primer 的并发上限会升高，窗口会相应缩短，但 CPU 总量不变。
2. **按 CPU 算，BMI 重建不是最大的开销。** 约 175–228 CPU 秒花在 BMI 上，而同一时段后台索引用了约 2 倍的 CPU。消费侧（clangd 直接读取 IFC）最多只能省掉前者。
3. ~~热启动已经复用了 BMI（三个工程热启动时 prime 数都是 0）~~ **已被 8.7 推翻**：prime 数为 0，只说明 mcppls 看到磁盘上有 BMI、跳过了 prime；实际上 clangd 在热启动时把 176 个模块全部重建了一遍。
4. **待确认的异常**：self-mcpp 热启动时一个模块都没 prime，首次跳转却仍要 78.5 s，而且这期间 clangd 的 CPU（81 s）约等于墙钟，也就是约 1 个核一直满载。这和"clangd 在打开文件的 worker 里逐个构建它 import 的模块"的特征一致：**热启动时 clangd 可能并没有真正复用磁盘上的 BMI，而是在打开 `cli.cppm` 时，把它的 import 闭包重新构建了一遍**。这只是假设，需要一次带 clangd 日志的补充运行来确认。如果属实，这是 mcppls 这边可以直接修的问题，修好后热启动首次跳转能省下约 78 s，而且不依赖任何 IFC 工作。

**对"消费侧是否值得做"的判断**：

- 值得做。它消除的是首次打开时的主要等待：mcpp 仓库在 4 核机器上约 2–4 分钟。
- 优先级放在生成侧之后。
- 在做它之前，先查清第 4 条热启动异常。那边的收益更直接、成本更低。

### 8.7 补测：热启动其实重建了全部 BMI（根因已定位）

**方法**：同一个 PR 的第二轮（run 36419266307，artifact `bmi-timing`），只跑 self-mcpp。

- 准备阶段空闲后，把 clangd 的 info 级日志环形缓冲导出。
- 冷启动和热启动之后，各给所有 `.pcm` 做一次快照。
- clangd 23.1 会记录 `Built module`、`Reusing persistent module`、`Reusing module` 这几种日志，本地用 `timing` fixture 验证过：冷启动出现的是前者，热启动出现的是中间那种。

**结果**：

| | 首次跳转 | clangd 日志里的 `Built module` | `Reusing persistent module` | 快照中的 `.pcm` 数 |
|---|---|---|---|---|
| 冷启动 | 87.8 s（准备窗口 158.7 s） | （日志被 4000 行环形缓冲截断） | - | 176 |
| 热启动 | 55.5 s | **176** | **0** | **352**（旧的 176 个一个没用上，又新写了 176 个） |

两轮之间的绝对耗时有差异（第一轮冷/热首次跳转分别是 126 s 和 78.5 s），这是 runner 之间的波动。**热启动全量重建这一点，两轮是一致的。**

**根因**：

- clangd 模块缓存的目录是 `modules/<单元源码名>-<源码路径哈希>/<命令哈希>/`。其中命令哈希由**工作目录加上完整编译参数（不含输出）**算出，依赖的 BMI、clangd 版本都不参与（`ModulesBuilder.cpp` 的 `getCompileCommandStringHash`）。
- 同一个 `std.cc`，冷启动时命令哈希是 `25864D8C218D24EC`，热启动时是 `1A91DE07102C408E`。
- 对比 server 日志中 `main.cpp` 的完整命令：
  - 冷启动（模型来自 producer，即 `mcpp emit build-database`）：`-I… -I… -I… -std=c++23 -O2 --sysroot=… -D__mcpp_target_linux__=1 …`
  - 热启动（模型来自缓存，日志："the cached mcpp model … matches its inputs; planning with it while the producer confirms it"）：`-std=c++23 -D__mcpp_target_linux__=1 -I… …`
  - 两者参数顺序不同，并且**热启动丢了 `-O2`**。
- 模型缓存存的是 S1 结构化数据库（`src/project/modelcache.cpp` 的 `model_to_json` 调用 `spec::to_json(model.database)`）。**S1 JSON 序列化再读回来这一趟不保持参数不变**：参数被规范化重排，`-O2` 被丢弃。producer 随后确认模型 "unchanged"，没有触发重新规划，于是整个热启动都在用这套不同的参数。具体是哪一步丢的，修复时再确认。

**影响**：

- 每当模型来源在 producer 和缓存之间切换一次（例如首次打开之后的第一次重开，或者缓存失效后又回到 producer），clangd 就会把所有模块重建一遍。对 mcpp 仓库来说，就是 176 个模块、1–4 分钟。
- 另外，热启动丢掉 `-O2`，会让 clang 看到的 `__OPTIMIZE__` 等预定义宏和冷启动不一致。这本身也是一个语义差异。

**建议交给 mcppls 处理，和 mcpp-safe 无关**：

- 让两条路径产出**逐字节相同**的参数：要么缓存保存原始参数，使 S1 往返成为恒等变换；要么在进入引擎计划之前，对两条路径统一做一次规范化（固定顺序，并明确 `-O` 类参数是保留还是去掉）。
- 加一个回归检查：同一个工程冷启动之后接着热启动，热启动时 clangd 日志里 `Built module` 的数量必须是 0。现有的 `module-cache-reused` 检查只覆盖了 `std`，而且 `timing` fixture 的两条路径参数恰好一致，所以没有发现这个问题。

**对消费侧判断的修正**：

- 修掉这个问题之后，热启动能省下约 55–78 s 的首次跳转等待和整轮 BMI 重建。收益大、改动小，应当先做。
- 消费侧（clangd 读取 IFC）的价值不变，仍然只针对真正的首次冷启动和底层模块修改。优先级依然排在生成侧之后。
