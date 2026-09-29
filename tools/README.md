# tools：开发工具

| 目录 | 内容 |
|---|---|
| `probe/` | `mcxx-probe`：用编译数据库驱动 libmc++ 的服务，方便开发和排查：`mcxx-probe --db 目录 --resource 目录 --cache 目录 [--index] 文件 [行:列 方法]...`。会打印计数器，支持 `MCXX_LOG` / `MCXX_TRACE`。`--tokens 文件...`：后端里 Clang 原始词法器的 token（lexdiff 的参照），`--tokens --bench N` 测速；`--symbols`：文件的大纲（syntaxdiff 的参照）；`--read-ifc X.ifc`：按 MC2 读出 `.ifc`，打印成 JSON；`--ifc X.ifc 文件`：文件的解析和 `.ifc` 读回的声明逐项比较（A1.1.3） |
| `lexdump/` | `mcxx-lexdump`：MC++ 自己的前端（`mcxx.frontend`）的输出。`文件...` 打印 token；`--pp` 打印预处理结果；`--ppdiff 文件 clang的-E输出` 逐 token 比对；`--directives` 给出文件的指令行；`--syntax` 打印大纲；`--fuzz N` 做模糊测试；`--bench N`、`--parse-bench N` 测速。不含 Clang |
| `conformance/` | `mcxx-conformance`：运行 `conformance/gates` 的门禁 fixture，统计每个特性的精确率和召回率，可输出 JSON。`--driver mcxx`：构建路径和编辑器路径的发现相同（A1.4.2）；`--frontend`：由 MC++ 自己的前端取事实，它能判定的特性（13 个），发现和 fixture 一致；转换类特性只要求不误报（A1.6.2、A1.8.3） |
| `checks/symbols.py` | 不需要 Clang 的程序里没有 Clang/LLVM 符号（A0.2.3）：用 `nm -C` 检查，mcxx 作为对照组 |
| `checks/corpus.py` | 语料检查（A0.5.1）：已构建语料的每个单元分别用 mcxx 和不含 MC++ 的 clang 23.1（llvm-clang-dev 的 `driver-smoke`）做 `-fsyntax-only`。要求 mcxx 零错误，编译器诊断和 clang 完全相同；MC++ 自己的诊断单独计数，不参与比较 |
| `checks/ifc.py` | MC2（M1.1）：从 SDK 源码构建 `ifc-printer`；方言 fixture `conformance/ifc/dialect` 读回的方言等于期望（A1.1.4）；`--corpus` 时，语料用 mcxx 构建后每个接口单元都有 `.ifc`（A1.1.1，依据 `build.ninja` 的 `bmi_out`）、`ifc-printer` 零错误读完全部 `.ifc`（A1.1.2）、读回的声明和解析结果逐项逐字段相等（A1.1.3） |
| `checks/facts.py` | T1 事实对照 Clang AST（A0.4.3）：语料自身的每个源文件，MSA 的声明（`mcxx-probe --facts`）和直接从 Clang AST 统计的声明（`--census`，Clang 自己的 RecursiveASTVisitor）按种类逐项相等 |
| `checks/toolchain.py` | mcxx 作为 mcpp 的工具链（V0.6，MC5 §7）：用未经修改的 mcpp 和 `llvm@23.1.0-mcxx`（`xpkgs/` 的包）构建并运行一个模块程序，并确认代码是 clang 23.1 编译的 |
| `checks/selfhost.py` | 自举（A0.5.3）：C-mcppls（mcpp-language-server 377d222）用 mcxx 作为工具链构建，并通过它自己的 `mcpp test`。json-brace-init 在语料里设为 warn：语料里有两处真实的这类问题 |
| `payload/payload.py` | 不经过 xlings，直接组装 mcxx 的 llvm 族 payload（与 `xpkgs/pkgs/m/mcxx.lua` 的布局相同） |
| `checks/compose.py` | `mcxx compose` 端到端（A0.6.1）：一个带自有静态插件的包，普通 mcxx 拒绝编译，组合后交给组合好的编译器，再次组合不重新链接。`--mcxx 路径`，第一次约 5 分钟 |
| `checks/specs.py` | 规范检查（A0.8.1）：schema、示例、反例、catalog 和 MC1 表格的一致性、要求 id 与 `conformance/traceability.json` 的证据；`--mcxx 路径` 时再检查真实的 mcxx 输出和两步编译只报一次。需要 `jsonschema` |
| `checks/lint.py` | 编译器检查不到的源码规则：`clang-exposure`（Clang 只能出现在 `modules/backend/clang*`）、`platform-exposure`（平台只能出现在 `modules/os`、`modules/arch`）、`json-brace-init`（为还没有用 mcxx 构建的代码保留；编译器本身已能捕获）。原始字符串的内容是数据，不参与检查。不带参数时检查整个仓库，约 0.5 s |
| `checks/lexdiff.py` | 词法对照 Clang（A1.6.1）：给定目录下每个 C++ 文件，`mcxx-lexdump` 和 `mcxx-probe --tokens` 的 token 逐个相同（种类、位置、原文，注释也比）。`--any` 连同没有后缀的文件（libc++ 头文件） |
| `checks/tools_safe.py` | mcpp-tools-safe 端到端（A1.5）：用 GCC 构建 `plugins/mcpp-tools-safe/tests/consumer`。干净构建通过；不改动再构建什么都不跑；加一个 goto，构建在检查处失败；去掉后再次通过。`--corpus` 时对 C-mcpp 的副本也做一遍 |
| `checks/syntaxdiff.py` | 语法对照 Clang（A1.7.1、A1.7.2）：语料自身的每个文件，`mcxx-lexdump --syntax` 的大纲和 Clang 后端的（`mcxx-probe --symbols`）逐个符号比较种类、名字、名字范围、整体范围；一致率低于 99.9% 或有文件解析失败即不通过。`--reference-cache` 保存 Clang 一侧的结果（`facts.py --reference-cache` 也会写） |
| `checks/ppdiff.py` | 预处理对照 Clang（A1.6.2）：编译数据库里语料自身的每个文件，`mcxx.frontend` 的预处理结果和 Clang `-E` 的主文件部分逐 token 相同；不同之处要么前端已说不确定，要么是头文件的宏（`--header-macros` 时由 Clang 给出这些宏） |

计划中还有 `devtools/`：差分测试、语料统计（M0.8）。分层检查和规范检查已经分别由 `checks/lint.py` 和 `checks/specs.py` 完成。
