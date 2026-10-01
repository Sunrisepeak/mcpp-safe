# plugins/flow：控制流插件（`mcxx-plugins-flow`）

阶段 Ⅰ 的 MF.3（总路线 `.agents/docs/2026-10-02-mcxx-roadmap-full-frontend.md`）：只有看函数的控制流才能判定的未定义行为来源，放在 MC1 的 `flow` 层。它们读 MC3 0.9.0 的 `control-flow` 事实，用插件 SDK 的分析 pass（`mcxx.plugin:flow`，MC4 0.5.0）。

| 特性 id | 类别 | 捕获什么 | 标准 |
|---|---|---|---|
| `uninitialized-read` | pitfall | 一个局部变量，在某条路径上还没有被写过就被读：标量没有初始化，或者平凡默认构造的类 | [basic.indet] |
| `missing-return` | pitfall | 返回值不是 `void` 的函数（`main`、协程、构造和析构函数除外），有一条路径走到了函数末尾 | [stmt.return] |
| `noreturn-returns` | pitfall | `[[noreturn]]` 函数有一条路径会返回：一个 `return` 语句，或者走到末尾 | [dcl.attr.noreturn] |

- 三项都加入 profile `safe`（级别 `deny`），因为 `safe` 针对的就是未定义行为的来源。其他情况下默认 `allow`，由包自己决定。
- 一条路径如果以 `return`、`throw` 或者不返回的调用（`[[noreturn]]`）结束，就不算走到末尾（MC3-4.14-5）。
- 取了地址、绑定到引用、作为成员调用的对象之后，变量被视为可能已经写过（MC3 §4.14 的 `address` 事件）。
- 模板本身（没有实例化的模式）的控制流不计算（`known` 为 false），这里不判定（MC4-2-10）。

```toml
[package.metadata.mcxx]
profile = "safe"          # 三项都是 deny

[package.metadata.mcxx.features]
uninitialized-read = "warn"
```

事实从哪里来：
- 阶段 Ⅰ 由 Clang 后端的 `:flow` 分区生成（Clang 的 `CFG`）；
- 阶段 Ⅱ 由自研前端的 MCIR 生成同样的事实（M4）。

插件只依赖 MSA，切换时不用改。
