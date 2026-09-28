# arch：架构常量（`mcxx.arch`）

`x86_64` 和 `aarch64` 两个包导出同一个模块 `mcxx.arch`：`Arch` 和 `ARCH`。os 包用 `[target.'cfg(arch = "...")'.dependencies]` 选中其中一个，所以它和 `mcxx.os` 一样是编译期常量，用 `if constexpr` 读取，不是宏。唯一的使用方是 `mcxx.os` 的 `PLATFORM`。

复制自 mcppls（同一作者），改名为 mcxx。
