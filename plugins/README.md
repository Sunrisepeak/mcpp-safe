# plugins：插件实现

MC++ 第一方插件的实现。插件机制本身在 [`modules/plugin`](../modules/plugin/README.md)（SDK、Catalog）和 [`modules/features`](../modules/features/README.md)（门禁，以及 MC++ 内置的 ISO 特性控制 `mc++.iso`）。这里只有具体插件，它们和第三方插件一样只针对 SDK（`mcxx.plugin`）编写。

**MC++ 内置的和插件提供的分工如下：**
- 内置的（`mc++.iso`）是对 ISO C++ 语言特性的明确控制：每一项都用标准的 stable name 标明，并且都是减法，去掉之后剩下的仍然是 ISO C++。
- 插件提供另外几类特性：policy（例如能不能用裸指针）、library（库控制）、pitfall（库的坑）、extension（MC++ 专有的扩展）、standard（更新的 C++ 标准的核心语言特性，默认关闭，按文件开启）。插件也可以替换内置的实现（见 `modules/plugin/README.md` 的"覆盖"）。

**一个目录是一个包，里面可以放多个插件模块**，按主题分组。链接这个包就会注册其中全部插件；把包放进哪个程序由组合方式决定：今天是 mcxx 的根 `mcpp.toml`（静态链接），以后由 `mcxx compose` 按项目生成（A0.6.1）。

| 目录 | 包 | 插件模块（provider） | 扩展点 | 特性 |
|---|---|---|---|---|
| [`std`](std/README.md) | `mcxx-plugins-std` | `mcxx.plugins.policy`（mc++.policy） | 规则 | `raw-pointers`（policy）、`lib:std.vector`（library） |
| | | `mcxx.plugins.cfg`（`[[mcpp::cfg(...)]]`） | 源码过滤器 | `ext:cfg`（extension） |
| [`libs`](libs/README.md) | `mcxx-plugins-libs` | `mcxx.plugins.json`（nlohmann::json） | 规则 | `json-brace-init`（pitfall） |
| [`flow`](flow/README.md) | `mcxx-plugins-flow` | `mcxx.plugins.flow`（控制流） | 分析 pass（MC4 0.5.0） | `uninitialized-read`、`missing-return`、`noreturn-returns`（pitfall，`flow` 层，profile `safe` 禁止） |
| [`lang/cpp26`](lang/README.md) | `mcxx-plugins-lang-cpp26` | `mcxx.plugins.lang.cpp26`（C++26） | 语言（编译参数） | C++26 的 54 篇核心语言提案，`c++26:reflection` 等（standard） |
| [`lang/cpp29`](lang/README.md) | `mcxx-plugins-lang-cpp29` | `mcxx.plugins.lang.cpp29`（C++29） | 语言（编译参数） | C++29 的 17 篇核心语言提案，`c++29:pack-indexing-template-names` 等（standard） |

[`examples/`](examples/README.md) 是同一个插件包的两种用法：静态组合（`naming`）和进程外（`naming-remote`，可以用 GCC 构建）。

[`mcpp-tools-safe`](mcpp-tools-safe/README.md) 不是 SDK 插件，而是 mcpp 的构建规则包（模块 `mcxx.check`）。用 GCC 等其他编译器构建时，它把 `mcxx check` 作为 blocking check 动作放进构建（M1.5）。

以后还会有 `gpu`（GPU 区域插件，M3），单独一个目录。

新增一个插件：放进主题相近的目录（新建一个模块单元，并在该包的 lib 根里 `export import` 它），或者新建一个目录。然后在 `tests/` 里只用事实做测试（不依赖 Clang，秒级），再在 `modules/backend/clang/tests` 里加一个经过 Clang 的用例、在 `conformance/gates` 里加正例和反例。
