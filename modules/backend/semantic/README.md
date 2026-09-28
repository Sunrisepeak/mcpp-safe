# backend/semantic：语义门面（`mcxx.backend`）

使用方（mcppls 的 mcxx 引擎、服务、工具）通过它拿到一个 `msa::Workspace`，以及后端的基本信息（`msa::BackendInfo`：名字、版本、semantic kit 应当使用的 libc++ 版本）。接口里只有 `msa::` 类型，不暴露是哪个后端在回答。

```cpp
import mcxx.backend;
auto workspace = mcxx::backend::make_workspace(options);
const auto about = mcxx::backend::info();
```

- 依赖：`mcxx-msa`、`mcxx-backend-clang`（今天的实现）。
- 不含任何 Clang 头文件（`clang-exposure` 规则）。
