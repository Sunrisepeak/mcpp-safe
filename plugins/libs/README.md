# plugins/libs：面向具体库的插件（`mcxx-plugins-libs`）

## `mcxx.plugins.json`：nlohmann::json

| 特性 id | 默认 | 捕获什么 |
|---|---|---|
| `json-brace-init` | deny | `Json x { expr };` 以及成员默认值 `Json m { nullptr };`：`nlohmann::basic_json` 有 `initializer_list` 构造函数，所以得到的是**只含一个元素的数组** `[expr]`，而不是 expr 的拷贝 |

判断条件都来自事实：类型特化自 `nlohmann::basic_json`（inline 的 ABI 命名空间已去掉），列表初始化，选中的是 `initializer_list` 构造函数，只有一个元素，而且这个元素本身不是花括号列表。所以对象字面量 `Json x { { "k", v } }`、拷贝 `Json x = y;` 和多元素数组都不会被报。

类别是 pitfall。它声明 `requires_declaration = "nlohmann"`：一个文件既没有声明也没有导入 `nlohmann` 时，这个特性不会被问到，也不会为它遍历 AST。所以它可以默认开启，对不用这个库的代码没有开销。

这个坑在 mcxx 引擎的开发中出现过好几次，mcppls 自己的代码里也有两处。确实想要一个单元素数组时，写 `Json::array({ value })`，或者加 `[[mcpp::allow("json-brace-init", "原因")]]`，这次豁免会记入审计。
