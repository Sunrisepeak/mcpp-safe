# plugins：插件实现

MC++ 第一方插件的实现。插件机制本身（SDK、门禁、宿主）在 [`modules/plugin`](../modules/plugin/README.md) 和 [`modules/features`](../modules/features/README.md)，这里只有具体插件，它们和第三方插件一样只针对 SDK（`mcxx.plugin`）编写。

**一个目录是一个包，里面可以放多个插件模块**，按主题分组。链接这个包就会注册其中全部插件；把包放进哪个程序由组合方式决定：今天是 mcxx 的根 `mcpp.toml`（静态链接），以后由 `mcxx compose` 按项目生成（A0.6.1）。

| 目录 | 包 | 插件模块 | 扩展点 |
|---|---|---|---|
| [`std`](std/README.md) | `mcxx-plugins-std` | `mcxx.plugins.safe`（mc++.safe v0） | 规则 |
| | | `mcxx.plugins.cfg`（`[[mcpp::cfg(...)]]`） | 源码过滤器 |
| [`libs`](libs/README.md) | `mcxx-plugins-libs` | `mcxx.plugins.json`（nlohmann::json） | 规则 |

以后还会有：`mcpp-tools-safe`（对非 mcxx 工具链以 blocking check 运行 `mcxx check`，M1.5）和 `gpu`（GPU 区域插件，M3），各自一个目录。

新增一个插件：放进主题相近的目录（新建一个模块单元，并在该包的 lib 根里 `export import` 它），或者新建一个目录。然后在 `tests/` 里只用事实做测试（不依赖 Clang，秒级），再在 `modules/backend/clang/tests` 里加一个经过 Clang 的用例。
