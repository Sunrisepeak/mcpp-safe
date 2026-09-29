# serve：`mcxx serve`（`mcxx.serve`，MC6）

libmc++ 的语义服务作为一个独立进程运行。宿主（编辑器引擎、构建工具、agent）有两种情况会用到它：不想链接 libmc++，或者希望服务崩溃时只丢掉一个请求，不影响宿主自己。

| 部分 | 内容 |
|---|---|
| 传输 | LSP 的基础协议（`Content-Length` 分帧，JSON-RPC 2.0），走标准输入输出：`frame`、`read_message` |
| 服务端 | `run(in, out, options)`：LSP 的请求交给 `mcxx.lsp` 的 Service；MC++ 自己的请求有 `mcxx/setCommands`、`mcxx/facts`（MC3）、`mcxx/gates`（MC1）、`mcxx/catalog` |
| 客户端 | `Client`：宿主一侧，启动 `mcxx serve`，读回应答和通知。进程结束后，下一次调用会重新启动它，并重放握手、最后一次 `mcxx/setCommands` 和所有仍打开的文档（A1.3.2）。每个请求都有时间限制 |

命令行：`mcxx serve [--db 目录] [--resource 目录] [--cache 目录]`。`--resource` 默认用程序旁边的 `../lib/clang/23`，也就是 payload 的布局。

测试：`mcpp test -p modules/serve`。测试程序本身带 `--serve` 参数运行时就是服务端，由 `Client` 驱动，覆盖以下内容：
- 握手；
- 门禁发现作为诊断推送；
- facts、gates、catalog、documentSymbol；
- 杀掉进程后，客户端重新启动它、重放状态，并且结果不变。
