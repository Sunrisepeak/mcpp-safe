# plugins/examples：插件示例

同一个插件包的两种用法（MC4 §3）：

| 目录 | 包 | 用法 |
|---|---|---|
| [`naming`](naming/) | `mcxx-example-naming`（库，模块 `example.naming`） | 规则本身：声明的名字要是 snake_case（policy 特性 `snake-case-names`）。静态组合：包声明 `naming = { path = ".../plugins/examples/naming" }`，然后运行 `mcxx compose` |
| [`naming-remote`](naming-remote/) | `mcxx-example-naming-remote`（程序） | 同一条规则作为独立进程，走 MC4 协议 1：包声明 `naming = { command = [".../mcxx-example-naming-remote"] }` |

`naming-remote` 可以用任何工具链构建。CI 用 GCC 16 + libstdc++ 构建它（`mcpp build --toolchain gcc@16.1.0`），然后由 `tools/checks/compose.py --remote <程序>` 比较：静态组合的 mcxx 和进程外的 GCC 程序，对同一个文件给出的诊断完全相同（A0.6.2）。

这两个示例不是 workspace 成员，需要在各自目录里构建。
