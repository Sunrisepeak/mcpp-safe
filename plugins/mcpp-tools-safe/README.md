# mcpp-tools-safe：用其他编译器构建时的 MC++ 门禁（E-PLG-1，M1.5）

一个 mcpp 构建规则包（host module，模块 `mcxx.check`），写法和 mcpp-index 里的 `clangtidy` 规则包一样。用 GCC 或者不是 mcxx 的 LLVM 构建程序时，它把 `mcxx check` 作为 **`role = "check"` 动作**放进构建图：

- 门禁报错时构建失败，报出的位置和用 mcxx 构建时一样；
- 默认和编译并行（`blocking = false`）。设成 `blocking = true` 时包里的编译边要等检查通过，但 mcpp（2026.9.28.2 到至少 2026.9.29.4）不会让模块接口单元的编译边等待 blocking 的检查，它自己的自检随即拒绝构建计划（"compile edge ... does not wait for 'mcpp-actions-...'"），所以只适合没有模块单元的包；
- 源码和 manifest 都没变时，这个检查不会再跑。mcpp 在命令成功时写 stamp。

```toml
# 使用方的 mcpp.toml
[build-dependencies]
mcpp-tools-safe = { version = "0.1.0", host-module = true }
llvm            = { version = "23.1.0-mcxx", tools = ["mcxx"] }   # 提供 mcxx 的工具链包

[package.metadata.mcxx]
profile = "strict"
```

```cpp
// build.mcpp
import std;
import mcpp;
import mcxx.check;
int main() { return mcxx::check::sources() ? 0 : 1; }   // src/ 下的全部源码
```

## 怎样检查

一条边：`mcxx check -p ${mcpp.compile_db} --cache <out>/cache 文件...`。

- **输入**：这些文件和 `mcpp.toml`（`[package.metadata.mcxx]` 决定门禁什么）。
- **输出**：一个 stamp。
- **怎样读编译命令**：`mcxx check -p` 用构建数据库里的命令读每个文件，GCC 的命令也可以，语义后端会去掉 GCC 的模块开关（MC5 0.1.1）。
- **模块接口**：文件导入的模块接口由 mcxx 自己构建，放进 cache，下次运行只重建有变化的。

只有一条边，而不是每个文件一条，因为这些文件共用同一批接口，每次运行只构建一遍。

`mcxx` 从这些地方找，依次尝试：

1. `options::program`；
2. 使用方依赖图里 `llvm` 包的 `mcxx` 工具（`mcpp::dep_bin`）；
3. 环境变量 `MCXX`。

需要 mcpp ≥ 2026.8.29.1：从这一版起，check 命令成功时由 mcpp 写 stamp。

## 测试

规则包只会被 build.mcpp 使用（它 `import mcpp`，也就是构建 API），所以不在本仓库的 workspace 里，单独编译它说明不了什么。[`tests/consumer`](tests/consumer) 才是测试：一个用 GCC 构建的模块程序，profile 是 strict。

`python3 tools/checks/tools_safe.py --mcxx <mcxx>` 依次检查：

- **干净构建**：通过，stamp 写出（A1.5.1）；
- **不改动再构建**：什么都不跑（A1.5.3）；
- **加一个 goto**：构建在检查这一步失败，指出 `[goto]`（A1.5.2）；
- **去掉 goto**：再次通过。

加 `--corpus <C-mcpp>` 时，对 C-mcpp 的一份副本（它的编译数据库是 GCC 的）再做一遍：加上规则、禁止 goto、注入一处 goto 让构建失败、去掉后再次通过。
