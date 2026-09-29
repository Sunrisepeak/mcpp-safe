# tools：开发工具

| 目录 | 内容 |
|---|---|
| `probe/` | `mcxx-probe`：用编译数据库驱动 libmc++ 的服务，方便开发和排查：`mcxx-probe --db 目录 --resource 目录 --cache 目录 [--index] 文件 [行:列 方法]...`。会打印计数器，支持 `MCXX_LOG` / `MCXX_TRACE`。`--tokens 文件...`：后端里 Clang 原始词法器的 token（lexdiff 的参照），`--tokens --bench N` 测速；`--symbols`：文件的大纲（syntaxdiff 的参照）；`--read-ifc X.ifc`：按 MC2 读出 `.ifc`，打印成 JSON；`--ifc X.ifc 文件`：文件的解析和 `.ifc` 读回的声明逐项比较（A1.1.3） |
| `lexdump/` | `mcxx-lexdump`：MC++ 自己的前端（`mcxx.frontend`）的输出。`文件...` 打印 token；`--pp` 打印预处理结果；`--ppdiff 文件 clang的-E输出` 逐 token 比对；`--directives` 给出文件的指令行；`--syntax` 打印大纲；`--facts` 打印 T1 声明（MC3 JSON，declsdiff 的一侧）；`MCXX_LOG=frontend.syntax=debug` 时逐条打印解析器的判断（记录的声明、每个 `{` 是语句块、lambda 体还是初始化器、控制语句的括号、跳过的地方）；`--fuzz N` 做模糊测试；`--bench N`、`--parse-bench N` 测速。不含 Clang |
| `conformance/` | `mcxx-conformance`：运行 `conformance/gates` 的门禁 fixture，统计每个特性的精确率和召回率，可输出 JSON。`--driver mcxx`：构建路径和编辑器路径的发现相同（A1.4.2）；`--frontend`：由 MC++ 自己的前端取事实，它能判定的特性（13 个），发现和 fixture 一致；转换类特性只要求不误报（A1.6.2、A1.8.3） |
| `checks/symbols.py` | 不需要 Clang 的程序里没有 Clang/LLVM 符号（A0.2.3）：用 `nm -C` 检查，mcxx 作为对照组 |
| `checks/corpus.py` | 语料检查（A0.5.1）：已构建语料的每个单元分别用 mcxx 和不含 MC++ 的 clang 23.1（llvm-clang-dev 的 `driver-smoke`）做 `-fsyntax-only`。要求 mcxx 零错误，编译器诊断和 clang 完全相同；MC++ 自己的诊断单独计数，不参与比较 |
| `checks/index.py` | E-IDX-5：规则包和插件从本仓库的 index（`index/mcxx`，tag `0.1.0` 的压缩包）取用：consumer 改为 `[build-dependencies.mcxx] mcpp-tools-safe = "0.1.0"` 后用 GCC 构建通过、写出 stamp；依赖 `mcxx-plugins-std`、`-libs` 的程序构建通过，目录里有它们的 provider |
| `checks/xlings.py` | E-XL-1：用本仓库的 xpkg（`xpkgs/pkgs/m/mcxx.lua`）经 xlings 安装 mcxx、`xlings use mcxx 0.1.0`，xlings 路径上的 `mcxx` 就是装进去的那个（`version --json` 相同），mcpp 列出工具链 `llvm 23.1.0-mcxx` |
| `checks/boundary.py` | 跨模块的方言边界（M1.2）：用 mcxx 编译 `conformance/boundary`，导入处的发现等于期望、导入处 `[[mcpp::allow]]` 的豁免进审计（A1.2.1）；删掉被导入包的源码结果不变（A1.2.2）；删掉 BMI 旁边的 `.ifc` 时从 store 读到；store 也空时报"无法得知"；带 `--toolchain` 时用 mcpp 构建 manifest 形式的 `app-manifest` |
| `checks/ifc.py` | MC2（M1.1）：从 SDK 源码构建 `ifc-printer`；方言 fixture `conformance/ifc/dialect` 读回的方言等于期望（A1.1.4）；`--corpus` 时，语料用 mcxx 构建后每个接口单元都有 `.ifc`（A1.1.1，依据 `build.ninja` 的 `bmi_out`）、`ifc-printer` 零错误读完全部 `.ifc`（A1.1.2）、读回的声明和解析结果逐项逐字段相等（A1.1.3） |
| `checks/facts.py` | T1 事实对照 Clang AST（A0.4.3）：语料自身的每个源文件，MSA 的声明（`mcxx-probe --facts`）和直接从 Clang AST 统计的声明（`--census`，Clang 自己的 RecursiveASTVisitor）按种类逐项相等 |
| `checks/toolchain.py` | mcxx 作为 mcpp 的工具链（V0.6，MC5 §7）：用未经修改的 mcpp 和 `llvm@23.1.0-mcxx`（`xpkgs/` 的包）构建并运行一个模块程序，并确认代码是 clang 23.1 编译的 |
| `checks/selfhost.py` | 自举（A0.5.3）：C-mcppls（mcpp-language-server 377d222）用 mcxx 作为工具链构建，并通过它自己的 `mcpp test`。json-brace-init 在语料里设为 warn：语料里有两处真实的这类问题 |
| `payload/payload.py` | 不经过 xlings，直接组装 mcxx 的 llvm 族 payload（与 `xpkgs/pkgs/m/mcxx.lua` 的布局相同） |
| `checks/compose.py` | `mcxx compose` 端到端（A0.6.1）：一个带自有静态插件的包，普通 mcxx 拒绝编译，组合后交给组合好的编译器，再次组合不重新链接。`--mcxx 路径`，第一次约 5 分钟 |
| `checks/specs.py` | 规范检查（A0.8.1）：schema、示例、反例、catalog 和 MC1 表格的一致性、要求 id 与 `conformance/traceability.json` 的证据；`--mcxx 路径` 时再检查真实的 mcxx 输出和两步编译只报一次。需要 `jsonschema` |
| `checks/lint.py` | 编译器检查不到的源码规则：`clang-exposure`（Clang 只能出现在 `modules/backend/clang*`）、`platform-exposure`（平台只能出现在 `modules/os`、`modules/arch`）、`json-brace-init`（为还没有用 mcxx 构建的代码保留；编译器本身已能捕获）、`direct-output`（库代码不直接写标准输出和标准错误，经 mcxx.base 的 log 和 trace；只有输出本身就是用途的 5 个文件除外，各自写明原因）。原始字符串的内容是数据，不参与检查。不带参数时检查整个仓库，约 0.5 s |
| `checks/lexdiff.py` | 词法对照 Clang（A1.6.1）：给定目录下每个 C++ 文件，`mcxx-lexdump` 和 `mcxx-probe --tokens` 的 token 逐个相同（种类、位置、原文，注释也比）。`--any` 连同没有后缀的文件（libc++ 头文件） |
| `checks/tools_safe.py` | mcpp-tools-safe 端到端（A1.5）：用 GCC 构建 `plugins/mcpp-tools-safe/tests/consumer`。干净构建通过；不改动再构建什么都不跑；加一个 goto，构建在检查处失败；去掉后再次通过。`--corpus` 时对 C-mcpp 的副本也做一遍 |
| `checks/syntaxdiff.py` | 语法对照 Clang（A1.7.1、A1.7.2）：语料自身的每个文件，`mcxx-lexdump --syntax` 的大纲和 Clang 后端的（`mcxx-probe --symbols`）逐个符号比较种类、名字、名字范围、整体范围；一致率低于 99.9% 或有文件解析失败即不通过。`--reference-cache` 保存 Clang 一侧的结果（`facts.py --reference-cache` 也会写） |
| `checks/ppdiff.py` | 预处理对照 Clang（A1.6.2）：编译数据库里语料自身的每个文件，`mcxx.frontend` 的预处理结果和 Clang `-E` 的主文件部分逐 token 相同；不同之处要么前端已说不确定，要么是头文件的宏（`--header-macros` 时由 Clang 给出这些宏） |
| `checks/declsdiff.py` | 声明对照 Clang（M2.1：A2.1.1、A2.1.2）：语料自身的每个源文件，`mcxx-lexdump --facts` 的 T1 声明和 Clang 后端的（`mcxx-probe --facts`）按名字位置和种类配对，逐个成员（限定名、所属、导出、各标志、类型文本、模板）统计一致率并给出不一致的例子。目前只报告，不设门槛（F1 的语义层是 M2 的工作） |
| `checks/platforms.py` | libmc++ 的单元测试在 openkal 的其他目标上：`--target x86_64-windows-gnu --wine` 在本机交叉构建每个成员的测试并用 wine 运行；`--target aarch64-macos --out 目录` 交叉构建后放进目录，在 macOS 上用 `--run 目录` 运行（先 ad hoc 签名）。测试经 `$MCXX_REPOSITORY` 找到仓库里的数据 |

计划中还有 `devtools/`：差分测试、语料统计（M0.8）。分层检查和规范检查已经分别由 `checks/lint.py` 和 `checks/specs.py` 完成。
