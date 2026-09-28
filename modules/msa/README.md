# msa：MC++ 语义 API（`mcxx.msa`，MC3）

编译器前端对一个 C++ 模块程序"知道什么"，全部用值类型表达，不涉及任何编译器自己的类型。libmc++ 之上的一切（LSP 服务、门禁、插件、mcppls）都只针对它编写；`mcxx.backend.clang` 用 Clang 实现它，MC++ 自研前端以后也实现它（计划 P1）。

- **位置**：行从 0 开始，列是 UTF-8 字节；LSP 层负责换算成 UTF-16。
- **程序**：`Workspace`（编译命令、模块、索引、`parse`、`complete`、`definitions`……）和 `Unit`（一个文件某个版本的解析快照，返回后不可变）。
- **实体与出现位置**：`Entity`、`Occurrence`、`Symbol`、`Diagnostic`、`CompletionItem`、`SignatureHelp`。
- **事实（`msa::fact`，MC3 v0）**：一个文件自己的代码"声明了什么、做了什么"，是门禁和插件规则的输入。T1 为 `Declaration`（包括声明类型用到的类模板，例如 `std::vector`）；T2 为 `Initialization`（形式、选中的构造函数、列表元素）、`Cast`、`Allocation`、`PointerArithmetic`、`Goto`、`MacroDefinition`；另外还有 `Suppression`（`[[mcpp::allow]]`）。每一份事实都带 `Certainty`。
- **后端信息**：`BackendInfo`（名字、版本、semantic kit 应当使用的 libc++ 版本）。

只依赖 std。它是整个 libmc++ 的共同语言，改动要谨慎：`Unit` 和 `Workspace` 上的每个虚函数，后端和测试里的假实现都要实现。
