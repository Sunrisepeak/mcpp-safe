# frontend：MC++ 自己的前端（`mcxx.frontend`，F1）

和 Clang 后端并列、不依赖 Clang/LLVM 的 C++ 前端（计划 P4）。F1 做三件事：

- 词法分析；
- 模块单元需要的那部分预处理（翻译阶段 4）；
- 声明级的语法树和大纲；
- 完整语法树（M3.0）：函数体、初始化器、默认实参、枚举值里的语句和表达式，按需读取，不影响大纲的快速路径。

它给出的 MC3 事实交给同一套门禁引擎。

| 分区 | 内容 |
|---|---|
| `:lex` | C++23 的原始词法分析（不做预处理），逐条照 Clang 23.1 的原始词法器（`lib/Lex/Lexer.cpp`）：token 种类用 Clang 的名字（`raw_identifier`、`l_paren`、`utf8_string_literal`……），便于逐个比对 |
| `:unicode` | 标识符可用的 Unicode 字符（XID_Start / XID_Continue，UAX #31，加上 Clang 作为扩展接受的数学记号）。**生成的**：`gen/unicode.py` 读所钉版本 Clang 的 `UnicodeCharSets.h`，Unicode 版本和参照一致（18.0） |
| `:predefined` | 三个目标（linux-x64、macos-arm64、windows-x64）的预定义宏，**生成的**：`gen/predefined.py` 跑 `mcxx c++ -target T -std=c++23 -dM -E` |
| `:preprocess` | 预处理：条件编译、文件自己的宏（`#`、`##`、`__VA_ARGS__`、`__VA_OPT__`、GNU 的 `, ## __VA_ARGS__`）、`#include` 记录、模块声明和 import 的识别 |
| `:syntax` | （接口单元只声明；每个分区的定义在同名的 `.cpp` 里，`:syntax` 的在 `parser.cpp` 和 `outline.cpp`）声明：namespace、class/struct/union、enum 和枚举项、函数（函数体按括号跳过）、变量、成员、别名、concept、模板，每个都有名字和整体范围，构成一棵树；`symbols()` 给出和 MSA `Unit::symbols()` 一样的大纲 |
| `:types` | 声明的类型文本：按写出来的 token，照 Clang 的 TypePrinter 打印保留语法糖的类型（`const std::string &`、`char *const *`、`int[3]`、`void (*)(int)`、`std::function<void (int)>`、`unsigned long`、constexpr 变量带 const）。推导出来的类型（`auto`、类模板实参推导）和不是字面量的数组界（Clang 打印它的值）不猜，给空；参数的 `auto`（泛型 lambda、简写函数模板）照写出来的打印，和 Clang 一样 |
| `:lookup` | 名字查找（[basic.lookup]）：函数的局部变量按所在块、参数、模板参数、类的成员（成员函数体里全部可见，连同基类的）、外层命名空间（在使用处之前声明的）、无名命名空间、using 指令和 using 声明、命名空间别名，限定名从左往右解析，成员访问的对象推类型：名字（变量、参数、成员、`this`）、调用（函数的返回类型，MC3 0.5.0）、下标（指针、数组、标准序列的元素，map 的值）、`T { }`、`T( )`、具名转换、`make_shared`/`make_unique`、`std::chrono` 的转换、`(*x)`；`auto` 从初始化器或 range-for 的范围推出；`->` 穿过指针和 `std::unique_ptr`/`shared_ptr`/`weak_ptr`/`optional`，以及类自己的 `operator->`（`std::expected`），`[]` 也用类自己的 `operator[]`；**特化的成员按实参读**：成员的类型用类模板的参数写成（`std::expected<R, E>::error` 是 `const _Err &`），参数绑定到对象类型写出的实参，没写的绑到默认实参（MC3 0.6.0 的 `template-parameters`，本文件的模板读它的模板头）；别名展开到它指的类型（别名模板带着实参，特化的成员别名带着特化的实参）；重载的返回类型不同时按实参选（个数、字面量的种类、已知类的对象，MC3 0.7.0 的 `parameters`），恰好一个合适才回答，返回类型写成函数模板参数的取对应实参的类型（`std::move` 取实参）；外层命名空间和无名命名空间里同名的函数、std 和它再导出的 C 库函数交给重载决议；`auto` 的初始化器顶层有二元或条件运算符时不推（最后一个操作数的类型不是整体的），`*e` 取所指、`&e` 取指针、lambda 是闭包类型；指定初始化器 `T { .n = v }`；导入的别名在它自己的作用域里找它指的类（名字里的内联命名空间跳过），导入类的成员也在它的基类里找（MC3 `bases`）。`names_of` 是 MC3 限定名的唯一来源（局部类的成员经函数的参数类型命名，和 Clang 一样）。解不出的不猜：成员访问找不到时给一个 `certain == false` 的条目（`why`：`deduced` 推导出的返回类型、`unknown` 对象的类说不出、`member` 知道类却没有这个成员），服务把它们交给 Clang 后端（A2.2.3）；依赖名、只有 ADL 找得到的不给 |
| `:declared` | 声明的类型解析（A2.1.2）：MC3 的 `templates` 和 `pointer` 照 Clang 的 `collect_templates`/`holds_pointer` 在规范类型上算——特化先列模板，再列各个类型实参的，默认实参也算，参数包不展开；依赖类型只列写出来的；别名看它指的（导入的别名用接口里 Clang 算好的列表，类模板的成员别名在特化里按实参展开）；注入类名不列，类型文本按 Clang 打印成 `Box<T>`。推导出来的类型（`auto`、结构化绑定、类模板实参推导）文本不打印——Clang 打印的是推导经过的语法糖——但推得出时它的模板和指针照算；解析不了的名字、特化的成员类型说"不确定"（`declared_types()` 的 `*_certain`、`why`）|
| `:ast` | 完整语法树的节点（M3.0）：语句、表达式、类型、名字、块里和类里的声明，各有一个稳定的句柄（`ast::Handle`：种类加下标），`Tree::get(handle)` 取回节点；每个节点带着它跨的 token，范围按 Clang 的算法。`dump()` 把树打成一行文字，`children()` 给出任意节点的子节点，`validate()` 检查树自己是否一致，`feature_of()` 给出节点对应的 MC1 特性 id。定义在 `ast.cpp`（名字、`dump`、范围）、`ast_walk.cpp`（`children`、`validate`）、`syntax_features.cpp`（特性 id 表） |
| `:bodies` | 完整解析的入口 `parse_bodies(syntax, options)`，和它的两个接口：语义反馈 `NameOracle`（名字是类型、模板还是值？）和语言扩展点 `SyntaxExtension`（C++26 的语法是一个扩展，`cpp26_syntax()`）。定义在 `bodies.cpp`（驱动、光标、试探解析）、`oracle.cpp`（默认的反馈：名字查找） |
| `:bodyparser` | （不导出）递归下降解析器，按关注点分在 `parse_name.cpp`（名字、模板实参）、`parse_expr.cpp`（表达式）、`parse_lambda.cpp`（lambda、requires 表达式）、`parse_type.cpp`（声明说明符、声明符、类型）、`parse_decl.cpp`（块和类里的声明、函数体）、`parse_stmt.cpp`（语句）、`syntax26.cpp`（C++26 的语法） |
| `:resolver` | （不导出）`:lookup` 和 `:declared` 共用的名字查找和表达式定型状态；定义分在 `lookup.cpp`（作用域、名字查找）、`typing.cpp`（表达式的类型、特化的成员、别名展开）、`declared.cpp` |
| `mcxx.frontend` | 上面几部分，再加 `facts(pp)` 和 `facts(syntax)`：MC3 事实，形状和 Clang 后端给的一样（见下文"事实与快速门禁"） |

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

