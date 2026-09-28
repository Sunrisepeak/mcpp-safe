# lsp：LSP 形态的语义服务（`mcxx.lsp`，MC6）

对任意 `msa::Workspace` 提供一套 LSP 语义：输入文档和请求，输出 JSON 结果和诊断通知。它不含传输层，由宿主喂给它消息：mcppls 的 mcxx 引擎、测试驱动，以及以后的 `mcxx serve`。

- 文档：`open`、`change`（全量或增量）、`close`（文件内容回到磁盘版本）、`saved`、`changed_on_disk`、`refresh`。
- 请求：definition、declaration、typeDefinition、implementation、hover、references、documentHighlight、documentSymbol、workspace/symbol（支持带作用域的查询）、semanticTokens/full、completion、signatureHelp、textDocument/diagnostic、symbolInfo（clangd 扩展，给出 USR）、prepareCallHierarchy 与 callHierarchy/outgoingCalls。
- 解析在自己的线程里进行，每次解析都通过 notify 回调发布 `textDocument/publishDiagnostics`；回调可以为空。
- `from_lsp` / `to_lsp`：UTF-16 与 UTF-8 字节列的换算。

测试：`mcpp test -p modules/lsp`。它使用一个不依赖 Clang 的假后端（`FakeWorkspace`），完整一轮约 3 s。
