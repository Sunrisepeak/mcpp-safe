# 进度（按里程碑文档逐项跟踪）

每个验证项、验收项、适配项都列在这里，写明当前状态和证据。状态有四种：✅ 达成，🟡 部分达成，⏳ 进行中，⬜ 未开始。
平台默认 linux-x64（本机 32 核，按"本地优先、CI 辅助"的规则执行）。

## 验证项

| # | 状态 | 结论与证据 |
|---|---|---|
| V0.1 | ✅ | Clang/LLVM 23.1 的前端库由 mcpp 在 openkal 上构建（speak-agent/llvm-clang-dev，tag 23.1.0），smoke 程序在本机和 CI 上都能运行 |
| V0.2 | 🟡 | mcxx（dev 构建，内含 Clang、CodeGen、x86-64 和 AArch64 后端）154 MB；改动驱动后重新编译加链接 5.25 s（本机 32 核，门槛 ≤ 60 s）。4 核 CI 上的数字和 release 体积待补 |
| V0.3 | ✅ | IFC SDK 0.43.5 加上 GSL 4.2.0 放在 `index/microsoft`，在 openkal 上用 mcpp 构建通过；`ifc-printer` 能运行（用真实 `.ifc` 文件的验证在 M1.1） |
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
| M1.4 接入 mcppls | 🟡 | **A1.4.1** ✅：fork 的 mcxx 引擎是默认引擎。<br>**A1.4.2** ✅：编辑器（workspace 解析）和构建（`mcxx c++`：预编译接口、检查单元）对门禁 fixture 的 117 条发现完全相同（`mcxx-conformance --driver`，已进入 CI）。有 1 个文件构建时不会被编译，因为它导入的接口有门禁错误，这个文件单独列出。<br>**A1.4.3** ✅：58 个 fixture。<br>**A1.4.4** 🟡：参数一致性已有测试（A0.5.2），热启动 `Built module = 0` 的回归检查还没有进 fork 的 CI |

M0.x、M1.x、M2.x、MS 各项按里程碑文档的编号逐项补到这里。

## 注意事项

- mcpp 2026.9.28.3（2026-09-29 自动升级）："workspace 成员只构建一次"。升级后第一次构建出现过一次找不到 `mcppls.os` 的模块文件，重跑后没有再现。
- 只有 workspace 根的 `[indices]` 生效：通过路径依赖使用 libmc++ 的一方也必须声明 `llvm` 这个 index。