## 语法：声明和大纲，和 Clang 的一致

解析器不知道类型。C++ 在需要类型才能区分的地方，按代码的形状决定：

- 声明符之前的名字是类型；
- 在声明之后，括号里的内容默认是参数表。只有第一个参数在默认值之前出现了只有表达式才会有的东西（字面量、`nullptr`、`true`、运算符），才当作初始化；
- 类作用域里的函数是成员；`X::f` 形式的定义，除非 `X` 是本文件里的 namespace，也算成员。

和 Clang 大纲一致的细节（从对比中得出）：

- 类模板的构造函数和析构函数带上模板参数（`Box<T, N>`）；
- 别名模板的名字位置是 `using`；
- 类内的 `= default`、`= delete`、`= 0` 计入范围，类外定义的 `S::S() = default` 不计入；
- 匿名 namespace、匿名类和匿名枚举本身不进大纲，它们里面的内容也不进；
- 友元不进大纲；
- 位置按 Clang 的算法：宏展开得到的记号，位置是最外层调用的起点；名字的终点是起点加上拼写长度；整体范围的终点是最后一个记号所在处（展开得到的记号就是宏名）的文件记号长度，算在它起始的那一行上，哪怕这个记号跨了多行（跨行的原始字符串）。

**总能给出一棵树**：解析不下去的地方，跳到本层的下一个 `;`，或者跳过一个配平的块，记一条诊断，然后继续。不配对的 `}` 和只出现在括号里、外面没有花括号的 `;` 会结束跳过，一处坏代码不会吞掉后面的声明。

