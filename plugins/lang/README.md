# plugins/lang：更新的 C++ 标准的核心语言特性

里程碑 ML（方案文档 §ML）：C++26、C++29 已进入工作草案的核心语言特性（不含标准库），每一代一个包、一个插件，按文件开启。两个包互相独立，都只依赖插件 SDK（`mcxx.plugin`），可以只组合其中一个。

| 目录 | 包 | 插件 | 特性 | 编译参数 |
|---|---|---|---|---|
| [`cpp26`](cpp26/README.md) | `mcxx-plugins-lang-cpp26` | `mcxx.plugins.lang.cpp26` | 54 篇提案，`c++26:<名字>` | `-std=c++2c` |
| [`cpp29`](cpp29/README.md) | `mcxx-plugins-lang-cpp29` | `mcxx.plugins.lang.cpp29` | 17 篇提案，`c++29:<名字>` | `-std=c++2d` |

**怎么开启。** 每篇提案是一个 MC1 特性，类别是 `standard`（MC1 0.5.0）：id 以标准名开头（MC1-2.1-5），`standard` 字段是提案编号，默认级别是 deny，也就是默认关闭（MC1-2.1-6）。在包、profile 或文件 glob 上把它设成 deny 以外的级别就是开启：

```toml
[package.metadata.mcxx.features]
"c++26:pack-indexing" = "allow"

[package.metadata.mcxx.files."src/meta/**"]
"c++26:reflection" = "allow"
```

一个文件只要开启了某一代里的任意一个特性，这个插件（MC4 0.4.0 的 language provider）就给这个文件的编译命令加上这一代的 `-std`；命令里是 GNU 方言时加 `gnu++` 形式。命令本身已经要求同一代或更新的标准时什么都不加，不会降级。没有开启任何特性的文件保持命令原样。`standard` 类特性不靠事实判定，所以默认关闭时不会让文件被遍历。

**特性表。** `src/table.cppm` 是生成的：[`cxx_status.py`](cxx_status.py) 读 MC++ 所用 Clang（llvm.clang-dev 23.1）源码里的 `clang/www/cxx_status.html`，把每篇核心语言提案连同它在这个 Clang 里的状态写进表：

- `baseline`：Clang 23.1 已经实现，`-std` 就能打开；
- `partial`：Clang 23.1 只实现了一部分，剩下的由 MC++ 的 Clang 补上；
- `todo`：由 MC++ 的 Clang 实现（llvm-clang-dev 里 `lang/cpp26/<特性>` 这样的分支）。

`partial` 和 `todo` 特性在 MC++ 的 Clang 里有了自己的编译选项以后，加到 `arguments()` 里标出的位置：只给开启了这个特性的文件加。

```
python3 plugins/lang/cxx_status.py .deps/src/llvm-project-23.1.0.src/clang/www/cxx_status.html
```

WG21 通过新提案、Clang 的页面跟进以后，重新运行一次即可。几篇提案共用一个标题（反射的六篇、错误行为的两篇），或者标题太长，它们的名字写在脚本的 `NAMES` 里。

**组合。** 和其他插件一样，链接这个包就会注册它的插件。今天它们还没有放进 mcxx 的根 `mcpp.toml`。
