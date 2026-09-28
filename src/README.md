# src：mcxx 驱动（MC5 v0）

mcpp-safe 的根包，产出 `mcxx` 可执行文件。

```
mcxx c++ <编译参数>        C++ 编译，在进程内完成（可执行文件叫 clang++、c++、g++ 时也是这个模式）
mcxx cc <编译参数>         C 编译（叫 clang、cc、gcc 时也是这个模式）
mcxx check <编译参数>      同一条命令只做检查（-fsyntax-only）：MC++ 的门禁和源码过滤器，加上编译器自己的诊断
mcxx version
```

- 驱动本身不包含任何 Clang 头文件：编译经由 `mcxx.backend.compiler`（门面），今天由进程内的 clang 23.1 实现。
- 链接进来的插件（`plugins/std`、`plugins/libs`）在每一次编译中生效。门禁的级别来自源文件最近的 `mcpp.toml` 里的 `[package.metadata.mcxx]`；设置 `MCXX_AUDIT` 时记录豁免。
- 以后还会加入：`mcxx serve`（M1.3）、`mcxx compose`（M0.6），以及作为 mcpp 工具链的 payload（V0.6、E-XIM-1）。