**检查**：`tools/checks/syntaxdiff.py`。一边是 Clang 后端的大纲（`mcxx-probe --symbols`，也就是编辑器文档大纲用的 `Unit::symbols()`），另一边是本前端（`mcxx-lexdump --syntax`），逐个符号比较种类、名字、名字范围和整体范围。2026-09-29 本机的结果：

| 条件 | 结果 |
|---|---|
| A1.7.1 零解析失败 | C-mcppls 253 个文件、C-mcpp 334 个文件，都没有解析诊断 |
| A1.7.2 一致率 ≥ 99.9% | C-mcppls：4725/4725 个符号一致（100%）。C-mcpp：宿主给出头文件的宏时（`--header-macros`，gtest 的 `TEST` 展开成类），18880/18880 一致（100%）；不给时，测试以外的 198 个文件是 99.942% |
| A1.7.3 模糊测试 | `mcxx-lexdump --fuzz 1000`：随机截断，或插入随机 token 和字节。C-mcppls 253 000 次、C-mcpp 334 000 次，没有崩溃，每次都给出大纲；调试构建另做 25 300 次 |
| A1.7.4 冷启动解析 ≤ 2 s（4 核） | C-mcppls 整个语料（252 个文件，2.6 MB）单线程 0.15–0.25 s（release，词法、预处理、解析全算） |

## 完整语法树（M3.0）

大纲不读函数体：`parse()` 把每个函数体、初始化器、默认实参、枚举值、`static_assert` 当作一段 token 记在 `Syntax::parts`（每段属于一个大纲里的声明），`symbols()` 和 mcppls 的快速路径不为此多付任何代价。要语句和表达式时再问一次：

```cpp
const auto syntax = mcxx::frontend::parse(text, { .file = path });
const auto tree = mcxx::frontend::parse_bodies(syntax, { .imported = &imported });   // 每段 part 一个根：tree.roots
for (const auto& root : tree.roots) std::println("{}", mcxx::frontend::ast::dump(tree, root.node));
```

