# boundary：跨模块的方言边界（M1.2）

三个包：

- `legacy/`：没有 profile 的库（`src/`）。它的接口导出裸指针、C 数组、C 可变参数函数，并通过 `export import :detail` 再导出一个联合体。
- `app/`：`profile = "safe"`，另外 `raw-pointers = "deny"`。在导入处用 `[[mcpp::allow]]` 标注。
- `app-manifest/`：同样的方言，标注写在 manifest 里（`[package.metadata.mcxx.imports."legacy"]`）。mcpp 自己的依赖扫描不接受导入声明上的属性，用 mcpp 构建时写在这里。

`app/` 的每个文件导入别的模块，行尾的 `// expect: <特性 id>...` 是导入处必须报出的发现；`// expect-waived: <特性 id>...` 是被导入处的 `[[mcpp::allow]]` 豁免、必须出现在审计里的发现（一行里两种都有时用 `;` 隔开）：

| 文件 | 导入 | 期望 |
|---|---|---|
| `plain.cpp` | `import legacy;` | 四个特性都在导入处报错（联合体来自再导出的 `legacy:detail`） |
| `partial.cpp` | 只豁免 `c-array` | 其余三个照报 |
| `waived.cpp` | 四个都豁免，带理由 | 没有错误，审计里有四条 |
| `trusts.cpp` | 导入同一个包里的 safe 模块 `app.table` | 什么都不报：它自己的方言禁止 C 数组，它导出的那一个是它自己豁免过的 |
| `app-manifest/src/main.cpp` | `import legacy;`，manifest 里允许四个特性 | 构建通过，审计里有四条，声明写作 `import legacy (mcpp.toml)` |

`tools/checks/boundary.py` 用 mcxx 编译（先构建 BMI 和 `.ifc`，再编译导入方），然后：

- **A1.2.1**：每个文件的发现等于期望；
- **A1.2.2**：删掉 `legacy/` 的源码后重新编译导入方，结果完全相同——判断只依赖对方的 `.ifc`；
- 删掉 BMI 旁边的 `.ifc`，从 store（按 BMI 内容保存的副本）照样读到；store 里也没有时，四个特性都报"无法得知"；
- 带 `--toolchain`（mcxx 作为 mcpp 的工具链）时，用 mcpp 构建 `app-manifest`。
