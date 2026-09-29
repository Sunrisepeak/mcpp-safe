# backend/clang：基于 Clang 23.1 的 MSA（`mcxx.backend.clang`）

libmc++ 两个可以出现 Clang 的包之一（见 `../README.md` 的边界规则）。它实现 MSA，并把 MC++ 的插件挂进 Clang。

## 分区

| 文件 | 分区 | 内容 |
|---|---|---|
| `clang.cppm` | 接口 | `make_workspace()`、`info()`、`version()`，接口里只有 `msa::` 类型 |
| `support.cppm`、`support.cpp` | `:support` | 在 16 MiB 的独立栈上运行 Clang（openkal 的线程栈固定为 256 KiB）、`ClangPool`、命令参数规范化、共享常量 |
| `store.cppm`、`store.cpp` | `:store` | 模块接口（BMI）：按依赖顺序构建，按内容缓存；编辑器缓冲覆盖；失败根因 |
| `unit.cppm`、`unit.cpp` | `:unit` | 解析一个文件：诊断、出现位置、符号、实体 |
| `facts.cppm`、`facts.cpp` | `:facts` | MC3 事实（`imports` 按编译加载的 BMI 读对方的 `.ifc`，顺着再导出）：遍历主文件 AST（声明、初始化和未初始化的局部变量、强制转换、new/delete、指针运算、goto、throw/try/typeid/asm/va_arg、`[[mcpp::allow]]`），宏和 `#include` 来自预处理器和源码管理器。只收集要求的种类（`fact::Kinds`） |
| `gate.cppm`、`gate.cpp` | `:gate` | `[[mcpp::allow]]` 属性；Clang 静态插件：解析前运行源码过滤器，解析后运行门禁规则；`quick_gates`（MC++ 自己的前端先给出的门禁结果） |
| `diagnostics.cppm`、`diagnostics.cpp` | `:diagnostics`（内部） | MC5 §9 的诊断视角：human 或 agent 时换掉 Clang 的诊断 consumer，把 Clang 的诊断（连同它的 note、fix-it）和门禁的发现（记下的级别、出处、修改建议、豁免写法）交给 `mcxx.diagnostics` 排版；clang 视角时不动 |
| `ifc.cppm`、`ifc.cpp` | `:ifc` | MC2：模块单元写出 BMI 且没有错误时，在 BMI 旁边写 `.ifc`（T1 声明、方言、再导出，`mcxx.ifc`），并登记给 store；libmc++ 自己构建 BMI 时也写（`InterfaceAction`）；导入上的 `[[mcpp::allow]]` 在 Clang 读文件前换成空格，变成对这个导入的豁免 |
| `completion.cppm`、`completion.cpp` | `:completion` | 代码补全、签名帮助 |
| `index.cppm`、`index.cpp` | `:index` | 后台构建的程序索引 |
| `workspace.cpp` | 实现单元 | `msa::Workspace` 的实现 |

每个分区的 `.cppm` 只有声明（和类型），定义在同名的 `.cpp` 实现单元里（MC5 §8）：改一个定义只重编它自己的 `.cpp`，导入这个分区的单元不受影响。唯一的例外是 `ModuleStore` 的构造和析构留在 `store.cppm`：Clang 22 不会在导入方生成 libc++ `std::stop_source` 内部的一个静态 constexpr 成员（经由分区的全局模块片段到达），分区自己的目标文件里用到 `stop_` 才生成它。

拆成分区是为了增量构建：改 `workspace.cpp` 只重编它自己（约 20 s）。

## 插件怎样进入 Clang

`:gate` 注册了一个 `AddAfterMainAction` 类型的 Clang 插件。它会进入这个程序运行的每一次编译，包括 mcxx 的编译和编辑器里的解析（ASTUnit）：
1. `CreateASTConsumer`：此时主文件已经确定、还没有被读取。用目标三元组和 `-D` 的定义构造 `plugin::Target`，运行全部源码过滤器，再用 `overrideFileContents` 替换主文件内容。
2. `HandleTranslationUnit`：依次完成以下几步：
   - 取这个包的 Plan（`features::plan_for`，有缓存）；
   - 用名字查找去掉这个文件里不可能出现的特性（`requires_declaration`）；
   - 只抽取剩下的特性需要的事实，运行门禁（`mcxx.features`）；
   - 把结果作为 Clang 诊断报出，写审计记录（`MCXX_AUDIT`）；
   - 如果这次编译写出一个模块单元的 BMI（`--precompile` 的输出或 `-fmodule-output`），而且到这里没有错误，就在 BMI 旁边写 `.ifc`（`:ifc`，MC2）：T1 声明另外收集一次（声明和类型），方言来自同一个 Plan。内容没变时不重写。

   什么都不用问时直接返回（计数 `gates.idle`），不遍历 AST。`MCXX_LOG=gates=debug` 分别给出 `gates.facts` 和 `gates.rules` 两段的耗时。

构建模块接口和后台索引时，门禁不运行（线程局部的 `gates_suppressed`）：门禁错误是这个文件自己的诊断，不应该让依赖它的模块都构建失败。源码过滤器在任何编译中都运行。

一个已知的 Clang 23 问题：解析器只用不带作用域的名字（`allow`）向插件询问带作用域的属性（`mcpp::allow`），询问不到就会丢掉参数。所以 `allow` 也登记为拼写之一。

## 测试

`test_arguments`：一个单元的命令无论以哪种形式进入，得到的参数都相同（MC5-6-2）。`mcpp test -p modules/backend/clang`（构建约 45 s，运行不到 1 s）：在临时目录生成不用标准库的模块程序，覆盖解析、实体、跨模块跳转、失败根因、缓冲区覆盖、诊断 code、`json-brace-init`、mc++.iso 在 profile strict 下的全部事实（加上插件 `raw-pointers` 的 warn 级别），以及 `[[mcpp::cfg]]`。