**节点和句柄**（A3.0.4）：每种节点在自己的数组里，句柄是（种类，下标），同一个文件、同样的选项得到同样的句柄；`tree.get(handle)` 取回节点，`^^x` 这样的反射节点持有被反射实体的句柄。节点是几个槽位加一个种类的结构，种类决定槽位的意思（写在 `ast.cppm` 每个种类的旁边）。覆盖面：C++23 的全部语句（含 `if constexpr`、`if consteval`、init-statement、条件里的声明、范围 for、try/catch、协程语句）和表达式（全部优先级、四种具名转换、C 风格和函数式转换、`new`/`delete`、lambda 的全部捕获形式、requires 表达式的四种要求、折叠表达式、指定初始化器、`sizeof...`、`noexcept`、`typeid`）、类型（声明说明符、声明符按从里到外的顺序推出类型：指针、引用、数组、函数、成员指针、`auto`、`decltype`、包展开）、名字（限定名、模板实参、运算符函数、析构函数、转换函数）、块里的声明（变量、结构化绑定、别名、局部类和枚举，类的成员和构造函数初始化器、友元、using）、GNU 的语句表达式、`&&label`、`__builtin_*`。

**消歧用语义反馈**，不再只按代码形状：当代码有两种读法（`a < b > (c)`、`T * x;`、`(T) - x`、`T x(a)`、`f<int>(x)`）时，解析器先问自己的作用域（本体里声明的局部变量、参数、模板参数），再问 `NameOracle`（默认是 `:lookup` 和导入的接口），都不知道才按代码形状决定，规则和大纲解析器用的相同。谁决定的都记下来（`ast::Decision`，按 `Basis`：`syntax`、`scope`、`lookup`、`feedback`、`shape`；`Tree::stats.by_basis` 有计数，`record_decisions` 保留每一条，trace 类别 `frontend.syntax` 逐条打印）。宿主可以给自己的 `NameOracle`（`BodyOptions::oracle`）。

**语言扩展点**（MC4，路线 §4）：核心只读 C++23。C++26 的语法——`^^`、`[: :]`（表达式、类型、嵌套名的前缀）、`template for`、`pre`/`post`、`contract_assert`、`T...[n]`、`= delete("why")`、`consteval { }`、结构化绑定包——由 `SyntaxExtension` 读，`syntax26.cpp` 里的 `cpp26_syntax()` 是 `plugins/lang/cpp26` 将来提供的东西；`BodyOptions::extensions = {}` 就是纯 C++23。扩展通过 `SyntaxContext` 看到光标、调用文法的产生式（`assignment_expression()`、`type_or_expression()`、`statement()`……）、往树里加节点；核心里没有任何 C++26 的特判，测试里另写了一个核心之外的扩展（`unless (c) s;`）。

**每个节点的 MC1 特性 id**（C3/D2）：`feature_of(node)`。已在目录里的（`mc++.iso` 的 `new-delete`、`reinterpret-cast`、`c-style-cast`、`const-cast`、`rtti`、`exceptions`、`goto`、`asm`、`union`、`c-array`、`c-varargs`，C++26 包的 `c++26:reflection`、`contracts`、`pack-indexing`、`expansion-statements`、`delete-with-reason`、`structured-bindings-can-introduce-pack`、`structured-binding-declaration-as-condition`、`variadic-friends`）直接用；目录里还没有的 C++11–23 构造和 GNU 扩展按目录的命名提议（`c++11:lambda`、`c++20:concepts`、`ext:gnu-statement-expression`……），`syntax_features()` 列出每一个，说明它是否已注册。门禁诊断还没有接到解析里（需要 MC1 目录先加这些 id）。

**错误恢复**：不用异常。一个产生式失败就设 `failed_`，所有产生式随即返回，最近的恢复点（一条语句、一段 part）把它变成一条诊断和一个 `error` 节点，跳到下一个 `;` 或一个配平的块之后继续；试探解析（声明还是表达式、类型还是表达式）失败时把光标、树、作用域、诊断全部退回。任意文本都给出一棵一致的树（`mcxx-lexdump --fuzz`：750 个变异的输入，没有崩溃，每棵树都通过 `validate`）。

