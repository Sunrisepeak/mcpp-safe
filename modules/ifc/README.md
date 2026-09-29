# ifc：IFC 格式的模块接口（`mcxx.ifc`，MC2）

一个模块单元编译出 BMI 时，MC++ 在 BMI 旁边写一个 `X.ifc`（`X.pcm` → `X.ifc`），内容是这个单元的接口：

- **T1 声明**：MC3 事实里 `local` 为 false 的声明，按事实里的顺序；
- **方言**：这个单元的包用的 profile、目录里每个特性对这个模块的级别、包给命名空间设的级别。

导入方从这个文件得知别的模块暴露了什么、在什么方言下编写，不需要读它的源码（M1.2）。规范是 [`specs/mc2-ifc.md`](../../specs/mc2-ifc.md)。

## 格式

用的是 Microsoft 的 IFC 格式，锁定在 0.43（IFC SDK 0.43.5，来自本仓库的 `index/microsoft`）：

- 声明是真正的 IFC 声明，放在 IFC 作用域里：命名空间和类是 `Scope`，函数、方法、构造和析构函数、字段、变量、别名、枚举各用对应的 sort，参数在函数的 chart 里。任何 IFC 读取方（例如 SDK 自带的 `ifc-printer`）都能读出名字、种类和位置。
- IFC 0.43 没有字段可放的 MC3 信息（entity、限定名、类型文本、模板、各个标志、范围）写在声明上的属性 `[[mcxx::decl(...)]]` 里，通过 `.msvc.trait.decl-attrs` 关联。
- 方言是全局作用域里的一个属性声明：`[[mcxx::mc2("1.1.0"), mcxx::profile("safe"), mcxx::feature("goto", "deny"), mcxx::reexport("m:part"), ...]];`（`reexport` 是单元的 `export import`，1.1.0 新增）。
- 为什么不用 IFC 的 vendor extension：SDK 的读取方遇到 `DeclSort::VendorExtension` 的声明、或者不认识的分区名会直接报错；属性是所有读取方都接受的标准 IFC。

结构体直接用 SDK 的定义，所以布局就是 SDK 的布局；每个结构体写之前清零（包括填充字节），同一个接口总是得到相同的字节。

## 接口

| 函数 | 作用 |
|---|---|
| `interface_declarations(facts)` | 事实里接口要带的声明（非 local） |
| `write(Interface)` / `read(bytes)` | 写成字节 / 读回；读取检查签名、格式版本、内容哈希和每一个偏移，不认识的内容一律报错 |
| `save(path, Interface)` / `load(path)` | 写文件（内容没变就不写，保持文件时间）/ 读文件 |
| `path_for(bmi)` | BMI 旁边的 `.ifc` 路径 |
| `differences(expected, actual)` | 两组声明逐项、逐字段比较的差异，用于往返检查 |
| `interface_for(bmi)` | 某个 BMI 对应的接口：先找旁边的 `.ifc`，没有时按 BMI 内容的 SHA-256 到 store 里找；按 BMI 的大小和时间缓存 |
| `note_written(bmi, ifc)` / `publish()` | 编译写出接口后登记；编译成功结束后（驱动调用）把接口按 BMI 内容复制进 store |
| `use_store(dir)` / `store_directory()` | store 的位置：`MCXX_IFC_STORE`，否则 `$XDG_CACHE_HOME/mcxx/ifc`，否则 `~/.cache/mcxx/ifc`；宿主可以指定自己的 |

读取不用 SDK 的 reader：它遇到坏文件时断言失败会直接结束进程（`ifc_assert` 调用 `exit`），编译器和编辑器读别的模块的文件时不能这样。

## 谁在用

- `mcxx.backend.clang` 的 `:ifc` 分区：编译模块单元并写出 BMI、且没有错误时，在门禁之后写 `.ifc`（`--precompile` 和 `-fmodule-output` 都算）。
- `mcxx-probe --read-ifc X.ifc`：按 MC2 读出来，打印成 JSON（`specs/schema/mc2-interface.schema.json`）；`--ifc X.ifc FILE`：和 FILE 的一次解析逐项比较。

## 测试

```
mcpp test -p modules/ifc                    # 往返、字节布局、坏文件、差异、原样不写
python3 tools/checks/ifc.py --mcxx … --probe … [--corpus … --resource …]
```

`tools/checks/ifc.py` 从 SDK 源码构建 `ifc-printer`，检查方言 fixture（`conformance/ifc/dialect`），并在语料上检查 A1.1.1–A1.1.3：每个接口单元都有 `.ifc`、`ifc-printer` 零错误读完所有 `.ifc`、读回的声明和解析的结果逐项相等。

## 导入方怎样用（M1.2）

Clang 后端收集 MC3 的 `imports` 事实时，对文件里的每个具名模块导入，按编译实际加载的 BMI 调用 `interface_for`，再顺着 `reexport` 找下去：得到每个模块的方言和导出的 T1 声明。特性的规则（`plugin::report_imports`）据此在导入处报出越过方言边界的东西。全程只读 `.ifc`，不读对方源码。

mcpp 会把依赖包和 `std` 的 BMI 从它的构建缓存复制进项目，旁边的 `.ifc` 不会跟着复制，所以编译成功后接口还按 BMI 内容存进 store，导入方旁边找不到时去 store 里找。如果 mcpp 的缓存是更早的 mcxx 构建的，store 里没有对应的接口，导入处会报"无法得知"：删掉那条缓存（例如 `~/.mcpp/build-cache/v1/std/<key>`）重新构建一次即可。

## 已知限制

- MC2 1.0 里声明的类型是 MC3 的文本，不是 IFC 的类型图；静态成员函数写成 `Method`。
- 依赖包的 BMI 如果是 mcpp 从它的构建缓存里恢复的，旁边没有 `.ifc`；导入方从 store 里找（见上）。
