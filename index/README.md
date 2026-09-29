# index：本仓库自建的 mcpp 包索引（规则 R2）

初期不改官方的 mcpp-index，所需的包由本仓库自己的索引提供。格式与 mcpp-index 相同：`<命名空间>/pkgs/<首字母>/<包名>.lua`。使用方在 workspace 根的 `mcpp.toml` 中接入（只有 workspace 根的 `[indices]` 生效）：

```toml
[indices]
llvm = { path = "index/llvm" }
microsoft = { path = "index/microsoft" }
mcxx = { path = "index/mcxx" }
```

| 命名空间 | 包 | 版本 | 来源 |
|---|---|---|---|
| `llvm` | `clang-dev` | 23.1.0、23.1.0.1、23.1.0.2、23.1.0.3、23.1.0.4 | speak-agent/llvm-clang-dev：Clang/LLVM 23.1 的前端库，由 mcpp 在 openkal 上构建 |
| `llvm` | `codegen-dev` | 23.1.0.1、23.1.0.2、23.1.0.3、23.1.0.4 | 同上，`codegen/`：CodeGen、优化器、x86-64 与 AArch64 后端 |
| `llvm` | `clang-driver` | 23.1.0.2、23.1.0.3、23.1.0.4 | 同上，`driver/`：clang 本身（driver、cc1、cc1as） |
| `mcxx` | `mcpp-tools-safe` | 0.1.0 | 本仓库 tag `0.1.0` 的 `plugins/mcpp-tools-safe`：MC++ 门禁作为 mcpp 构建规则（host module，E-PLG-1、E-IDX-5） |
| `mcxx` | `mcxx-plugins-std`、`mcxx-plugins-libs` | 0.1.0 | 本仓库 tag `0.1.0` 的 `plugins/std`、`plugins/libs`：静态 provider，给 `mcxx compose` 或宿主（mcppls）链接；对 libmc++ SDK 的依赖是压缩包内的路径依赖 |
| `microsoft` | `gsl` | 4.2.0 | microsoft/GSL（纯头文件） |
| `microsoft` | `ifc-sdk` | 0.43.5 | microsoft/ifc：IFC 的读取器、DOM 和 `ifc-printer`（V0.3）。`modules/ifc`（MC2）用它的结构定义和 SHA-256；`tools/checks/ifc.py` 用它的源码构建 `ifc-printer` |

第四段版本号是打包修订号，上游代码版本相同。按版本使用的包会进入 mcpp 的全局构建缓存（`~/.mcpp/build-cache`），所有项目共用同一份构建结果。所以只有内容真的变了才打新版本。
