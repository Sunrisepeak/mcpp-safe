# plugins/std：MC++ 标准插件（`mcxx-plugins-std`）

## `mcxx.plugins.safe`：mc++.safe v0（M0.7）

MC++ 安全方言要门禁的特性。默认全部 `allow`；`profile = "safe"` 时全部为 `deny`；包、模块、命名空间可以各自覆盖（见 `modules/features/README.md`）。

| 特性 id | 类别 | 捕获什么（事实） |
|---|---|---|
| `raw-pointer-arithmetic` | language | 指针的 `+ - += -= ++ --`，以及对指针的 `[]`（C 数组自己的下标不算） |
| `new-delete` | language | `new`、`delete`、`new[]`、`delete[]` |
| `reinterpret-cast` | language | `reinterpret_cast`，以及实际做了重解释的 C 风格或函数式转换（经由 `void*` 的不算） |
| `c-array` | language | 类型为 C 数组的变量、成员、参数 |
| `goto` | language | `goto`、间接 `goto` |
| `macros` | language | 本文件里的宏定义 |
| `union` | language | 联合体的定义 |
| `lib:std.vector` | library | 声明的类型里出现了 `std::vector`（库控制的样例） |

## `mcxx.plugins.cfg`：`[[mcpp::cfg(...)]]`

和 Rust 的 `#[cfg(...)]` 一样：一段声明只在某些目标上存在。

```cpp
[[mcpp::cfg(windows)]] void open_console() { AllocConsole(); }   // 在 Linux 上根本不会被编译
[[mcpp::cfg(unix)]] void open_console() {}
[[mcpp::cfg(all(target_arch = "x86_64", not(debug_assertions)))]] void fast_path();
[[mcpp::cfg(feature = "simd")]] void simd();                      // mcpp 的 feature：MCPP_FEATURE_SIMD
export [[mcpp::cfg(target_os = "linux")]] int use_epoll();
```

- 键（`target_` 前缀可省略）：`target_os`、`target_family`、`target_arch`、`target_env`、`target_pointer_width`、`target_endian`、`feature`。裸名字：`windows`、`unix`（家族），`linux`、`macos`、`ios`、`android`、`freebsd`、`wasi`（系统），`debug_assertions`（没有定义 `NDEBUG`）。组合：`all(...)`、`any(...)`、`not(...)`。
- 实现是源码过滤器：解析之前，目标不满足的声明被替换成空白（换行保留），编译器看不到它，之后所有位置都不变；目标满足时只去掉属性本身。前面的 `export` 和其他 `[[...]]` 随声明一起清除。
- 适用于以 `;` 或代码块结束的声明和语句。不适用于 `import`：构建的依赖扫描在任何编译器插件运行之前就读取了 import。
- 写错的谓词会在属性所在位置报错。

## 测试

`mcpp test -p plugins/std`：规则和过滤器只用事实与文本测试，不依赖 Clang。经过 Clang 的用例在 `modules/backend/clang/tests`。
