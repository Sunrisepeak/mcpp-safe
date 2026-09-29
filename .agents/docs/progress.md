# 进度（按里程碑文档逐项跟踪）

每个验证项、验收项、适配项都列在这里，写明当前状态和证据。状态有四种：✅ 达成，🟡 部分达成，⏳ 进行中，⬜ 未开始。
平台默认 linux-x64（本机 32 核，按"本地优先、CI 辅助"的规则执行）。

## 验证项

| # | 状态 | 结论与证据 |
|---|---|---|
| V0.1 | ✅ | Clang/LLVM 23.1 的前端库由 mcpp 在 openkal 上构建（speak-agent/llvm-clang-dev，tag 23.1.0），smoke 程序在本机和 CI 上都能运行 |
| V0.2 | 🟡 | mcxx（dev 构建，内含 Clang、CodeGen、x86-64 和 AArch64 后端）154 MB；改动驱动后重新编译加链接 5.25 s（本机 32 核，门槛 ≤ 60 s）。4 核 CI 上的数字和 release 体积待补 |
| V0.3 | ✅ | IFC SDK 0.43.5 加上 GSL 4.2.0 放在 `index/microsoft`，在 openkal 上用 mcpp 构建通过；`ifc-printer` 能运行，并在 M1.1 读完 C-mcppls 构建出的全部 133 个 `.ifc`，零错误 |
| V0.4 | ✅ | `llvm.codegen-dev`（Clang CodeGen、LLVM 优化器和后端，x86-64 与 AArch64）和 `llvm.clang-driver`（clang 本身：driver、cc1、cc1as 在进程内）。mcxx 驱动预编译模块接口、生成目标文件，外部 `ld.lld` 链接，hello-modules 运行输出 `answer=42`；AArch64 目标文件也能生成 |
| V0.5 | ✅ | 不能。openkal 的静态进程调用 `dlopen` 返回 "Dynamic loading not supported"。MC4 只采用静态组合和进程外协议两种方式 |
| V0.6 | ✅ | 能。mcxx 作为 llvm 族的 payload，mcpp 不做任何修改，以 `mcpp build --toolchain llvm@23.1.0-mcxx` 使用它，构建并运行了一个 `import std` 的模块程序，代码由 clang 23.1 编译（`tools/checks/toolchain.py`）。<br>这需要以下几点：<br>- mcxx 的默认目标是 x86_64-unknown-linux-gnu（llvm-clang-dev 23.1.0.3，宿主仍是 openkal 的 musl），因为 mcpp 做本机构建时会传 `--no-default-config`；<br>- payload 带 mcpp 的安装标记；<br>- 重新构建 mcxx 后要清掉 mcpp 的 std 模块缓存项（`xpkgs/README.md`） |
| V0.7 | ✅ | mcpp 这一半：`[indices] llvm = { path = "index/llvm" }`。xlings 这一半：`xpkgs/pkgs/m/mcxx.lua`，用 `xlings config --add-xpkg` 注册，再 `xlings install mcxx:mcxx@0.1.0` 安装到 mcpp 的 xlings（E-XIM-1） |

## 适配项

| # | 状态 | 说明 |
|---|---|---|
| E-OK-1 | ✅ | 缺 `pread` 和文件 mmap，在构建配方中绕开（`HAVE_PREAD 0`） |
| E-OK-2 | ✅ | 见 V0.5 |
| E-IDX-1 | ✅ | `index/llvm/pkgs/c/clang-dev.lua` |
| E-IDX-2 | ✅ | `index/microsoft/pkgs/{g/gsl,i/ifc-sdk}.lua` |
| （新增） | ✅ | `index/llvm` 中的 `llvm.codegen-dev`、`llvm.clang-driver`，版本 23.1.0.2 |
| E-LS-2 | ✅ | fork 的 PR #1 |
| E-XIM-1 | ✅ | `xpkgs/pkgs/m/mcxx.lua`（见 V0.7） |
| E-MCPP-1 | ✅ | 不改 mcpp（见 V0.6） |
| E-MCPP-2 | ✅ | `mcxx compose`（见 M0.6） |
| E-LS-5 | 🟡 | linux 达成（见检查点 1） |

