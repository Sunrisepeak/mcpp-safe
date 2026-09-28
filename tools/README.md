# tools：开发工具

| 目录 | 内容 |
|---|---|
| `probe/` | `mcxx-probe`：用编译数据库驱动 libmc++ 的服务，方便开发和排查：`mcxx-probe --db 目录 --resource 目录 --cache 目录 [--index] 文件 [行:列 方法]...`。会打印计数器，支持 `MCXX_LOG` / `MCXX_TRACE` |
| `conformance/` | `mcxx-conformance`：运行 `conformance/gates` 的门禁 fixture，统计每个特性的精确率和召回率，可输出 JSON |
| `checks/compose.py` | `mcxx compose` 端到端（A0.6.1）：一个带自有静态插件的包，普通 mcxx 拒绝编译，组合后交给组合好的编译器，再次组合不重新链接。`--mcxx 路径`，第一次约 5 分钟 |
| `checks/specs.py` | 规范检查（A0.8.1）：schema、示例、反例、catalog 和 MC1 表格的一致性、要求 id 与 `conformance/traceability.json` 的证据；`--mcxx 路径` 时再检查真实的 mcxx 输出和两步编译只报一次。需要 `jsonschema` |
| `checks/lint.py` | 编译器检查不到的源码规则：`clang-exposure`（Clang 只能出现在 `modules/backend/clang*`）、`json-brace-init`（为还没有用 mcxx 构建的代码保留；编译器本身已能捕获）。不带参数时检查整个仓库，约 0.5 s |

计划中还有 `devtools/`：差分测试、语料统计（M0.8）。分层检查和规范检查已经分别由 `checks/lint.py` 和 `checks/specs.py` 完成。
