# xpkgs：本仓库自建的 xlings 包（E-XIM-1、V0.7 的 xlings 一半）

| 文件 | 包 | 作用 |
|---|---|---|
| [`pkgs/m/mcxx.lua`](pkgs/m/mcxx.lua) | `mcxx:mcxx@0.1.0` | mcxx 作为 llvm 族工具链的 payload，mcpp 以 `llvm@23.1.0-mcxx` 使用它（V0.6，MC5 §7），mcpp 本身不需要任何修改 |

## 安装（linux-x64、macOS arm64、Windows x64）

mcpp 用自己的 xlings（`~/.mcpp/registry`），所以包要注册到那里：

```
export XLINGS_HOME=~/.mcpp/registry
$XLINGS_HOME/bin/xlings config --add-xpkg xpkgs/pkgs/m/mcxx.lua
MCXX_BINARY=<构建出的 mcxx> \
MCXX_CLANG_HEADERS=<llvm.clang-dev 的 llvm/clang/lib/Headers，例如 .mcpp/.xlings/data/xpkgs/llvm-x-clang-dev/23.1.0.4/*/llvm/clang/lib/Headers> \
    $XLINGS_HOME/bin/xlings install mcxx:mcxx@0.1.0 -y
```

之后在任何 mcpp 项目里都可以用：`mcpp build --toolchain llvm@23.1.0-mcxx`。端到端检查见 `tools/checks/toolchain.py`。

macOS 和 Windows 上（E-XIM-3）步骤相同，`MCXX_BINARY` 是为该平台交叉构建的 mcxx（在 Linux 上 `mcpp build --target aarch64-macos` / `--target x86_64-windows-gnu`），依赖的是该平台的 xim:llvm 22.1.8。区别：

- 配置文件里的 `--target` 取自 xim:llvm 自己的 `clang++ -dumpmachine`（macOS 上是 `arm64-apple-darwin…`，Windows 上是 `x86_64-pc-windows-msvc`，mcpp 在那里就是为 MSVC 构建的），不是 mcxx 自己构建时的 MinGW 目标。mcpp 只在 Linux 上绕过配置文件（`--no-default-config`，见下文），在 macOS 和 Windows 上读它，所以这个目标起作用。
- macOS 上 mcxx 是复制过去再 ad hoc 签名的（在别处构建的程序不签名在 arm64 上启动不了），其余名字链接到它。
- Windows 上 payload 的目录是 junction、文件是硬链接（不能链接时复制），都不需要特权；卸载或重装时先解除这些 junction，删除永远不会进到 xim:llvm 的 payload 里。登记到 `xim-x-llvm/23.1.0-mcxx` 的也是 junction。payload 里的工具另有一个不带 `.exe` 的名字（同一个文件的硬链接）：LLVM 在 openkal 的 Windows 目标上按 Linux 构建，按名字找要启动的程序（`-fuse-ld=lld` 的 `lld-link`）时不加扩展名。

CI（`.github/workflows/ci.yml` 的 `mcxx-cross` 和 `xpkg`）在 Linux 上交叉构建两个平台的 mcxx，在 windows-2022 和 macos-14 上用 xlings 安装它、用 mcpp 构建并运行一个模块程序。

## payload 的内容

payload 采用 xim:llvm 的布局，mcpp 就会把它当作 llvm 族的工具链：

| 路径 | 内容 |
|---|---|
| `bin/clang++`、`bin/clang` | mcxx（硬链接）。驱动按被调用时的名字选择模式（MC5-2-2） |
| `bin/clang++.cfg`、`clang.cfg` | xim:llvm 22.1.8 的配置（glibc、内核头文件、libc++、lld、compiler-rt），前面加上 `--target=x86_64-unknown-linux-gnu` |
| `lib/clang/23/include` | Clang 23.1 的内置头文件 |
| 其余全部 | 链接到 xim:llvm 22.1.8：libc++ 及其 std 模块源码、compiler-rt、lld、binutils、clang-scan-deps |
| `.mcpp_ok` | mcpp 的安装标记。没有它，mcpp 会尝试从索引安装 `xim:llvm@23.1.0-mcxx`，而索引里没有这个包 |

安装后，payload 还会登记到 mcpp 查找 llvm 工具链的位置：`xim-x-llvm/23.1.0-mcxx` 是一个指向本包的链接，卸载时删除。

## 与 mcpp 相关的三点

- **默认目标**：mcpp 在 Linux 上做本机构建时会传 `--no-default-config`，这样 payload 配置里的 `--target` 就不起作用了。所以 mcxx 的默认目标必须是 x86_64-unknown-linux-gnu，这从 llvm-clang-dev 23.1.0.3 起配置：宿主仍然是 openkal 的 musl，默认目标改为 linux-gnu。
- **std 模块缓存**：mcpp 的全局 std 模块缓存（`~/.mcpp/build-cache/v1/std`）按工具链的路径和版本作为键，不看编译器本身。所以重新构建 mcxx 并重新安装 payload 后，要删掉引用 `23.1.0-mcxx` 的缓存项：`grep -rl 23.1.0-mcxx ~/.mcpp/build-cache/v1/std`。
- **构建类型**：mcxx 目前是 dev 构建（未优化），作为工具链编译会慢一些。
