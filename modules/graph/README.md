# graph：模块事实与模块图（`mcxx.graph`）

不经过编译器，按词法从源文件里读出模块信息，再建立整个程序的模块图。

- `scan(text)` → `Scan`：模块名（`m`、`m:p`）、是否导出、有没有全局模块片段、导入列表，以及每个导入和模块声明在文本中的位置（`import_spans`、`module_span`）。它会跳过注释、字符串、原始字符串、花括号内部和预处理行；`import m [[...]];` 这样带属性的导入仍然算导入（M1.2）。
- `Graph`：`provider(module)`、`requires_of`、`closure(roots, &missing)`（依赖在前）、`modules()`、`cycles()`。

`mcxx.backend.clang` 用它确定构建模块接口的顺序，以及诊断该落在哪一个 import 上。测试：`mcpp test -p modules/graph`。
