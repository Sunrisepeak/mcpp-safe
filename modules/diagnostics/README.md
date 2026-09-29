# diagnostics：两种视角的诊断（`mcxx.diagnostics`，MC5 §9）

编译期检查的报错，给人看的和给 AI agent 看的不一样：

- **human**：照 Rust 的报错排版——`error[代码]: 说明`、` --> 文件:行:列`、源码行和下划线，然后 `= help:`（改成什么、在这里怎样豁免）和 `= note:`（这个级别是在哪里设的：profile、包、模块、文件、命名空间）。终端上有颜色。
- **agent**：每条诊断一个 JSON 对象，一行（`specs/schema/mc5-diagnostic.schema.json`）：稳定的代码、MC3 的范围（从 0 起）、级别和它的出处、修改建议和豁免写法（都是可以直接写进代码的文本）、编译器的 note 和 fix-it。
- **clang**：编译器自己的格式，不动（构建日志、IDE 的 problem matcher）。

`mcxx c++ --mcxx-diagnostics=human|agent|clang ...` 选择；驱动把它作为 `MCXX_DIAGNOSTICS` 传下去，构建工具不方便加参数时也可以直接设这个环境变量。都没有时，终端上用 human，否则用 clang。

这个包不涉及编译器：后端（`mcxx.backend.clang` 的诊断 consumer）把 Clang 的诊断和 MC++ 门禁的发现变成 `Item` 交给它；`mcxx check -p` 直接用 MSA 的诊断。

## 文件

接口单元只声明，定义在实现单元里（MC5 §8）。

| 文件 | 内容 |
|---|---|
| `src/diagnostics.cppm` | 接口：`View`、`Item`、`human`、`agent`、`render` |
| `src/view.cpp` | 选择视角：选项、环境变量、是否终端 |
| `src/human.cpp` | human 视角 |
| `src/agent.cpp` | agent 视角 |

## 测试

`mcpp test -p modules/diagnostics`：两种视角各自的内容，门禁发现和编译器自己的诊断（带 note 和 fix-it）。端到端（真实编译、schema 校验）在 `tools/checks/specs.py --mcxx`。
