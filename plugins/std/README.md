# plugins/std：MC++ 标准插件（`mcxx-plugins-std`）

## `mcxx.plugins.policy`：mc++.policy

不对应某一项 ISO 特性的使用规定，以及一个库控制的样例。默认都是 `allow`，也不在任何内置 profile 里，由包自己决定。ISO 语言特性（goto、union、C 数组、指针算术……）由 MC++ 内置的 `mc++.iso` 控制，见 `modules/features/README.md`。

| 特性 id | 类别 | 捕获什么（事实） |
|---|---|---|
| `raw-pointers` | policy | 声明的类型里有裸指针：变量、成员、参数、返回类型、别名，以及模板实参里的 `T*`。引用、`nullptr_t`、`sizeof(T*)` 不算 |
| `lib:std.vector` | library | 声明的类型里出现了 `std::vector`（库控制的样例；需要 `declaration_types` 这类事实） |

"能不能用指针"就是一个编译器功能配置的例子：

```toml
[package.metadata.mcxx.features]
raw-pointers = "deny"

[package.metadata.mcxx.namespaces."app::ffi"]     # 和 C 接口打交道的地方例外
raw-pointers = "allow"
```

## `mcxx.plugins.cfg`：`[[mcpp::cfg(...)]]`

和 Rust 的 `#[cfg(...)]` 一样：一段声明只在某些目标上存在。这是一个 **extension**，MC++ 专有：每一处使用都报告为特性 `ext:cfg` 的发现，默认 `allow`；`profile = "portable"`（只允许 ISO C++）时是错误。

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
