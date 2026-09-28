# 进度（按里程碑文档逐项跟踪）

每个验证项、验收项、适配项都列在这里，写明当前状态和证据。状态有四种：✅ 达成，🟡 部分达成，⏳ 进行中，⬜ 未开始。
平台默认 linux-x64（本机 32 核，按"本地优先、CI 辅助"的规则执行）。

## 验证项

| # | 状态 | 结论与证据 |
|---|---|---|
| V0.1 | ✅ | Clang/LLVM 23.1 的前端库由 mcpp 在 openkal 上构建（speak-agent/llvm-clang-dev，tag 23.1.0），smoke 程序在本机和 CI 上都能运行 |
| V0.2 | ⬜ | 等 V0.4 完成后测量 |
| V0.3 | ✅ | IFC SDK 0.43.5 加上 GSL 4.2.0 放在 `index/microsoft`，在 openkal 上用 mcpp 构建通过；`ifc-printer` 能运行（用真实 `.ifc` 文件的验证在 M1.1） |
| V0.4 | ⬜ | |
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
| E-LS-2 | ✅ | fork 的 PR #1 |
| E-LS-5 | 🟡 | linux 达成（见检查点 1） |

## 里程碑

M0.x、M1.x、M2.x、MS 各项按里程碑文档的编号逐项补到这里。

## 注意事项

- mcpp 2026.9.28.3（2026-09-29 自动升级）："workspace 成员只构建一次"。升级后第一次构建出现过一次找不到 `mcppls.os` 的模块文件，重跑后没有再现。
- 只有 workspace 根的 `[indices]` 生效：通过路径依赖使用 libmc++ 的一方也必须声明 `llvm` 这个 index。
