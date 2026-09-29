# features：特性门禁，以及 MC++ 内置的 ISO 特性控制（`mcxx.features`，MC1 v0）

这个包有两部分，都不依赖任何后端，只读事实和注册在 Catalog 里的 provider：

- **门禁**（`mcxx.features`）：一个发现可以来自 MC++ 内置的规则，也可以来自插件。门禁根据这个特性在出现位置的级别，把它变成错误、警告、豁免，或者什么都不报。
- **`mc++.iso`**（`mcxx.features.iso`）：MC++ 内置的 provider，负责对 ISO C++ 语言特性的明确控制。它和插件一样针对 SDK 编写，注册成内置的，插件可以替换其中任何一项（见 [`../plugin`](../plugin/README.md) 的"覆盖"）。

## mc++.iso：ISO C++ 特性，全部是减法

每一项都用 ISO C++ 的 stable name 标明，也都是**减法**：程序不用这项特性，仍然是任何编译器都能编译的 ISO C++。默认全部 `allow`，MC++ 默认就是 ISO C++。

| 特性 id | stable name | 捕获什么 | 所在 profile |
|---|---|---|---|
| `raw-pointer-arithmetic` | [expr.add] [expr.sub] [expr.pre.incr] | 指针的 `+ - += -= ++ --`，以及对指针的 `[]`（C 数组自己的下标不算） | safe |
| `new-delete` | [expr.new] [expr.delete] | `new`、`delete`、`new[]`、`delete[]` | safe |
| `reinterpret-cast` | [expr.reinterpret.cast] | `reinterpret_cast`，以及实际做了重解释的 C 风格或函数式转换（经由 `void*` 的不算） | safe |
| `c-style-cast` | [expr.cast] [expr.type.conv] | `(T)x`，以及目标为标量的 `T(x)`（它就是 `(T)x`） | safe |
| `const-cast` | [expr.const.cast] | `const_cast` | safe |
| `union` | [class.union] | 联合体的定义 | safe |
| `c-array` | [dcl.array] | 类型为 C 数组的变量、成员、参数 | safe |
| `c-varargs` | [dcl.fct] [cstdarg.syn] | 带 C `...` 的函数，以及 `va_arg` | safe |
| `uninitialized` | [dcl.init.general] [basic.indet] | 默认初始化后值不确定的局部变量：标量、标量数组，以及平凡默认构造的非空类。static 和 thread_local 会被零初始化，不算 | safe |
| `asm` | [dcl.asm] | asm 声明（语句里的和文件作用域的） | safe |
| `include` | [cpp.include] [module.global.frag] | 不在全局模块片段里的 `#include`；在模块单元中，它位于模块的 purview 里 | modules |
| `goto` | [stmt.goto] | `goto`、间接 `goto` | strict |
| `macros` | [cpp.replace] | 本文件里的宏定义 | strict |
| `exceptions` | [except.throw] [except.pre] | `throw`、`try`（包括函数 try 块） | 由包自己设置 |
| `rtti` | [expr.typeid] [expr.dynamic.cast] | `typeid`、`dynamic_cast` | 由包自己设置 |

### profile

| profile | 含义 | 内容 |
|---|---|---|
| `safe` | **编译器不会替你检查的未定义行为来源**：越界指针运算、new/delete 的泄漏、重复释放和释放后使用、类型双关（`reinterpret_cast`、联合体）、写入 const 对象、读取未初始化的值、C 可变参数的类型错误、asm | 上表中标 safe 的 10 项 |
| `modules` | 所有依赖都通过 import：`#include` 只能出现在全局模块片段里 | `include` |
| `strict` | `safe` 加上 `modules`，再加上 `goto` 和宏定义 | 13 项 |
| `portable` | 只允许 ISO C++：**所有 extension 类别的特性都禁止**（MC++ 专有的扩展，例如 `[[mcpp::cfg]]`） | 按类别（`ext:cfg` 等） |

插件的特性可以通过 `Feature::profiles` 加入这些 profile；插件也可以定义新的 profile，或者重新定义一个已有的 profile（`Profile::replaces`）。

## 配置

写在包的 `mcpp.toml` 里。mcpp 保留 `[package.metadata.*]`，但不解释其内容：

