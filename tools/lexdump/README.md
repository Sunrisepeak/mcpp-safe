# lexdump：`mcxx-lexdump`

MC++ 自己的前端（[`modules/frontend`](../../modules/frontend/README.md)）的命令行出口，供检查和排查使用。不含 Clang（`tools/checks/symbols.py`）。

| 用法 | 输出 |
|---|---|
| `mcxx-lexdump 文件...` | 每个 token 一行 JSON：`file`、`kind`（Clang 的名字）、`line`、`column`、`text`（原文，续行保留）。和 `mcxx-probe --tokens` 同一格式，`tools/checks/lexdiff.py` 逐行比对 |
| `mcxx-lexdump --bench N 文件...` | 词法分析 N 次，取最好的一次：字节数、token 数、秒、MB/s |
| `mcxx-lexdump --pp [选项] 文件` | 预处理结果，一个 JSON：`certain`、诊断、模块、include、宏、token 的拼写 |
| `mcxx-lexdump --ppdiff [选项] 文件 clang的-E输出` | 和 Clang `-E` 输出的主文件部分逐 token 比对，给出结论（`equal`、`uncertain`、`header-macro`、`certain`）和第一处不同。`tools/checks/ppdiff.py` 调用它 |
| `mcxx-lexdump --syntax [选项] 文件` | 大纲，一个 JSON：每个符号的种类、名字、范围、名字范围、父节点，以及解析诊断。和 `mcxx-probe --symbols` 同一格式，`tools/checks/syntaxdiff.py` 比对 |
| `mcxx-lexdump --fuzz N 文件...` | 每个文件随机截断或插入随机 token、字节，各 N 次，每次都解析并取大纲；进程正常结束就算通过（A1.7.3） |
| `mcxx-lexdump --parse-bench N 文件...` | 词法、预处理、解析全部文件 N 次，取最好的一次（A1.7.4） |
| `mcxx-lexdump --directives 文件` | 文件的指令行（词法器认出来的，不包括原始字符串和注释里的），以及文件自己定义的宏名：`ppdiff.py --header-macros` 由此向 Clang 要头文件的宏 |

选项：`--target T`（取哪个目标的预定义宏）、`-DNAME[=VALUE]`、`-UNAME`、`--header-macros 文件`（`-dM -E` 格式的头文件宏）。
