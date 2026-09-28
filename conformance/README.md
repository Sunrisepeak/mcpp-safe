# conformance：一致性测试

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

- 应当报告的行，在行尾写 `// expect: <特性 id> [<特性 id> ...] [-- 说明]`。
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
