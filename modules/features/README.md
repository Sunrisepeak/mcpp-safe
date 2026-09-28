# features：特性门禁（`mcxx.features`，MC1 v0）

把插件规则的发现变成错误、警告、豁免或者什么都不报，依据是这个特性在出现位置的级别。它不依赖任何后端，只读事实和已注册的规则。

## 配置

写在包的 `mcpp.toml` 里。mcpp 保留 `[package.metadata.*]`，但不解释其内容：

```toml
[package.metadata.mcxx]
profile = "safe"                                    # 插件声明的一组级别

[package.metadata.mcxx.features]                    # 整个包
"json-brace-init" = "deny"
goto = "warn"

[package.metadata.mcxx.modules."app.legacy"]        # 一个模块（分区沿用它所属模块的设置）
goto = "allow"

[package.metadata.mcxx.namespaces."app::detail"]    # 一个命名空间及其内部的命名空间
reinterpret-cast = "allow"
```

级别有三档：`allow`、`warn`、`deny`（也可以写 `off`、`warning`、`error`）。无法识别的级别和写错的值会作为警告报出，不会被静默忽略。

## 优先级（A0.3.3）

声明上的 `[[mcpp::allow("id")]]` > 命名空间 > 模块 > 包 > profile > 特性的默认级别。

## 豁免与审计（A0.3.4）

- `[[mcpp::allow("id")]]` 或 `[[mcpp::allow("id", "原因")]]` 写在声明上，覆盖这个声明的范围。
- 每一次豁免都记入 `Result::waived`；设置 `MCXX_AUDIT=<文件>` 时，每次豁免追加一行 JSON：特性、文件、行列、所在声明、原因。
- 不可豁免的特性（`Feature::waivable = false`）即使写了豁免也照样报错，并说明它不可豁免。
- 豁免中写了没有任何规则声明过的 id，会报警告并给出位置。

## 诊断（A0.7.5）

code 是特性 id；消息包含特性 id、改法和豁免方法，例如：

```
error: `wrapped` is list-initialized from one value: it holds that value in a one-element array, not a copy of it [json-brace-init]; write `Json x = value;` for a copy, or `Json::array({ value })` for an array; to allow it here, [[mcpp::allow("json-brace-init")]] on the declaration
```

测试在 `plugins/*/tests` 中进行，因为测门禁需要有规则。
