# frontend：MC++ 自己的前端（`mcxx.frontend`，F1）

和 Clang 后端并列、不依赖 Clang/LLVM 的 C++ 前端（计划 P4）。F1 目前做两件事：词法分析，以及模块单元需要的那部分预处理（翻译阶段 4）；它给出的 MC3 事实交给同一套门禁引擎。语法树是下一步（M1.7）。

| 分区 | 内容 |
|---|---|
| `:lex` | C++23 的原始词法分析（不做预处理），逐条照 Clang 23.1 的原始词法器（`lib/Lex/Lexer.cpp`）：token 种类用 Clang 的名字（`raw_identifier`、`l_paren`、`utf8_string_literal`……），便于逐个比对 |
| `:unicode` | 标识符可用的 Unicode 字符（XID_Start / XID_Continue，UAX #31，加上 Clang 作为扩展接受的数学记号）。**生成的**：`gen/unicode.py` 读所钉版本 Clang 的 `UnicodeCharSets.h`，Unicode 版本和参照一致（18.0） |
| `:predefined` | 三个目标（linux-x64、macos-arm64、windows-x64）的预定义宏，**生成的**：`gen/predefined.py` 跑 `mcxx c++ -target T -std=c++23 -dM -E` |
| `:preprocess` | 预处理：条件编译、文件自己的宏（`#`、`##`、`__VA_ARGS__`、`__VA_OPT__`、GNU 的 `, ## __VA_ARGS__`）、`#include` 记录、模块声明和 import 的识别 |
| `mcxx.frontend` | 上面几部分，再加 `facts(pp)`：MC3 的 `macros` 和 `includes` 事实，形状和 Clang 后端给的一样 |

```cpp
import mcxx.frontend;
auto tokens = mcxx::frontend::lex(text);
auto pp = mcxx::frontend::preprocess(text, { .file = path, .target = "x86_64-unknown-linux-gnu" });
auto facts = mcxx::frontend::facts(pp);   // 交给 mcxx::features::evaluate，和 Clang 路径一样
```

## 词法：和 Clang 逐 token 一致

覆盖面，每一条都照 Clang 的做法：

- 标识符：关键字在原始词法里也是标识符；`$`、UCN（`é`、`\U…`、`\u{…}`）、UTF-8 字母，按 UAX #31 判断。
- pp-number：数字分隔符、指数；十六进制浮点数的 `p+` 只在 `0x` 开头时成立。
- 字面量：字符和字符串的各种前缀、原始字符串。用户定义字面量的后缀：字符串和字符只接受 `_` 开头的，或者标准库的后缀（`s`、`sv`、`h`、`min`、`ms`、`us`、`ns`、`i`、`il`、`if`、`d`、`y`）。
- 标点：全部标点和二连符，包括 `<::` 规则。
- 注释。
- 续行：反斜杠、可选空白、换行；在原始字符串的正文里不起作用。
- BOM。
- 不成 token 的东西各自算一个 `unknown`：空的或没有结束的字面量、坏的原始字符串分隔符、没有结束的注释、孤立字符。

和 Clang 唯一的不同：字面量之外、命名了一个字母的 `\N{名字}`。Clang 会查名字表，这里没有 Unicode 名字表，所以 Clang 读成标识符，这里读成 `unknown`。

**检查**：`tools/checks/lexdiff.py`。一边是 `mcxx-lexdump`（本前端），另一边是 `mcxx-probe --tokens`（后端里 Clang 的原始词法器，和 `-dump-raw-tokens` 同一个模式），逐文件比对 token 的种类、位置和原文，注释也比。2026-09-29 本机的结果：

| 语料 | 一致 |
|---|---|
| C-mcppls、C-mcpp、本仓库 | 1886/1886 个文件，357 万个 token |
| LLVM、Clang、libc、libcxx 的源码，libc++ 头文件 | 10012/10012 个文件，2554 万个 token |
| [`corpus/lex`](corpus/lex)：刻意挑的边角情况（续行、未结束的字面量、UCN、UTF-8、坏分隔符、后缀、数字） | 7/7 |

**速度**（release 构建，同进程，最好一次）：C-mcpp 的 207 个源文件共 6.9 MB，本词法器 151 MB/s，Clang 的原始词法器 141 MB/s。命令：`mcxx-lexdump --bench N` 和 `mcxx-probe --tokens --bench N`。

## 预处理：模块单元要的那部分

