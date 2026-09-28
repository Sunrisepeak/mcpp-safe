# driver：mcxx 驱动（`mcxx.driver`，MC5）

mcxx 的全部命令都在这里，以库的形式提供：
- 仓库根包 `src/main.cpp` 只有一行，调用 `run()`，并把 MC++ 的标准插件（`plugins/std`、`plugins/libs`）链接进来；
- `mcxx compose` 生成的程序也调用 `run()`，另外链接包自己的静态插件。

| 命令 | 作用 |
|---|---|
| `mcxx c++` / `cc` / `check` | 编译或检查（MC5 §2），经由 `mcxx.backend.compiler` |
| `mcxx features [--json]` | Catalog（MC1 §10） |
| `mcxx compose [--manifest 文件]` | 构建本包的编译器：MC++ 驱动、标准插件，加上包声明的静态插件（MC4 §3） |
| `mcxx version [--json]` | MC5 §5 |

## 静态组合（A0.6.1）

- **生成**：`mcxx compose` 在 `~/.cache/mcxx/compose/<key>/` 生成一个 mcpp 包（也可以用 `MCXX_COMPOSE_CACHE` 或 `XDG_CACHE_HOME` 指定位置），交给 `mcpp build` 构建，然后链接到 `<key>/bin/mcxx`。
  - `<key>` 由三部分哈希得到：mcxx 版本、MC++ 源码位置、静态插件集合。
  - 生成的文件只在内容变化时才重写，所以插件集合和源码都没有变时不会重新链接（MC4-3-3）。
- **编译时的切换**：编译一个声明了静态插件的包里的文件时，如果当前 mcxx 不是为这个插件集合组合出来的：
  - 已经有组合好的编译器，就 `execv` 它，参数不变；
  - 没有，就报错，并提示运行 `mcxx compose`（MC4-3-4）。
- **实测**：第一次组合约 5 分钟。Clang 库来自全局缓存，但 libmc++ 的路径依赖要为新程序重新构建一次。之后不变时约 6 s，不重新链接。
- **端到端检查**：`tools/checks/compose.py --mcxx <mcxx>`。

MC++ 源码的位置：`MCXX_SOURCE_ROOT`，否则就是构建这个驱动时的源码树。