**检查**：

| 工具 | 内容 | 结果（2026-10-02 本机） |
|---|---|---|
| `tools/checks/bodies.py` | 三个语料每一段 part 都读成语句和表达式：读了多少、几段留下诊断、错误节点、树的结构问题、各层做了多少决定 | 带宿主给的头文件宏（`--header-macros`）：C-mcppls 224 个文件 3065 段、C-xlings 229 个文件 2272 段、C-mcpp 346 个文件 27059 段，**零失败、零错误节点、零结构问题**；不带头文件宏时 C-mcpp 的 6737 段里有 3 段失败，全是 gtest 的 `TEST` 没展开 |
| `tools/checks/bodies.py` 之外的压力 | libc++ 顶层头文件 141 个文件 6213 段 0 失败；clang/lib/AST 84 个文件 6444 段 5 失败（`PRIx32` 之类头文件里的宏）；没有崩溃、没有超时 | |
| `tools/checks/bodiesdiff.py` | 语句和表达式的种类、范围，和 Clang 的 AST（`-Xclang -ast-dump=json`）比：`corpus/bodies` 里四个自包含的文件（表达式、语句、模板、其余） | 1653 个节点，一致 100%（Clang 的隐式节点、实例化重复、没有字节的节点不算，见脚本开头） |
| `mcxx-lexdump --fuzz` | 随机截断、插入 token 和字节，大纲、事实、完整解析都跑 | 750 次，0 崩溃，0 个树不一致 |
| `tools/checks/lint.py` | `modules-only`：核心里没有 `#include` | 通过 |

和语料对比的第二半（种类和范围在语料上和 Clang 的 AST 比）需要 Clang 后端给每个文件的显式语句和表达式：`mcxx-probe --syntax-nodes` 应该对每个文件打印 `{"nodes": [{"class": Stmt::getStmtClassName(), "op": 运算符的写法, "range": [起点偏移, 终点偏移)}]}`，只含函数体里显式的节点（`isImplicit` 的、`ImplicitCastExpr` 之类和被包装的同范围节点不要），模板只给模式不给实例化。`bodiesdiff.py` 的 `clang_nodes` 就是这个的 JSON 版本；映射表（`MINE`）和过滤规则已经写在脚本里。

**速度**（release，同一批文件，最好一次，机器负载很高所以只作量级）：C-mcppls 224 个文件 2.2 MB，大纲 0.060 s（36 MB/s），完整解析另加 0.099 s（22 MB/s）；C-xlings 3.1 MB，0.073 s 加 0.100 s；C-mcpp 的非测试源码 6.4 MB，0.176 s 加 0.212 s。大纲路径只多记 part（一个 16 字节的结构）。

**还没有做的**：函数体之外的表达式（模板头里的默认实参和约束、`decltype` 之外的类型里的表达式、属性的实参、`noexcept(...)` 以外的说明符，以及没有被大纲当作 part 的声明）；整个文件的解析（现在由大纲解析器决定哪些是函数、哪些是初始化器，它认错的形状——比如推导指引——函数体就读不到，已修的是 `T (&a)[N]` 和 `T (C::*)()` 这两种参数）；属性的结构（只记范围）；模板的实例化和重载决议（M3.3、M3.5）；常量求值器接口（M3.6，AST 里已有 `throw`、`try`、`handler` 节点）；C++29 的语法（目前的清单里没有需要新语法的论文，`pack-indexing-template-names` 的 `T...[n]<args>` 还没读）。

## 事实与快速门禁（M1.8）

