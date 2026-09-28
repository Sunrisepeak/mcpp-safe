# plugin：插件核心

插件机制本身在这里，插件的实现在仓库根目录的 [`plugins/`](../../plugins/README.md)。两者分开，第一方插件和第三方插件走同一条路：都只针对 SDK 编写。

| 目录 | 包 / 模块 | 内容 | 状态 |
|---|---|---|---|
| [`sdk`](sdk/) | `mcxx-plugin` / `mcxx.plugin` | 插件作者使用的接口：扩展点、注册表 | v0 |
| `host`（计划） | `mcxx.plugin.host` | `mcxx compose`：按项目声明的插件集合生成临时 workspace，链接出本项目的 mcxx（A0.6.1）；进程外插件的宿主端（A0.6.2） | M0.6 |
| `remote`（计划） | `mcxx.plugin.remote` | 进程外插件那一侧的协议库，不依赖 openkal，任何工具链都能构建 | M0.6 |

## SDK 里的两类扩展点（MC4 v0）

1. **规则（`Rule`）**：读取一个文件的事实（`msa::fact::Facts`），报告发现（`Finding`）。它只说"这里用了某个特性"，不决定这是错误、警告还是允许，那由配置决定（`../features`）。每条规则声明它的特性（`Feature`）：id、类别、所在层、说明、改法、默认级别、是否可豁免，以及在各个 profile 下的级别。
2. **源码过滤器（`SourceFilter`）**：在解析之前拿到文件原文和目标平台（`Target`），可以返回一份替换。替换后的文本必须和原文等长、换行位置不变，这样所有位置都不会变；宿主会检查这一点（`apply_source_filters`）。

注册是静态的：在插件的 `.cppm` 或 `.cpp` 里写 `mcxx::plugin::Registration<MyRule> registration;`。openkal 构建的是静态程序，不能 `dlopen`（V0.5），所以插件随包链接进程序就会生效。

## 怎样写一个插件

```cpp
export module acme.rules;
import std; import mcxx.msa; import mcxx.plugin;

class Rules final : public mcxx::plugin::Rule {
    std::vector<mcxx::plugin::Feature> features_ { { .id = "acme-no-goto", .category = "language", .layer = "syntax",
                                                     .summary = "a goto", .fix = "use structured control flow",
                                                     .default_level = mcxx::plugin::Level::deny } };
public:
    std::string_view name() const override { return "acme.rules"; }
    std::span<const mcxx::plugin::Feature> features() const override { return features_; }
    void check(const mcxx::plugin::Context& c, std::vector<mcxx::plugin::Finding>& out) const override {
        for (const auto& g : c.facts.gotos) out.push_back({ "acme-no-goto", g.range, "`goto " + g.label + "`", g.container });
    }
};
mcxx::plugin::Registration<Rules> registration;
```

第一方的例子见 `plugins/std`（mc++.safe、`[[mcpp::cfg]]`）和 `plugins/libs`（nlohmann::json）。
