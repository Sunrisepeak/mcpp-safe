# plugin/sdk：插件 SDK（`mcxx.plugin`，MC4）

provider 写给它的接口：特性（级别、类别、由哪些事实判定）、profile、认领的属性；规则（`Rule`：拿到 `Context`，报出 `Finding`，`Context::fail` 报告失败）；源码过滤器（`SourceFilter`：解析之前改写文本）；注册（静态初始化时 `Registration<R>`）以及把全部注册解析成的 `Catalog`（覆盖、替换、冲突在这里决定）。一个 provider 只看 MSA 的事实，看不到任何后端的类型。

整体说明见上一级的 [`README.md`](../README.md) 和 `specs/mc4-plugins.md`。

## 文件

接口单元只声明，定义在实现单元里（MC5 §8）。

| 文件 | 内容 |
|---|---|
| `src/plugin.cppm` | 主接口：只 `export import` 下面四个分区 |
| `src/feature.cppm` | `:feature`：级别、类别、特性、profile、属性规格 |
| `src/rule.cppm` | `:rule`：`Finding`、`Failure`、`Context`、`Provider`、`Rule` |
| `src/filter.cppm` | `:filter`：源码过滤器 |
| `src/catalog.cppm` | `:catalog`：注册和 `Catalog`；`SDK_ABI`（插件库与装入它的编译器必须一致的布局版本，库里的 `mcxx_plugin_sdk_abi`）和 `hold_registrations`/`release_registrations`（插件库装入期间的注册先扣下，MC4-3-7） |
| `src/names.cpp` | 级别和类别的写法、特性的宏名 |
| `src/catalog.cpp` | 注册表、解析成 Catalog、声明的事实子树、应用源码过滤器 |

## 测试

SDK 的行为由使用它的包测试：`modules/features`（内置 provider 和覆盖）、`plugins/std`、`plugins/libs`、`modules/plugin/host`（进程外、插件库）。