`facts(syntax)` 给出语法层就能读出的 MC3 事实，容器和限定名的算法和 Clang 后端一样（省略 inline namespace）：函数里声明的东西（参数、局部变量、lambda 的参数和 init-capture）只用名字本身，和 Clang 一样；`exported` 按词法上的外层传下来（参数、成员、局部变量都算），和 Clang 的 `isInExportDeclContext` 一样；结构化绑定是一个变量，名字是 `[a, b]`，位置在 `[`；lambda 里声明的是局部的，不论 lambda 在哪里；条件里的声明要有初始化器（`if (a && b)` 是表达式），只有 catch 的参数可以止于 `)`；别名模板的事实在它的名字处（大纲在 `using`，和 Clang 一样）；无名参数在声明符之后的那个 token，无名 namespace 在它的 `{`，都和 Clang 放的位置一样。

`tools/checks/declsdiff.py` 逐个成员对照 Clang 后端（M2.1 的度量）：C-mcppls 220 个文件，Clang 的 16232 个声明配上了 16221 个（99.93%），没有多出来的；限定名 99.95%，所属命名空间、`exported`、`local`、c-array、union、c-variadic 100%；名字查找（`tools/checks/refsdiff.py`，C-mcppls 220 个文件，2026-09-30）：目标声明在本文件里的 41346 个引用，相同 99.80%，报为不确定 0.06%；目标在别处（本模块的接口、其他模块、std）的 48652 个，从导入模块的 MC2 接口读到声明（1.2.0 起包括经 using 声明可达的，1.3.0 起包括基类和函数的返回类型，1.4.0 起包括模板参数，1.5.0 起包括函数的参数），相同 96.71%，报为不确定 2.82%；其中 std 的 12328 个相同 97.70%；两类合计指向别的只有 2 个（0.002%）。`test_lookup`：60 个名字，期望值全是 Clang 对同一段源码的回答。类型（`:declared`，declsdiff 带上编译命令里的模块文件）：类型文本相同 82.97%，不同 0，其余 17.03% 是推导出来的，报为不确定；模板列表相同 92.76%（起点 42%），不同 0，不确定 7.24%；指针相同 92.78%，不同 0。类型文本不算作 F1 收集了 `declaration_types`：需要类型的门禁不由 F1 判定。

| 种类 | 内容 |
|---|---|
| `declarations` | 包括函数的参数和局部变量。`pointer`、`c-array`、`c-variadic`、`union` 按写出来的判断：类型模板实参里的 `*`、声明符里的 `*`、`[ ]`、`, ...`。本文件里的别名会展开；导入模块导出的别名由宿主通过 `Known::aliases` 传入（`exported_aliases()` 取自接口文件）。`va_list` 按目标区分：x86-64 Linux 上是数组，macOS 和 Windows 上是指针 |
| `gotos`、`allocations`、`uses` | `goto`、`new`/`delete`（带 `[]`）、`throw`/`try`/`typeid`/`asm`/`va_arg`，函数体、初始化器、lambda 里都算 |
| `casts` | 只有具名转换（`static_cast` 等四个）。C 风格和函数式转换要知道类型，这里不给 |
| `suppressions` | `[[mcpp::allow("id, id2", "理由")]]`，范围是所在声明的整体范围，豁免因此和 Clang 路径一致 |
| `macros`、`includes` | 来自预处理 |

另外，`import_annotations(text)` 读出导入上的 `[[mcpp::allow(...)]]`（M1.2），`blank_import_annotations` 把它们换成空格、位置不变：Clang 不接受导入上的属性，宿主在 Clang 读文件之前用它。导入带进来什么要看对方的 BMI 和 `.ifc`，这一层读不到，所以快速门禁里 `imports` 不算作需要的事实，由解析后的结果补上。

**门禁 fixture**：`mcxx-conformance --frontend`。由本前端取事实、同一个门禁引擎判定，13 个特性给出的发现和 fixture 完全一致：

- asm、c-array、c-varargs、const-cast、exceptions、goto、include、macros、new-delete、raw-pointers、reinterpret-cast、rtti、union。

另有 C 风格转换（9 处）、按位重解释的 C 风格转换（3 处）"没有类型就看不到"，属于预期；没有误报。

