# backend/compiler：编译门面（`mcxx.backend.compiler`）

mcxx 驱动通过它执行一条编译命令：输入命令行，得到退出码。今天由进程内的 clang 23.1 实现（`../clang-compiler`），驱动本身不包含任何 Clang 头文件。

```cpp
import mcxx.backend.compiler;
namespace compiler = mcxx::backend::compiler;
compiler::run(argc, argv);                          // argv[0] 表示模式：…/clang++ 为 C++
compiler::run_as("c++", argv[0], args);             // "c++" 或 "c"
compiler::mode_for_name("clang++");                 // "c++"：以编译器的名字被调用
```

编译过程中，链接进本程序的全部插件都会生效：源码过滤器（例如 `[[mcpp::cfg]]`）在解析前运行，门禁规则在解析后运行，详见 `../clang/README.md`。
