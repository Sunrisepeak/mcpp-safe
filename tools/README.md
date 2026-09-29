# tools：开发工具

| 目录 | 内容 |
|---|---|
| `probe/` | `mcxx-probe`：用编译数据库驱动 libmc++ 的服务，方便开发和排查：`mcxx-probe --db 目录 --resource 目录 --cache 目录 [--index] 文件 [行:列 方法]...`。会打印计数器，支持 `MCXX_LOG` / `MCXX_TRACE` |
| `conformance/` | `mcxx-conformance`：运行 `conformance/gates` 的门禁 fixture，统计每个特性的精确率和召回率，可输出 JSON |
| `checks/corpus.py` | 语料检查（A0.5.1）：已构建语料的每个单元分别用 mcxx 和不含 MC++ 的 clang 23.1（llvm-clang-dev 的 `driver-smoke`）做 `-fsyntax-only`。要求 mcxx 零错误，编译器诊断和 clang 完全相同；MC++ 自己的诊断单独计数，不参与比较 |
| `checks/facts.py` | T1 事实对照 Clang AST（A0.4.3）：语料自身的每个源文件，MSA 的声明（`mcxx-probe --facts`）和直接从 Clang AST 统计的声明（`--census`，Clang 自己的 RecursiveASTVisitor）按种类逐项相等 |
| `checks/toolchain.py` | mcxx 作为 mcpp 的工具链（V0.6，MC5 §7）：用未经修改的 mcpp 和 `llvm@23.1.0-mcxx`（`xpkgs/` 的包）构建并运行一个模块程序，并确认代码是 clang 23.1 编译的 |
| `checks/selfhost.py` | 自举（A0.5.3）：C-mcppls（mcpp-language-server 377d222）用 mcxx 作为工具链构建，并通过它自己的 `mcpp test`。json-brace-init 在语料里设为 warn：语料里有两处真实的这类问题 |
| `payload/payload.py` | 不经过 xlings，直接组装 mcxx 的 llvm 族 payload（与 `xpkgs/pkgs/m/mcxx.lua` 的布局相同） |
| `checks/compose.py` | `mcxx compose` 端到端（A0.6.1）：一个带自有静态插件的包，普通 mcxx 拒绝编译，组合后交给组合好的编译器，再次组合不重新链接。`--mcxx 路径`，第一次约 5 分钟 |
| `checks/specs.py` | 规范检查（A0.8.1）：schema、示例、反例、catalog 和 MC1 表格的一致性、要求 id 与 `conformance/traceability.json` 的证据；`--mcxx 路径` 时再检查真实的 mcxx 输出和两步编译只报一次。需要 `jsonschema` |
| `checks/lint.py` | 编译器检查不到的源码规则：`clang-exposure`（Clang 只能出现在 `modules/backend/clang*`）、`json-brace-init`（为还没有用 mcxx 构建的代码保留；编译器本身已能捕获）。不带参数时检查整个仓库，约 0.5 s |

计划中还有 `devtools/`：差分测试、语料统计（M0.8）。分层检查和规范检查已经分别由 `checks/lint.py` 和 `checks/specs.py` 完成。
