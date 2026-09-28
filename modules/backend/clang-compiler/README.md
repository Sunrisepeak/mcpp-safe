# backend/clang-compiler：进程内的 clang（`mcxx.backend.clang.compiler`）

libmc++ 两个可以出现 Clang 的包之一。它把 clang 23.1 本身放进 mcxx：driver、cc1、cc1as 都在进程内运行（`llvm.clang-driver`，包含 `llvm.codegen-dev` 的 x86-64 和 AArch64 后端），链接这一步按 clang 的做法启动外部的 `ld.lld`。

- `run(argc, argv)`：clang 本身；`argv[0]` 表示模式。
- `run_as(mode, self, args)`：`mode` 为 `clang++` 或 `clang`；`self` 是程序自己的路径，clang 据此找到旁边的工具和资源目录，需要时用 `-cc1` 重新调用自己。
- 目标注册在这里完成（`LLVMInitializeX86*`、`LLVMInitializeAArch64*`）：各个包共用的 `llvm/Config/Targets.def` 是空的。

它和 `../clang` 分成两个包，因为 mcpp 会把一个包的全部目标文件链接进使用方：mcppls 只链接语义那一半。
