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
| V0.6 | ⬜ | |
| V0.7 | 🟡 | mcpp 这一半已达成：`[indices] llvm = { path = "index/llvm" }` 能解析并构建 `llvm.clang-dev`，mcpp-safe 和 fork 都通过它构建。xlings 这一半（`xpkgs/`）还没做 |

## 适配项

| # | 状态 | 说明 |
|---|---|---|
| E-OK-1 | ✅ | 缺 `pread` 和文件 mmap，在构建配方中绕开（`HAVE_PREAD 0`） |
| E-OK-2 | ✅ | 见 V0.5 |
| E-IDX-1 | ✅ | `index/llvm/pkgs/c/clang-dev.lua` |
| E-IDX-2 | ✅ | `index/microsoft/pkgs/{g/gsl,i/ifc-sdk}.lua` |
| （新增） | ✅ | `index/llvm` 中的 `llvm.codegen-dev`、`llvm.clang-driver`，版本 23.1.0.2 |
| E-LS-2 | ✅ | fork 的 PR #1 |
| E-LS-5 | 🟡 | linux 达成（见检查点 1） |

## 里程碑

| # | 状态 | 说明 |
|---|---|---|
| M0.5 驱动 | ⏳ | 根包 `mcxx`：`mcxx c++/cc`（也可以用 clang++、clang、c++、cc 这些名字调用）以及 `mcxx check`；V0.6 的工具链 payload 还在进行 |
| M0.3 门禁 | 🟡 | 以下各项已实现，并由 `modules/features/tests` 覆盖：<br>- 配置：`profile` 可以是一个或多个；包、模块、命名空间的级别；未知的 id 和 profile 报警告；<br>- 优先级；<br>- 豁免与审计（`MCXX_AUDIT`）。<br>A0.3.1 的 MC1 草案（schema）未做 |
| M0.6 插件 | 🟡 | SDK v0 已实现：<br>- 扩展点：规则、源码过滤器、profile；<br>- 覆盖：特性、provider、profile；<br>- `mcxx features`。<br>`mcxx compose`（A0.6.1）、进程外协议（A0.6.2）和故障隔离（A0.6.3）未做 |
| M0.7 mc++.safe | 🟡 | **A0.7.1**：`mc++.iso` 共 15 个 ISO 特性，都带 stable name；profile `safe` 覆盖 10 个未定义行为来源；插件样例 `raw-pointers`、`lib:std.vector`、`ext:cfg`。<br>**A0.7.2**：19 个特性、20 个程序、41 个文件，精确率和召回率都是 100%（`mcxx-conformance`）。<br>**A0.7.3**：跨模块用例。<br>**A0.7.4**：`test_override`。<br>**A0.7.5**：诊断带 id、改法和豁免方法。<br>还差 MC1 的示例文件 |

M0.x、M1.x、M2.x、MS 各项按里程碑文档的编号逐项补到这里。

## 注意事项

- mcpp 2026.9.28.3（2026-09-29 自动升级）："workspace 成员只构建一次"。升级后第一次构建出现过一次找不到 `mcppls.os` 的模块文件，重跑后没有再现。
- 只有 workspace 根的 `[indices]` 生效：通过路径依赖使用 libmc++ 的一方也必须声明 `llvm` 这个 index。
