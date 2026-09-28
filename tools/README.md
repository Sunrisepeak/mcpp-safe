# tools：开发工具

| 目录 | 内容 |
|---|---|
| `probe/` | `mcxx-probe`：用编译数据库驱动 libmc++ 的服务，方便开发和排查：`mcxx-probe --db 目录 --resource 目录 --cache 目录 [--index] 文件 [行:列 方法]...`。会打印计数器，支持 `MCXX_LOG` / `MCXX_TRACE` |
| `checks/lint.py` | 编译器检查不到的源码规则：`clang-exposure`（Clang 只能出现在 `modules/backend/clang*`）、`json-brace-init`（为还没有用 mcxx 构建的代码保留；编译器本身已能捕获）。不带参数时检查整个仓库，约 0.5 s |

计划中还有 `devtools/`：分层检查、规范检查、差分测试、语料统计（M0.8）。