## 里程碑

| # | 状态 | 说明 |
|---|---|---|
| M0.5 驱动 | 🟡 | 驱动在 `modules/driver`：<br>- 命令：`c++`、`cc`、`check`、`features`、`compose`、`version --json`；<br>- 按名字调用：clang++、clang、c++、cc、g++、gcc。<br>**A0.5.3** ✅ 自举：C-mcppls（377d222）用 mcxx 作为 mcpp 工具链构建，31 个测试程序全部通过（`tools/checks/selfhost.py`，干净检出时构建 37 s，mcxx 为未优化构建）。MC++ 默认开启的 json-brace-init 在语料中发现两处真实问题：`Json result { nullptr }` 得到 `[null]`，`const Json clientId { … }` 得到 `[clientId]`，测量时设为 warn。<br>**A0.5.4** ✅ MC5 0.1.0。<br>**A0.5.2** ✅ 参数规范化。libmc++ 一侧：`derived_arguments`（MC5 §6）。以下各种形式得到逐字节相同的参数：build database 的编译命令、构建计划的预编译命令、命令行、紧贴写法、相对路径的 file 字段、从缓存读回的参数、为未列出的文件推断出的命令。值、顺序、选项不同时参数也不同（`modules/backend/clang/tests/test_arguments.cpp`）。fork 一侧：同一个 S1 模型，一次来自 producer，一次从模型缓存读回，规划得到的引擎参数逐字节相同，这正是 mcpp-language-server#30 的情形（fork 的 `tests/test_arguments.cpp`，在 PR #1 里）。<br>**A0.5.1** ✅ 语料检查：C-mcppls 的 1882 个单元（包括 openkal 运行时源码），mcxx 零错误，编译器诊断和不含 MC++ 的 clang 23.1（`driver-smoke`）完全相同。MC++ 另外报出 2 条自己的诊断，就是上面 json-brace-init 的两处（`tools/checks/corpus.py`，26 s）。 |
| M0.2 分层 | ✅ | **A0.2.1**：按依赖集合拆包。<br>**A0.2.2/A0.2.4**：`lint.py` 的两条规则，CI 强制执行：<br>- `clang-exposure`：Clang 只出现在 `backend/clang*`；<br>- `platform-exposure`：openkal 和平台宏只出现在 `modules/os`、`modules/arch`。<br>为满足这两条做了两处调整：栈切换的汇编移到 arch 包，并以常量 `STACK_SWITCH` 提供；标准错误的写入移到 os 包（`write_standard_error`）。<br>**A0.2.3**：`tools/checks/symbols.py`。features 的测试程序和用 GCC 构建的插件程序里没有 Clang/LLVM 符号；对照组 mcxx 有约 19.7 万个 |
| M0.3 门禁（fixture） | ✅ | **A0.3.3/A0.3.4**：`conformance/gates/scopes` 覆盖每一层优先级，豁免必须出现在审计里（runner 设置 `MCXX_AUDIT` 并核对），3 条豁免与期望一致 |
| M0.4 MSA | 🟡 | **A0.4.3** ✅ C-mcppls 自身的 253 个源文件，MSA 导出的 T1 声明和直接从 Clang AST 统计的结果按种类逐项相等，两边都是 18357 个（`tools/checks/facts.py`，参照是 `mcxx-probe --census`，由 Clang 自己的 RecursiveASTVisitor 统计）。<br>对照中发现并修正了一处：写在函数类型里的形参（`std::function<void(int level)>`）属于类型，不是文件的声明（MC3 §4.2）。<br>先试过用 Clang 的 JSON AST dump 作参照：它只在文件变化时写出文件名，跟踪不可靠，而且不输出 init-capture，所以没有采用。<br>A0.4.1（MC3 0.1.0）✅；A0.4.2 🟡（查询的一致性测试在 `modules/backend/clang/tests`） |
| M0.8 规范 | 🟡 | A0.8.1：`specs/` 下 MC1、MC3、MC4、MC5 都有正文、schema、示例和要求 id。`tools/checks/specs.py` 通过：316 项检查，100 条要求中 97 条有证据，其余 3 条列在 `$pending` 里，等待 A0.5.1、A0.5.2 和 V0.6。已经进入 CI。<br>这项检查发现并修复了一个缺陷：先 `--precompile`、再用 `-c` 编译 `.pcm` 时，门禁会报告两次。**A0.8.2** ✅：runner 的 JSON 报告包含每个程序和总的耗时。<br>**A0.8.3**：CI 的 job 包括以下各项：<br>- 构建；<br>- 分层与源码规则、规范检查；<br>- 单元测试；<br>- 门禁一致性测试；<br>- 自举和 T1 事实（C-mcppls）；<br>- 工具链；<br>- 组合与进程外插件。<br>A0.5.1 的语料对比需要一个参照 clang（`driver-smoke`），在本机运行 |
| M0.3 门禁 | 🟡 | 以下各项已实现，并由 `modules/features/tests` 覆盖：<br>- 配置：`profile` 可以是一个或多个；包、模块、命名空间的级别；未知的 id 和 profile 报警告；<br>- 优先级；<br>- 豁免与审计（`MCXX_AUDIT`）。<br>A0.3.1（MC1 草案，schema 与示例校验）✅；A0.3.2（`test_spec`：`mcpp.toml` 示例逐项等于规范的 JSON）✅ |
| M0.6 插件 | 🟡 | **A0.6.1** ✅ `mcxx compose`（`modules/driver`）：<br>- 第一次组合约 5 分钟，不变时约 6 s 且不重新链接；<br>- 编译时自动交给组合好的编译器；<br>- 缺少组合时报错；<br>- 端到端检查：`tools/checks/compose.py`，已进入 CI。<br>**A0.6.2** ✅ 进程外协议 1：`modules/plugin/{wire,remote,host}`。示例插件 `plugins/examples/naming-remote` 用 GCC 16 + libstdc++ 构建，经由进程外协议对同一个文件给出的诊断，和把 `plugins/examples/naming` 静态组合进来的 mcxx 完全相同（`tools/checks/compose.py --remote`，已进入 CI）。为此，只依赖 std 的包（`msa`、`plugin/{sdk,wire,remote}`）不再声明 openkal-llvm-runtime，因为这个运行时只允许 LLVM 编译器，由最终的程序来声明。<br>**A0.6.3** ✅ 崩溃、超时、乱码、协议不符都报告为 `mcxx-plugin`，编译器不崩溃、不卡住（`test_host` 故障注入）。<br>**A0.6.4** ✅ MC4 0.1.0 |
| M0.7 mc++.safe | 🟡 | **A0.7.1**：`mc++.iso` 共 15 个 ISO 特性，都带 stable name；profile `safe` 覆盖 10 个未定义行为来源；插件样例 `raw-pointers`、`lib:std.vector`、`ext:cfg`。<br>**A0.7.2**：19 个特性、20 个程序、41 个文件，精确率和召回率都是 100%（`mcxx-conformance`）。<br>**A0.7.3**：跨模块用例。<br>**A0.7.4**：`test_override`。<br>**A0.7.5**：诊断带 id、改法和豁免方法。<br>还差 MC1 的示例文件 |

