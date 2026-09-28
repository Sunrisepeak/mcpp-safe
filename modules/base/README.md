# base：基础设施（`mcxx.base`）

libmc++ 各包共用的基础部分，不依赖 openkal-llvm-runtime 以外的任何东西。大部分复制自 mcppls 的 mcppls-base（同一作者，Apache-2.0），改名为 mcxx。

| 模块 | 内容 |
|---|---|
| `mcxx.base.error` | 错误值与 `Result` |
| `mcxx.base.path` | 路径规范化与拼接 |
| `mcxx.base.text` | 文本工具，包括 LSP 需要的 UTF-16 位置换算 |
| `mcxx.base.uri` | `file:` URI 与路径互转 |
| `mcxx.base.sha256` | SHA-256 |
| `mcxx.base.log` | 面向进程的日志（标准错误、滚动文件） |
| `mcxx.base.trace` | 面向库的可观测性：按类别和级别输出日志（`MCXX_LOG`）、耗时区间、计数器、Chrome trace 文件（`MCXX_TRACE`） |
| `mcxx.base.toml` | TOML 解析，复制自 mcpp（Apache-2.0）；用来读取 `[package.metadata.mcxx]` |

`import mcxx.base;` 导入以上全部。测试：`mcpp test -p modules/base`。
