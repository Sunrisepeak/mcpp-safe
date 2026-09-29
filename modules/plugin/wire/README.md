# plugin/wire：插件的 JSON 格式（`mcxx.plugin.wire`）

MC++ 插件在进程之间传递的数据格式，宿主（`../host`）和插件进程（`../remote`）两边都用它：

| 内容 | 规范 |
|---|---|
| 一个文件的事实：`facts_to_json`、`facts_from_json` | MC3 §4.11（`specs/schema/mc3-facts.schema.json`） |
| 特性、profile、provider 描述、发现、目标平台 | MC4 §2、§6（`specs/schema/mc4-protocol.schema.json`） |
| 一条消息占一行：`line`、`message_from` | MC4 §6.1 |

读取是严格的：缺少的或类型不对的成员都报错，并写明是哪一个成员（例如 `facts.casts[0].kind`）。写出后再读回，得到的是同一个值（MC3-4.11-2）。

测试：`mcpp test -p modules/plugin/wire`，包括规范示例 `specs/examples/mc3-facts.json` 的往返。

## 文件

接口单元只声明，定义在实现单元里（MC5 §8）。

| 文件 | 内容 |
|---|---|
| `src/wire.cppm` | 主接口：`export import :api` |
| `src/api.cppm` | `:api`：类型和函数的声明 |
| `src/json.cppm` | `:json`（内部分区）：读写共用的——缺成员或类型不对时报出成员名的 reader、枚举的写法、位置、声明 |
| `src/facts.cpp` | MC3 事实的 JSON（§4.11） |
| `src/protocol.cpp` | MC4 的消息（§6） |