| M1.3 `mcxx serve` | ✅ | **A1.3.1**：MC6 1（`specs/mc6-serve.md`），`modules/serve`：<br>- 传输是 LSP 基础协议；<br>- 文档通知和 Service 的请求；<br>- MC++ 自己的 `mcxx/setCommands`、`mcxx/facts`、`mcxx/gates`、`mcxx/catalog`；<br>- 示例会话是真实输出。<br>**A1.3.2**：`Client` 在进程被杀掉后重新启动它，重放命令和打开的文档，结果不变（`test_serve`）。请求有时间限制。`specs.py --mcxx` 检查真实的 `mcxx serve` 生命周期 |
| M1.9 属性与区域 | ✅ | **A1.9.1** 属性插件：SDK 提供 `AttributeSpec`，宿主以 `ClaimedAttrInfo` 在 Clang 中认领 catalog 里的属性，记为 MC3 事实 `attributes`，规则用 `plugin::subtree` 读取对应的子树。示例 `[[acme::hot]]`，Clang 端到端测试见 `test_backend`。<br>**A1.9.2** 区域插件：`[[acme::device]]` 在所在声明范围内套用 profile `acme.device`，范围外不受影响，范围内的豁免照样有效（`plugins/examples/device` 的测试和 `test_backend`）。<br>**A1.9.3**：MC4 0.2.0 的扩展点有规则、源码过滤器、profile、属性、区域，库控制通过规则的 library 类别实现。MC1、MC3 也升到 0.2.0 |
| M1.6 自研前端 F1：词法与预处理 | ✅ | `modules/frontend`（`mcxx.frontend`）。<br>**A1.6.1**：词法照 Clang 23.1 的原始词法器逐条实现，`tools/checks/lexdiff.py` 逐 token 比对种类、位置和原文，注释也比：<br>- C-mcppls、C-mcpp 和本仓库：1886/1886 个文件、357 万 token 一致；<br>- LLVM、Clang、libc、libcxx 源码和 libc++ 头文件：10012/10012 个文件、2554 万 token 一致；<br>- 边角语料 7/7。<br>唯一已知的不同：字面量外命名字母的 `\N{名字}`（没有 Unicode 名字表）。速度 151 MB/s，Clang 原始词法器 141 MB/s（release、同进程）。<br>**A1.6.2**：预处理覆盖语料实际用到的指令，加上 `#undef`、`#elifdef`、`#error`、`#pragma`。`#include` 只记录，purview 里的记为不确定；宏记为 MC3 `macros` 事实，由 `macros` 特性控制。不确定时照实说（`certain`）。`ppdiff.py` 对照 Clang `-E`：<br>- C-mcppls 253/253 一致；<br>- C-mcpp 带头文件宏时 334/334 一致，不带时 187 一致、147 差在头文件宏，没有"自称确定却不一致"的。<br>`mcxx-conformance --frontend`：由本前端取事实，`include` 和 `macros` 的发现和 fixture 一致。标准的例子（[cpp.scope] 例 3–5、`__VA_OPT__`）是单元测试。<br>**A1.6.3**：lint 的 clang-exposure 规则；`symbols.py` 查 `mcxx-lexdump` 和测试程序，都是 0 个 Clang/LLVM 符号。均已进入 CI |
| M1.7 自研前端 F1：语法树 | ✅ | `mcxx.frontend:syntax`：声明级解析（不知道类型，按代码形状决定），给出和 MSA `Unit::symbols()` 同形的大纲。<br>**A1.7.1**：C-mcppls 253 个文件、C-mcpp 334 个文件，没有解析诊断。<br>**A1.7.2**：`tools/checks/syntaxdiff.py` 对照 Clang 后端的大纲（种类、名字、名字范围、整体范围）：<br>- C-mcppls 4725/4725（100%）；<br>- C-mcpp 带头文件宏时 18880/18880（100%），不带时测试以外 99.942%。<br>**A1.7.3**：`mcxx-lexdump --fuzz 1000`，C-mcppls 253 000 次、C-mcpp 334 000 次，没有崩溃；调试构建另做 25 300 次。<br>**A1.7.4**：C-mcppls 整个语料单线程 0.15–0.25 s（release）。<br>另外：后端对 GCC 编译命令去掉 GCC 的模块开关（MC5 0.1.1），C-mcpp 的 GCC 编译数据库因此能在语义服务里解析。均已进入 CI（C-mcppls） |
| M1.4 接入 mcppls | ✅ | **A1.4.1** ✅：fork 的 mcxx 引擎是默认引擎。<br>**A1.4.2** ✅：编辑器（workspace 解析）和构建（`mcxx c++`：预编译接口、检查单元）对门禁 fixture 的 117 条发现完全相同（`mcxx-conformance --driver`，已进入 CI）。有 1 个文件构建时不会被编译，因为它导入的接口有门禁错误，这个文件单独列出。<br>**A1.4.3** ✅：58 个 fixture。<br>**A1.4.4** ✅：参数一致性已有测试（A0.5.2）。fork 的 `mcxx.yml` 用新 fixture `mcxx-warm` 做冷启动加热启动（同一个 workspace 和 cache，`--expect-warm`）：热启动时 std、程序的接口和分区的模块文件都是冷启动留下的，没有重建也没有新增，也就是 `Built module = 0`；本机导航 2.9 s 降到 0.5 s |
| M1.8 替换 mcppls 的 native 引擎 | ✅ | **A1.8.1**：mcppls 的 native 引擎改用 `mcxx.frontend`（fork 的 PR）：<br>- 模块扫描（`scan_source`）的词法换成 F1，语义不变，各分支照报、标注 conditional；<br>- 文档大纲是模块符号加上 F1 的大纲，不再等 Clang 解析，mcxx 引擎不再应答；<br>- 工作区符号来自每个文件的 F1 大纲。<br>**A1.8.2**：fork 的单元测试全部通过，58 个 Linux fixture 全部通过（fork 提交 25dc86a，见 fork PR）。<br>**A1.8.3**：`msa::Workspace::quick`，Clang 后端用 F1 实现：每次修改立刻发布 F1 能判定的门禁发现（不带 version，所以不会被当成这个版本的解析结果），解析完再替换。`mcxx serve` 端到端：goto、new/delete、reinterpret_cast、写出的 `*` 声明符、宏五类发现在修改后 1–4 ms 到达。F1 的事实（参数、局部变量、指针与数组标志、goto、new/delete、具名转换、throw/try/typeid、asm、`[[mcpp::allow]]`）经 `mcxx-conformance --frontend` 检查，13 个特性的发现和 fixture 完全一致，没有误报 |
| M1.1 生成 IFC（MC2 v1） | ✅ | `modules/ifc`（`mcxx.ifc`）加上 Clang 后端的 `:ifc` 分区：模块单元编译出 BMI（`--precompile` 或 `-fmodule-output`）且没有错误时，在门禁之后把 `X.ifc` 写在 `X.pcm` 旁边。格式是 IFC 0.43，结构体直接用 SDK 的定义：T1 声明是真正的 IFC 声明（作用域、函数、字段、变量、别名、枚举，参数在函数的 chart 里），IFC 没有字段可放的 MC3 信息写在 `[[mcxx::decl(...)]]` 属性里，方言是全局作用域里的属性声明。读取不用 SDK 的 reader（坏文件会让它结束进程），每个偏移都检查。<br>**A1.1.1**：C-mcppls 用 mcxx 构建后，111/111 个接口单元都有 `.ifc`（依据 `build.ninja` 的 `bmi_out`）；关掉 mcpp 的包缓存时，构建里 134 个 BMI 除了 `std`（来自 mcpp 的 std 缓存）都有 `.ifc`。<br>**A1.1.2**：`ifc-printer`（从 SDK 源码构建，`tools/checks/ifc.py`）读完全部 133 个 `.ifc`，零错误；`.ifc` 共 3.0 MB，同一构建的 BMI 80 MB。<br>**A1.1.3**：`mcxx-probe --ifc`：111 个接口单元、4109 个声明，读回和解析结果逐项逐字段相等。这项检查发现并修复了一个事实缺陷：无名类/联合的限定名和类型带着命令行写法的文件路径，编辑器和构建得到不同的名字（MC3-4-4）。<br>**A1.1.4**：fixture `conformance/ifc/dialect`（profile、包级别、模块级别、命名空间级别）：每个模块单元读回的方言等于期望，并列出目录的全部 19 个特性。<br>**A1.1.5**：`specs/mc2-ifc.md` 1.0.0：锁定 IFC 0.43、编码表、读取要求；schema `mc2-interface.schema.json` 和真实输出的示例；`specs.py` 另外核对签名、版本、SHA-256 内容哈希、同样的接口得到同样的字节（发现并修复了结构体填充字节未清零）、编辑器解析和 `mcxx check` 不写 `.ifc`。均已进入 CI。<br>依赖包和 `std` 的 BMI 由 mcpp 从构建缓存复制进项目时旁边没有 `.ifc`：M1.2 起编译成功后接口还按 BMI 内容的 SHA-256 存进 store，导入方从那里读到 |
| M1.2 跨模块的方言边界 | ✅ | MC3 0.4.0 的 `imports` 事实：文件里每个具名模块导入，按编译加载的 BMI 找到对方的 `.ifc`（旁边，或 store 里按 BMI 内容保存的副本），顺着 MC2 1.1.0 的 `reexport` 找下去，给出每个模块的方言和导出的 T1 声明。SDK 的 `plugin::report_imports`：特性的规则在导入处报出越过方言边界的东西——对方导出了、而对方自己的方言并不禁止的；读不到接口时报"无法得知"。`c-array`、`union`、`c-varargs`、`raw-pointers`、`lib:std.vector` 已接上。发现属于那个特性，级别是导入方的。<br>豁免写在导入上 `import legacy [[mcpp::allow("c-array", "原因")]];`（Clang 不接受导入上的属性，mcxx 在 Clang 读文件之前把它换成空格，位置不变；libmc++ 的模块扫描和 BMI 构建同样处理），或者写在 manifest 的 `[package.metadata.mcxx.imports."legacy"]`（MC1 0.3.0）：mcpp 自己的依赖扫描不认导入上的属性，用 mcpp 构建时用这种写法。两种都进审计。<br>**A1.2.1**：`conformance/boundary`（legacy 包没有方言，app 包 safe 加 `raw-pointers = "deny"`），`tools/checks/boundary.py` 用 mcxx 编译：未标注的导入报出四个特性（联合体来自再导出的分区），部分标注的报其余三个，全部标注的通过并有四条审计，导入同包 safe 模块的什么都不报；manifest 形式的 `app-manifest` 由 mcpp 以 mcxx 为工具链构建通过，审计里有四条。<br>**A1.2.2**：删掉 legacy 包的源码后重新编译，发现和审计完全相同；再删掉 BMI 旁边的 `.ifc`，从 store 读到，结果不变；store 也空时四个特性都报"无法得知"。<br>另外：编辑器路径（Clang 后端单元测试）同样报出和豁免；`std` 经 mcpp 的 std 缓存复制进项目后，接口从 store 按内容找到（safe 包 `import std;` 构建通过，清空 store 后报 3 个"无法得知"）。快速门禁和 `mcxx-conformance --frontend` 不把 `imports` 算作 F1 需要的事实，13 个特性照旧由 F1 判定。均已进入 CI |
| M1.5 对非 mcxx 工具链的强制执行 | 🟡 | `plugins/mcpp-tools-safe`（模块 `mcxx.check`）：mcpp 的构建规则包，把 `mcxx check -p <编译数据库>` 作为 blocking 的 check 动作；新增的 `mcxx check -p` 由语义后端按 GCC 的编译命令解析，模块接口自己构建进 cache。`tools/checks/tools_safe.py`：用 GCC 16 构建 consumer，**A1.5.1** 干净构建通过、stamp 写出；**A1.5.3** 不改动再构建什么都不跑；**A1.5.2** 加一个 goto，构建在检查处失败，去掉后再次通过。已进入 CI。对 C-mcpp 本身（`--corpus`）的一次运行因本机磁盘写满中断，待重跑；E-IDX-5（本仓库 index 里的包条目）待发布 tag 后补上 |

M0.x、M1.x、M2.x、MS 各项按里程碑文档的编号逐项补到这里。

## 注意事项

- mcpp 2026.9.28.3（2026-09-29 自动升级）："workspace 成员只构建一次"。升级后第一次构建出现过一次找不到 `mcppls.os` 的模块文件，重跑后没有再现。
- 只有 workspace 根的 `[indices]` 生效：通过路径依赖使用 libmc++ 的一方也必须声明 `llvm` 这个 index。
