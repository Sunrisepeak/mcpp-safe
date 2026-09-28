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
| M0.5 驱动 | 🟡 | 驱动在 `modules/driver`：<br>- 命令：`c++`、`cc`、`check`、`features`、`compose`、`version --json`；<br>- 按名字调用：clang++、clang、c++、cc、g++、gcc。<br>**A0.5.3** ✅ 自举：C-mcppls（377d222）用 mcxx 作为 mcpp 工具链构建，31 个测试程序全部通过（`tools/checks/selfhost.py`，干净检出时构建 37 s，mcxx 为未优化构建）。MC++ 默认开启的 json-brace-init 在语料中发现两处真实问题：`Json result { nullptr }` 得到 `[null]`，`const Json clientId { … }` 得到 `[clientId]`，测量时设为 warn。<br>**A0.5.4** ✅ MC5 0.1.0。<br>A0.5.1（与 clang 23.1 `-fsyntax-only` 的诊断集合对比）和 A0.5.2（参数规范化 fixture）未做 |
| M0.8 规范 | 🟡 | A0.8.1：`specs/` 下 MC1、MC3、MC4、MC5 都有正文、schema、示例和要求 id。`tools/checks/specs.py` 通过：316 项检查，100 条要求中 97 条有证据，其余 3 条列在 `$pending` 里，等待 A0.5.1、A0.5.2 和 V0.6。已经进入 CI。<br>这项检查发现并修复了一个缺陷：先 `--precompile`、再用 `-c` 编译 `.pcm` 时，门禁会报告两次。A0.8.2、A0.8.3 还有部分未完成 |
| M0.3 门禁 | 🟡 | 以下各项已实现，并由 `modules/features/tests` 覆盖：<br>- 配置：`profile` 可以是一个或多个；包、模块、命名空间的级别；未知的 id 和 profile 报警告；<br>- 优先级；<br>- 豁免与审计（`MCXX_AUDIT`）。<br>A0.3.1（MC1 草案，schema 与示例校验）✅；A0.3.2（`test_spec`：`mcpp.toml` 示例逐项等于规范的 JSON）✅ |
| M0.6 插件 | 🟡 | **A0.6.1** ✅ `mcxx compose`（`modules/driver`）：<br>- 第一次组合约 5 分钟，不变时约 6 s 且不重新链接；<br>- 编译时自动交给组合好的编译器；<br>- 缺少组合时报错；<br>- 端到端检查：`tools/checks/compose.py`，已进入 CI。<br>**A0.6.2** ✅ 进程外协议 1：`modules/plugin/{wire,remote,host}`。示例插件 `plugins/examples/naming-remote` 用 GCC 16 + libstdc++ 构建，经由进程外协议对同一个文件给出的诊断，和把 `plugins/examples/naming` 静态组合进来的 mcxx 完全相同（`tools/checks/compose.py --remote`，已进入 CI）。为此，只依赖 std 的包（`msa`、`plugin/{sdk,wire,remote}`）不再声明 openkal-llvm-runtime，因为这个运行时只允许 LLVM 编译器，由最终的程序来声明。<br>**A0.6.3** ✅ 崩溃、超时、乱码、协议不符都报告为 `mcxx-plugin`，编译器不崩溃、不卡住（`test_host` 故障注入）。<br>**A0.6.4** ✅ MC4 0.1.0 |
| M0.7 mc++.safe | 🟡 | **A0.7.1**：`mc++.iso` 共 15 个 ISO 特性，都带 stable name；profile `safe` 覆盖 10 个未定义行为来源；插件样例 `raw-pointers`、`lib:std.vector`、`ext:cfg`。<br>**A0.7.2**：19 个特性、20 个程序、41 个文件，精确率和召回率都是 100%（`mcxx-conformance`）。<br>**A0.7.3**：跨模块用例。<br>**A0.7.4**：`test_override`。<br>**A0.7.5**：诊断带 id、改法和豁免方法。<br>还差 MC1 的示例文件 |

M0.x、M1.x、M2.x、MS 各项按里程碑文档的编号逐项补到这里。

## 注意事项

- mcpp 2026.9.28.3（2026-09-29 自动升级）："workspace 成员只构建一次"。升级后第一次构建出现过一次找不到 `mcppls.os` 的模块文件，重跑后没有再现。
- 只有 workspace 根的 `[indices]` 生效：通过路径依赖使用 libmc++ 的一方也必须声明 `llvm` 这个 index。
