# conformance：一致性测试

## gates：门禁 fixture（M0.7 A0.7.2、A0.7.3）

`gates/` 下每个含源文件的目录是一个程序（其中的文件一起编译，模块接口按需构建），使用离它最近的 `mcpp.toml` 里的 `[package.metadata.mcxx]`：`safe/` 用 `profile = "safe"`，`libs/` 用各规则的默认级别。

- 应当报告的行，在行尾写 `// expect: <特性 id> [<特性 id> ...] [-- 说明]`。
- 每一条门禁诊断都必须有对应的期望，每一个期望也都必须被报告，而且在同一行。
- fixture 本身必须能编译：门禁以外的错误算作 fixture 有问题。
- 不用标准库（需要的类型在文件里自己给出最小定义），所以不依赖工具链。

```
mcpp build -p tools/conformance
<target>/bin/mcxx-conformance [--json 报告.json] conformance/gates
```

输出每个特性的 found、missed、wrong，以及精确率和召回率；有任何不符时退出码为 1。当前：9 个特性（mc++.safe 的 8 个，加上 json-brace-init），10 个程序、20 个文件，精确率和召回率都是 100%。每个特性至少有 5 个正例和 5 个反例。反例尽量取"差一点就是"的写法，例如类类型迭代器上的算术、经由 `void*` 的转换、`#if 0` 里的宏、`mine::vector`。

`safe/cross-module/` 是 A0.7.3 的两个跨模块用例：导入方的 `auto v = make()` 推导出指针，其上的算术要被捕获；导出别名 `using Buf = int[16]` 在导入方声明的变量要被捕获为 C 数组。
