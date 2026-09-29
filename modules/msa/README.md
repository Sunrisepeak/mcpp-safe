# msa：MC++ 语义 API（`mcxx.msa`，MC3）

编译器前端对一个 C++ 模块程序"知道什么"，全部用值类型表达，不涉及任何编译器自己的类型。libmc++ 之上的一切（LSP 服务、门禁、插件、mcppls）都只针对它编写；`mcxx.backend.clang` 用 Clang 实现它，MC++ 自研前端以后也实现它（计划 P1）。

- **位置**：行从 0 开始，列是 UTF-8 字节；LSP 层负责换算成 UTF-16。
- **程序**：`Workspace`（编译命令、模块、索引、`parse`、`complete`、`definitions`……）和 `Unit`（一个文件某个版本的解析快照，返回后不可变）。
- **实体与出现位置**：`Entity`、`Occurrence`、`Symbol`、`Diagnostic`、`CompletionItem`、`SignatureHelp`。
- **事实（`msa::fact`，MC3 v0）**：一个文件自己的代码"声明了什么、做了什么"，是门禁和插件规则的输入。事实分三组：
  - T1：`Declaration`。包括声明类型用到的类模板（例如 `std::vector`）、是否含裸指针、是否 C 数组、联合体、C 可变参数。
  - T2：
    - `Initialization`：形式、选中的构造函数、列表元素，以及值是否不确定（`indeterminate`）；
    - `Cast`、`Allocation`、`PointerArithmetic`、`Goto`、`MacroDefinition`；
    - `Use`：`throw`、`try`、`typeid`、`asm`、`va_arg` 等语言构造的使用；
    - `Include`：是否在全局模块片段里。
  - 另外还有 `Suppression`（`[[mcpp::allow]]`）。

  每一份事实都带 `Certainty`。`fact::Kinds` 是事实种类的集合：一个特性声明它依赖哪几种（`Feature::needs`），后端只收集被要求的那几种（`Facts::collected`）。声明的类型文本单独作为一种（`declaration_types`），因为它最花时间。
- **后端信息**：`BackendInfo`（名字、版本、semantic kit 应当使用的 libc++ 版本）。

只依赖 std。它是整个 libmc++ 的共同语言，改动要谨慎：`Unit` 和 `Workspace` 上的每个虚函数，后端和测试里的假实现都要实现。

## 文件

接口单元只声明，定义在实现单元里（MC5 §8）。

| 文件 | 内容 |
|---|---|
| `src/msa.cppm` | 主接口：只 `export import` 下面四个分区 |
| `src/basics.cppm` | `:basics`：位置、范围、Location，certainty，诊断，实体的种类，occurrence 的角色 |
| `src/entities.cppm` | `:entities`：服务回答用的值——实体、occurrence、符号、补全、签名，以及 workspace 报告自己的（后端、命令、失败、状态） |
| `src/facts.cppm` | `:facts`：MC3 的事实（`specs/mc3-facts.md`） |
| `src/service.cppm` | `:service`：后端实现的接口，`Unit` 和 `Workspace` |