语料里实际用到的指令就是这些（C-mcpp 自己的源码：`#include`、`#if`/`#ifdef`/`#ifndef`/`#elif`/`#else`/`#endif`、`#define`；C-mcppls：只有 GMF 里的 `#include`）。除此之外再覆盖 `#undef`、`#elifdef`/`#elifndef`、`#error`/`#warning`、`#pragma`（包括 `push_macro`/`pop_macro`）和 `_Pragma`。

- `#include` **只记录，不进入**。在模块单元里，它应该出现在全局模块片段（GMF）里，而 GMF 里的声明不属于这个单元。出现在 purview 里时，记为不确定，门禁 `include` 照常报告。
- **宏由 `macros` 特性控制**：每个 `#define` 都记为 MC3 的 `macros` 事实，门禁引擎按配置决定它是错误、警告还是允许。例如 strict profile 禁止宏，包自己放开后就不报。
- 条件表达式按 intmax_t/uintmax_t 求值：`defined`、`__has_cpp_attribute`（标准属性给出 C++23 的值）、短路、`?:`、逗号。
- 模块行（pp-module、pp-import）：`module;` 打开 GMF；`export module m:p [[attr]];` 是模块声明；`module :private;` 开始私有片段；`import m;`、`import :p;`、`import <h>;`、`import "h";` 都按 import 处理。像 `auto module = import(1);` 这样的行只是普通代码。

**不确定时照实说**：这时 `certain` 为 false，原因作为 note 写进诊断，给出的答案和编译器在一无所知时的一样。以下情况算不确定：

- 条件里用到一个名字，而前面包含过的头文件可能把它定义成宏；
- `__has_include` 之类需要编译器才知道的查询；
- 本预处理器不覆盖的指令（`#line`、`#embed`、`#include_next`、……）。

判断一个名字能不能确定"没有定义"：另一个目标预定义、本目标没有预定义的名字（Linux 上的 `_WIN32`），头文件不会去定义，所以可以确定。文件自己在最后一个 `#include` 之后 `#undef` 或 `#define` 过的名字也可以确定。

**头文件的宏**：本前端不读头文件。宿主手里有这些宏时（编译器可以给出），通过 `PreprocessOptions::header_macros` 传进来，它们从第一个 `#include` 起生效；再设 `header_macros_complete`，不在其中的名字就可以确定不是宏。

**检查**：

- `tools/checks/ppdiff.py`：同一条编译命令，Clang 的 `-E` 输出按行标记取出主文件的部分，和本前端逐 token 比对。结果分四类：
  - `equal`：一致；
  - `uncertain`：不一致，但前端已经说了不确定；
  - `header-macro`：不一致的地方是某个头文件的宏；
  - `certain`：前端说确定，结果却不一致，这是缺陷。

  加 `--header-macros` 时，脚本扮演宿主，头文件的宏由 Clang 给出。2026-09-29 本机的结果：

  | 语料 | 结果 |
  |---|---|
  | C-mcppls | 253/253 equal，有没有 `--header-macros` 都一样 |
  | C-mcpp | 334/334 equal（`--header-macros`）；不加时 187 equal、147 header-macro、0 certain |

- `mcxx-conformance --frontend`：门禁 fixture 的每个文件都经过本前端取事实，再交给同一个门禁引擎。凡是由 `macros`、`includes` 事实判定的特性，发现必须和 fixture 的 `// expect:` 一致（A1.6.2）。

## 不依赖 Clang（A1.6.3）

- `tools/checks/lint.py` 的 clang-exposure 规则覆盖本包。
- `tools/checks/symbols.py` 检查 `mcxx-lexdump` 和测试程序：0 个 Clang/LLVM 符号。
- 依赖只有 `openkal-llvm-runtime`（C++ 运行时）和 `mcxx-msa`（不涉及任何编译器）。

## 测试与生成

- `mcpp test -p modules/frontend`：
  - `test_lex`：难写对的词法情况；
  - `test_preprocess`：标准自己的例子（[cpp.scope] 的例 3–5、[cpp.subst] 里的 `__VA_OPT__`）、按目标取的条件、模块行、不确定的情形、`#error`。
- 重新生成表：
  - `python3 modules/frontend/gen/unicode.py <clang-dev>/clang/lib/Lex/UnicodeCharSets.h > modules/frontend/src/unicode.cppm`
  - `python3 modules/frontend/gen/predefined.py <mcxx> > modules/frontend/src/predefined.cppm`