```toml
[package.metadata.mcxx]
profile = "safe"                                    # 或者几个一起：["safe", "modules"]，取其中最严格的级别

[package.metadata.mcxx.features]                    # 整个包
"json-brace-init" = "deny"
exceptions = "deny"
raw-pointers = "deny"                               # 插件 mc++.policy 的特性

[package.metadata.mcxx.modules."app.legacy"]        # 一个模块（分区沿用它所属模块的设置）
goto = "allow"

[package.metadata.mcxx.namespaces."app::detail"]    # 一个命名空间及其内部的命名空间
reinterpret-cast = "allow"

[package.metadata.mcxx.files."src/compat/**"]       # 路径匹配的文件（相对 mcpp.toml 所在目录，MC1 0.4.0）
include = "allow"                                   # 包装 C 库的文件可以用头文件，其余照 profile 禁止

[package.metadata.mcxx.imports."legacy"]            # 导入 legacy 时允许它带进来的东西（M1.2）
allow = ["c-array", "raw-pointers"]
reason = "C 库，已经包装"
```

文件的级别：`*` 匹配路径的一段之内，`**` 匹配任意多段，`?` 匹配一个字符；多个模式都匹配时，非通配字符最多的那个说了算。优先级从高到低：声明上的豁免、区域、命名空间、文件、模块、包、profile、默认（MC1 §6）。

级别有三档：`allow`、`warn`、`deny`（也可以写 `off`、`warning`、`error`）。下面这些都会作为警告报出，不会被静默忽略：
- 无法识别的级别；
- 没有任何 provider 声明过的特性 id（例如写错的 `gotoo`）；
- 没有任何 provider 定义过的 profile；
- provider 之间的冲突。

## 优先级（A0.3.3）

声明上的 `[[mcpp::allow("id")]]` > 命名空间 > 模块 > 包 > profile（几个 profile 取其中最严格的）> 特性的默认级别。

## 豁免与审计（A0.3.4）

- `[[mcpp::allow("id")]]` 或 `[[mcpp::allow("id", "原因")]]` 写在声明上，覆盖这个声明的范围。
- 每一次豁免都记入 `Result::waived`；设置 `MCXX_AUDIT=<文件>` 时，每次豁免追加一行 JSON：特性、文件、行列、所在声明、原因。
- 不可豁免的特性（`Feature::waivable = false`）即使写了豁免也照样报错，并说明它不可豁免。
- 豁免中写了没有任何 provider 声明过的 id，会报警告并给出位置。

## 跨模块的方言边界（M1.2）

一个模块导入另一个模块时，对方接口导出的东西会随导入进入这个文件。事实 `imports`（MC3 §4.13）给出每个导入带进来的接口：对方和它再导出的模块各自的方言、导出的 T1 声明，全部读自对方的 `.ifc`（MC2），不读源码。`c-array`、`union`、`c-varargs`（mc++.iso）和 `raw-pointers`、`lib:std.vector`（mc++.policy）的规则用 SDK 的 `plugin::report_imports` 在导入处报出：

- 对方导出了这个特性的东西，而对方自己的方言并不禁止它（禁止的话，它自己就门禁过了，漏出来的是它审计过的豁免）；
- 对方的接口读不到（旁边和 store 里都没有）：带进来什么无法得知，同样是这个特性的发现。

发现属于这个特性，所以级别是导入方的。豁免写在导入上：`import legacy [[mcpp::allow("c-array, raw-pointers", "原因")]];`（宿主在 Clang 读文件之前把这个属性换成空格，Clang 不接受导入上的属性），或者写在 manifest 的 `[package.metadata.mcxx.imports."legacy"]` 里——mcpp 自己的依赖扫描不认导入上的属性，用 mcpp 构建时用这种写法。两种都进审计。

## 计划（Plan）与开销

`make_plan(config)` 把一个包的配置对照 Catalog 解析一次，结果按 manifest、修改时间和 Catalog 版本缓存（`plan_for`）。解析出来的内容：
- 每个特性的基础级别；
- 哪些特性在某处不是 `allow`（`gated`）；
- 每条规则要被问到哪些特性（`wanted`）；
- 需要收集哪些事实（`needs`）。

对一个具体文件，`select(plan, declared)` 再去掉这个文件里不可能出现的特性（`Feature::requires_declaration`）。宿主（`modules/backend/clang` 的 `:gate`）只收集剩下的特性需要的事实。什么都不用问时，这个文件不遍历 AST。

## 诊断（A0.7.5）

code 是特性 id；消息包含特性 id、改法和豁免方法，例如：

```
error: `x` (`int`) is not initialized [uninitialized]; give it an initializer (`T x {};` for zero); to allow it here, [[mcpp::allow("uninitialized")]] on the declaration
```

## 测试

`mcpp test -p modules/features`：
- `test_iso`：每个内置特性、各个 profile、多个 profile 同时使用、Plan 只问被门禁的特性、作用域和豁免、配置里的未知 id 和 profile。
- `test_override`：插件替换内置特性、同一 id 的冲突、重新定义 profile、取代整个 provider。

插件的规则在 `plugins/*/tests` 里测试。
