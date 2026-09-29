# plugin/host：插件的宿主一侧（`mcxx.plugin.host`）

`load(config)` 做以下几件事（插件库见 `library.cpp`：在 Linux 上 mcxx 是导出名字的 `-static-pie` 程序，库的引用先绑定到 mcxx 的 SDK、MSA 和运行时，它注册的 provider 就进了 mcxx 的 Catalog）：
1. 找出包在 `[package.metadata.mcxx.plugins]` 里声明的进程外插件（带 `command` 的条目），每个进程只启动一次，在包的目录里运行。
2. 握手，然后把插件描述的每个 provider 作为代理注册进 Catalog：
   - 规则代理把文件的事实（MC3 JSON）发过去，拿回发现；
   - 过滤器代理把文本发过去。
3. 代理只处理声明它的那个包里的文件。

**故障隔离（MC4 §5）**：插件出了问题，编译器不会跟着崩溃或卡住：
- 处理的情况：
  - 插件崩溃；
  - 超过时间限制（`timeout-ms`，默认 10 s）没有回答；
  - 写出不是 MC4 消息的内容；
  - 协议不符。
- 处理方式：插件进程被杀掉，然后通过 `plugin::Context::fail` 报告，诊断的 code 是 `mcxx-plugin`，写明插件、文件和原因。
- 严重程度：它负责的特性在这个文件里是 deny 时报错误，否则报警告。
- 启动不了或握手失败：它门禁什么都无从知道，这个包的每个文件都报错误（MC4-5-5）。

`set_composed` 和 `composed` 记录当前 mcxx 是和哪些静态插件包组合出来的（`mcxx compose`）。门禁据此发现"声明了、但没有组合进来"的静态插件（MC4-3-4）。

实现使用 POSIX（`posix_spawn`、`poll`），Windows 以后再支持。子进程的标准错误默认丢弃，`MCXX_LOG=plugins=debug` 时显示。

测试：`mcpp test -p modules/plugin/host`：
- 测试程序自己带 `--serve` 参数运行时就是一个插件，这样同一条规则可以分别在进程内和进程外比较结果（MC4-6.3-3）；
- 崩溃、超时、乱码、协议不符、包的作用域，这些故障都用 shell 脚本注入。

## 文件

接口单元只声明，定义在实现单元里（MC5 §8）。

| 文件 | 内容 |
|---|---|
| `src/host.cppm` | 接口：`load`、`Process`、`Session` |
| `src/process.cpp` | 插件进程：标准流接成管道，带截止时间地读，杀掉 |
| `src/session.cpp` | 和一个插件进程的 MC4 会话：握手、请求、关闭 |
| `src/proxy.cpp` | 进程外插件在目录里的代理，按包加载 |
| `src/library.cpp` | 插件库（MC4 §3 `library`）：`dlopen` 装入本进程；装入期间注册的 provider 先扣下，读到库的 `mcxx_plugin_sdk_abi` 与本编译器相同才收进 Catalog，否则丢弃并报告（MC4-3-7） |