**快速门禁（A1.8.3）**：`msa::Workspace::quick(path, text)`，Clang 后端用本前端实现。编辑器里每次修改，`mcxx.lsp` 的 Service 立刻（在解析之前）发布本前端判定的那些特性的发现，同时带上上次解析的其他诊断；这个版本解析完成后，再由完整的诊断取代。`mcxx serve` 端到端测试（`modules/serve` 的 test_serve）：一次修改同时加入 goto、new/delete、reinterpret_cast、写出的 `*` 声明符和宏，五类发现在 4 ms 后到达（调试构建；要求 ≤ 100 ms）。mcppls 的 mcxx 引擎用的就是这个 Service。

## 不依赖 Clang（A1.6.3）

- `tools/checks/lint.py` 的 clang-exposure 规则覆盖本包。
- `tools/checks/symbols.py` 检查 `mcxx-lexdump` 和测试程序：0 个 Clang/LLVM 符号。
- 依赖只有 `openkal-llvm-runtime`（C++ 运行时）、`mcxx-msa` 和 `mcxx-base`（都不涉及任何编译器）。

## 可观测

完整解析的决定（每一次"类型还是表达式"、"模板实参还是小于号"、"声明还是表达式"，谁决定的）、试探解析为什么失败、查找说某个名字是什么，也在 `frontend.syntax`（debug 级）；`mcxx-lexdump --bodies [--dump] [--nodes] [--decisions] 文件` 把整棵树（或每个节点的种类和范围）和各项计数打出来，`--fragment '语句;'` 读命令行上的几条语句。

解析器的判断记在 trace 的 `frontend.syntax` 类别（debug 级）：`MCXX_LOG=frontend.syntax=debug mcxx-lexdump --facts 文件` 逐条打印记录的声明（种类、名字、位置、父声明）、每个 `{` 被当作语句块、lambda 体还是初始化器、控制语句括号的范围、跳过的地方。是否打开在每次解析开始时问一次（`tracing_`），关着时每个 trace 点只是一次分支，参数不求值；`trace::enabled` 本身在低于所有类别的级别时只做一次原子读。

## 测试与生成

- `mcpp test -p modules/frontend`：
  - `test_lex`：难写对的词法情况；
  - `test_preprocess`：标准自己的例子（[cpp.scope] 的例 3–5、[cpp.subst] 里的 `__VA_OPT__`）、按目标取的条件、模块行、不确定的情形、`#error`；
  - `test_syntax`：类的成员（构造、析构、运算符、转换、字段、静态）、namespace 的嵌套和匿名、枚举、模板和别名和 concept、类外定义、参数表还是初始化、`export` 和 `extern "C"`、出错后继续；
  - `test_bodies_expr`、`test_bodies_stmt`、`test_bodies_decl`、`test_bodies_cpp26`、`test_bodies_tree`（M3.0，共 57 个用例、432 条断言）：表达式（优先级、结合性、转换、`<` 是模板实参还是小于、折叠、new/delete、初始化列表）；语句和块里的声明（声明还是表达式、结构化绑定、局部类、作用域、错误恢复、乱文本不死循环）；part（构造函数初始化器、function-try-block、`= default`/`delete`/`0`、默认实参、枚举值、模板的体）、lambda、requires、类型；C++26 的每一种语法及其特性 id，和没有扩展时核心只读 C++23；树本身（句柄取回节点、每个节点都能从根走到、范围、决定由谁做、宿主给的 `NameOracle`、导入带来的模板、核心之外写的语言扩展、trace）。期望值是解析器打出来的树，逐条看过。
- 重新生成表：
  - `python3 modules/frontend/gen/unicode.py <clang-dev>/clang/lib/Lex/UnicodeCharSets.h > modules/frontend/src/unicode.cppm`
  - `python3 modules/frontend/gen/predefined.py <mcxx> > modules/frontend/src/predefined.cppm`
