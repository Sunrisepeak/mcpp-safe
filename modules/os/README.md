# os：平台常量（`mcxx.os`）

`linux`、`macos`、`windows` 三个包导出同一个模块 `mcxx.os`，形状完全相同：`Family`、`FAMILY`、`FAMILY_NAME`、`EXECUTABLE_SUFFIX`、`PATH_LIST_SEPARATOR`、`PLATFORM`、`CASE_INSENSITIVE_PATHS`。一次构建的依赖图里只会有其中一个：`mcpp.toml` 的 `[target.'cfg(os = "...")'.dependencies]` 按构建目标选中它，这是编译期的选择。

所以代码用 `if constexpr (mcxx::os::FAMILY == ...)` 读取这些常量，不写 `#ifdef`：既没有宏可以判断，也不需要。三个包之间的差异只在 `<平台>/src/os.cppm` 里。`PLATFORM`（`linux-x64`、`linux-arm64` ……）还取决于架构，来自 [`../arch`](../arch/README.md)。

复制自 mcppls（同一作者），改名为 mcxx。
