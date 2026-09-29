# MC6 — `mcxx serve`: the Semantic Service as a Process

| | |
|---|---|
| Specification | MC6 |
| Version | 1 (protocol version 1) |
| Status | Draft |
| Schema | [`schema/mc6-requests.schema.json`](schema/mc6-requests.schema.json) |
| Examples | [`examples/mc6-session.json`](examples/mc6-session.json) |
| Implementation | `modules/serve` (server and client), `modules/lsp` (the service), `modules/driver` (`mcxx serve`) |
| License | Apache-2.0 |

## Abstract

`mcxx serve` is libmc++'s semantic service as a separate process, for a host -- an editor engine, a build tool, an agent -- that wants MC++'s answers without linking libmc++, or wants a crash of the service to cost it a request rather than itself. It speaks the Language Server Protocol's base protocol and a subset of LSP's requests, and adds MC++'s own: a document's facts (MC3), its gates (MC1), the catalog, and the program's compile commands. This specification defines the transport, the lifecycle, MC++'s requests and what a host's client does when the process dies.

## 1. Conventions

- The key words **MUST**, **MUST NOT**, **REQUIRED**, **SHALL**, **SHALL NOT**, **SHOULD**, **SHOULD NOT**, **RECOMMENDED**, **MAY** and **OPTIONAL** are to be interpreted as described in BCP 14 (RFC 2119, RFC 8174) when, and only when, they appear in all capitals.
- "LSP" is the Language Server Protocol 3.17; JSON-RPC is 2.0.

## 2. Transport and lifecycle

`mcxx serve [--db DIR] [--resource DIR] [--cache DIR]` reads messages from its standard input and writes messages to its standard output, each framed as LSP's base protocol frames it (`Content-Length: N`, a blank line, N bytes of JSON). <a id="MC6-2-1"></a><sup>MC6-2-1</sup> It writes nothing else to its standard output; its logs go to standard error (MC5 §4). <a id="MC6-2-2"></a><sup>MC6-2-2</sup>

- `initialize` answers LSP's `capabilities`, with `capabilities.experimental.mcxx` = `{ "version": 1, "requests": [...] }` naming the MC++ requests of §3 the server has. <a id="MC6-2-3"></a><sup>MC6-2-3</sup>
- `exit` ends the process: with status 0 when `shutdown` came before it, and 1 otherwise. <a id="MC6-2-4"></a><sup>MC6-2-4</sup>
- `textDocument/didOpen`, `didChange`, `didClose`, `didSave` and `workspace/didChangeWatchedFiles` are LSP's; after each change of an open document the server publishes `textDocument/publishDiagnostics` for it, MC++'s gate findings among them with the feature id as `code` (MC1 §9). <a id="MC6-2-5"></a><sup>MC6-2-5</sup>
- Every other LSP request the service answers (hover, definition, references, documentSymbol, completion, ...) is answered as the service answers it; one it does not is an error `-32601`. <a id="MC6-2-6"></a><sup>MC6-2-6</sup>

## 3. MC++'s requests

| Method | Params | Result |
|---|---|---|
| `mcxx/setCommands` | `commands`: compile commands as `compile_commands.json` has them (`directory`, `file`, `arguments` or `command`) | `null`; the open documents are parsed again |
| `mcxx/facts` | `textDocument.uri` of an open document | the document's facts, MC3 §4.11 |
| `mcxx/gates` | `textDocument.uri` | `manifest`, `module`, `profiles`, `problems`, and for every feature: `id`, `category`, `level` (for the document's module, outside any namespace setting), `gated` |
| `mcxx/catalog` | -- | the catalog, MC1 §10 |

- `mcxx/facts` MUST answer the facts of the document's current version, every kind collected. <a id="MC6-3-1"></a><sup>MC6-3-1</sup>
- `mcxx/gates` MUST answer what the compiler applies to the same file: the configuration of MC1 §5 and the levels of MC1 §6. <a id="MC6-3-2"></a><sup>MC6-3-2</sup>
- The params and results MUST validate against [`schema/mc6-requests.schema.json`](schema/mc6-requests.schema.json). <a id="MC6-3-3"></a><sup>MC6-3-3</sup>

## 4. Isolation: the host's client

A host runs `mcxx serve` as a child process through a client (`mcxx::serve::Client`). When the process ends -- a crash, a kill -- the client MUST NOT end the host: a request in flight fails with the reason, and the next request or notification starts the process again. <a id="MC6-4-1"></a><sup>MC6-4-1</sup> A process started again MUST be told what the host had told the one before it: the handshake, the last `mcxx/setCommands`, and every document still open with its latest full text, before the host's next message. <a id="MC6-4-2"></a><sup>MC6-4-2</sup> A request that gets no answer within the client's time limit is an error, not a hang. <a id="MC6-4-3"></a><sup>MC6-4-3</sup>
