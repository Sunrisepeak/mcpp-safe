# conformance：一致性测试

- `gates/`：门禁 fixture（下文）。
- `ifc/dialect/`：MC2 的方言 fixture（A1.1.4）：一个包设了 profile、包级别、模块级别和命名空间级别；每个模块单元的 `.ifc` 读回的方言必须等于 `expected.json`，并且列出目录里的每一个特性。由 `tools/checks/ifc.py` 编译和检查。
- `traceability.json`：`specs/` 中每条规范要求（`MC<n>-<节>-<序号>`）对应的证据，由 `tools/checks/specs.py` 检查（见 `specs/README.md`）。

## gates：门禁 fixture（M0.7 A0.7.2、A0.7.3）

`gates/` 下每个含源文件的目录是一个程序（其中的文件一起编译，模块接口按需构建），使用离它最近的 `mcpp.toml` 里的 `[package.metadata.mcxx]`：

| 目录 | 配置 | 测的是 |
|---|---|---|
| `safe/` | `profile = "safe"` | mc++.iso 中的未定义行为来源：`raw-pointer-arithmetic`、`new-delete`、`reinterpret-cast`、`c-style-cast`、`const-cast`、`c-array`、`union`、`c-varargs`、`uninitialized`、`asm`，以及跨模块用例 |
| `strict/` | `profile = "strict"` | `goto`、`macros`、`include`（包括模块 purview 里的 `#include`，全局模块片段里的不报） |
| `embedded/` | 包设置 `exceptions`、`rtti` 为 deny | 不在任何 profile 里的 ISO 特性 |
| `policy/` | 包设置 `raw-pointers = "deny"` | 插件的 policy：能不能用裸指针 |
| `library/` | 包设置 `"lib:std.vector" = "deny"` | 库控制 |
| `portable/` | `profile = "portable"` | extension 类别整体禁止：`ext:cfg`（`[[mcpp::cfg]]` 的每一处使用，包括对当前目标被清除的） |
| `libs/` | 各规则的默认级别 | `json-brace-init` |
| `scopes/` | `strict`，外加包、模块、命名空间各自的设置 | MC1 §6 的每一层优先级（A0.3.3）：<br>- 包覆盖 profile；<br>- 模块覆盖包，分区沿用模块的设置，别的模块不受影响；<br>- 命名空间覆盖模块，内层命名空间覆盖外层；<br>- 声明上的豁免（`// expect-waived:`，要出现在审计里，A0.3.4） |

- 应当报告的行，在行尾写 `// expect: <特性 id> [<特性 id> ...] [-- 说明]`。被声明上的 `[[mcpp::allow]]` 豁免的行写 `// expect-waived: <特性 id>`：runner 设置 `MCXX_AUDIT`，每一条这样的期望都必须出现在审计记录里，审计里也不能有没被期望的豁免。
- `--json` 的报告带每个程序的耗时和总耗时（A0.8.2）。
- `--driver <mcxx>`（A1.4.2）：每个程序再按构建的方式用 mcxx 编译一遍，即先预编译模块接口，再检查其他单元。构建和编辑器报出的门禁发现必须是同一个集合。有的文件构建时根本不会被编译，因为它导入的接口有门禁错误，接口没有写出；这样的文件会列出来，不参与比较。
- 每一条门禁诊断都必须有对应的期望，每一个期望也都必须被报告，而且在同一行。
- fixture 本身必须能编译：门禁以外的错误算作 fixture 有问题。
- 不用标准库（需要的类型在文件里自己给出最小定义），所以不依赖工具链。

```
mcpp build -p tools/conformance
<target>/bin/mcxx-conformance [--json 报告.json] conformance/gates
```

输出每个特性的 found、missed、wrong，以及精确率和召回率；有任何不符时退出码为 1。

当前规模：
- 19 个特性：mc++.iso 15 个，插件 4 个（`raw-pointers`、`lib:std.vector`、`ext:cfg`、`json-brace-init`）；
- 20 个程序、41 个文件；
- 精确率和召回率都是 100%；
- 每个特性至少有 5 个正例和 5 个反例。

反例尽量取"差一点就是"的写法，例如：
- 类类型迭代器上的算术；
- 经由 `void*` 的转换、函数式写法构造一个类（`Meters(v)`）；
- `#if 0` 里的宏或 `#include`；
- `mine::vector`；
- 有默认成员初始化的类、static 局部变量；
- 模板参数包、`catch (...)`；
- 引用、`nullptr_t`、`sizeof(int*)`。

`safe/cross-module/` 是 A0.7.3 的两个跨模块用例：导入方的 `auto v = make()` 推导出指针，其上的算术要被捕获；导出别名 `using Buf = int[16]` 在导入方声明的变量要被捕获为 C 数组。
