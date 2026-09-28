# plugin/remote：进程外插件的插件一侧（`mcxx.plugin.remote`）

一个插件程序由两部分组成：它的插件包，加上下面这个 main：

```cpp
import mcxx.plugin.remote;
int main() { return mcxx::plugin::remote::serve(); }
```

`serve()` 用 MC4 协议 1 通过标准输入输出服务这个程序里注册的 provider：
- 注册方式就是 SDK 的 `Registration`，和静态组合时完全一样。所以同一个插件包既能静态链接进 mcxx，也能作为独立进程运行。
- 它回答 `hello`（协商版本）、`check`、`filter`，收到 `shutdown` 或输入结束时退出。
- 标准输出上只写协议消息（MC4-6.1-2），日志写到标准错误。

测试：`mcpp test -p modules/plugin/remote`，用字符串流模拟宿主，覆盖握手、版本不匹配、check、filter 和错误。
