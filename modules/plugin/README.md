# plugin：插件核心

**MC++ 本身就是一个插件系统。** MC++ 的内置功能和第三方插件都是 SDK 上的 *provider*，使用同一套接口：

- MC++ 内置的 ISO C++ 特性控制是内置 provider `mc++.iso`，在 [`../features`](../features/README.md) 里实现。
- 第一方插件在 [`plugins/`](../../plugins/README.md)。
- 第三方插件放在它们自己的包里。

插件可以做三件事：

- **控制**：给任意类别的特性加门禁。
- **扩展**：用源码过滤器增加 C++ 没有的东西。
- **覆盖**：替换某个特性（包括 MC++ 内置的）的实现、整个 provider，或者一个 profile。

| 目录 | 包 / 模块 | 内容 | 状态 |
|---|---|---|---|
| [`sdk`](sdk/) | `mcxx-plugin` / `mcxx.plugin` | provider 的接口、注册、解析（Catalog）、失败报告（`Context::fail`） | v0 |
| [`wire`](wire/README.md) | `mcxx-plugin-wire` / `mcxx.plugin.wire` | MC3 事实与 MC4 消息的 JSON 格式 | v0 |
| [`remote`](remote/README.md) | `mcxx-plugin-remote` / `mcxx.plugin.remote` | 进程外插件的插件一侧：`serve()` | v0（协议 1） |
| [`host`](host/README.md) | `mcxx-plugin-host` / `mcxx.plugin.host` | 进程外插件的宿主一侧：启动、握手、代理、超时与崩溃隔离；记录组合进来的静态插件 | v0（协议 1） |

静态组合 `mcxx compose` 在 [`../driver`](../driver/README.md)。

## 扩展点（MC4 v0）

所有扩展点都派生自 `Provider`，它有四个方法：

| 方法 | 说明 |
|---|---|
| `name()` | 这个 provider 的名字 |
| `features()` | 能门禁的特性 |
| `profiles()` | 定义的 profile |
| `replaces()` | 被它整个取代的 provider 名字 |

| 扩展点 | 做什么 | 例子 |
|---|---|---|
| **规则**（`Rule`） | 读取一个文件的事实（`msa::fact::Facts`），报告发现（`Finding`）。它只说"这里用了某个特性"，不决定这是错误、警告还是允许，那由配置决定 | `mc++.iso`、`mc++.policy`、`mcxx.plugins.json` |
| **源码过滤器**（`SourceFilter`） | 在解析之前拿到原文和目标平台（`Target`），可以返回一份等长、换行不变的替换，宿主会检查这一点。也可以把自己扩展特性的使用报告成发现 | `[[mcpp::cfg]]`（报告 `ext:cfg`） |
| **profile**（`Profile`） | 一组级别的名字：可以包含别的 profile，可以给一整个类别定级别，也可以按 id 给任何 provider 的特性定级别。特性也可以通过 `Feature::profiles` 自己加入某个 profile | `safe`、`modules`、`strict`、`portable`、`acme.device` |
| **属性**（`AttributeSpec`） | 认领 `[[ns::name]]`：编译器接受它，每一处使用记为 MC3 事实（`fact::Attribute`：名字、参数、所在声明）；规则用 `plugin::subtree` 取得这个声明范围内的事实 | `[[acme::hot]]`（`plugins/examples/device`） |
| **区域**（`AttributeSpec::region`） | 指定了 profile 的属性：在它所在的声明范围内，这个 profile 的级别更严时就用它的（MC1 §6） | `[[acme::device]]`：范围内禁止异常、RTTI、new/delete |

### 特性的类别：全部对插件开放

| 类别 | 含义 | 减法后还是 ISO C++ 吗 | 例子 |
|---|---|---|---|
| `iso` | 一项 ISO C++ 语言特性，用标准的 stable name 标明（`Feature::standard`） | 是 | `goto` [stmt.goto]、`uninitialized` [basic.indet] |
| `policy` | 怎样使用语言的规定，不对应某一项 ISO 特性 | 是 | `raw-pointers`：能不能用裸指针 |
| `library` | 程序可以用哪些库设施 | 是 | `lib:std.vector` |
| `pitfall` | 库或语言里的坑 | 是 | `json-brace-init` |
| `extension` | MC++ 在 C++ 之上加的东西，属于 MC++ 专有，用了它代码就离不开 MC++ | 否（profile `portable` 全部禁止） | `ext:cfg` |

### 覆盖

| 写法 | 效果 |
|---|---|
| `Feature::replaces = true` | 提供一个别人已经提供的 id，并取而代之（MC++ 内置的也可以）。例如用一个知道"先写后读"的流分析替换 `mc++.iso` 的 `uninitialized`：`mc++.iso` 就不会再被问到这个特性 |
| `Provider::replaces()` 返回 `{"mc++.iso"}` | 取代整个 provider：它的规则不再运行，它的特性和 profile 由新的 provider 提供，或者不再存在 |
| `Profile::replaces = true` | 重新定义一个同名 profile |

**冲突**：同一个 id 有两个 provider，而且都没有声明 `replaces` 时，优先用内置的，其次用先注册的。另一个不会被使用，冲突记入 `Catalog::problems`，并作为警告报出。所以不会悄悄地被别人换掉。

## 性能：可控、可扩展、可覆盖，但不丢性能

| 机制 | 作用 |
|---|---|
| **Catalog** | 注册在静态初始化时完成，之后解析一次。有新的注册时重建（`generation`） |
| **Plan**（`../features`） | 一个包的配置对照 Catalog 解析一次，按 manifest、修改时间和 Catalog 缓存 |
| `Context::wants(id)` | 规则只会被问到在这个文件里不是 `allow` 的特性，其余的不用检查 |
| `Feature::needs`（`msa::fact::Kinds`） | 宿主只收集被问到的特性需要的事实。声明的类型文本单独作为一类（`declaration_types`），因为它最花时间 |
| `Feature::requires_declaration` | 例如 `"nlohmann"`：文件里没有声明也没有导入这个名字时，跳过这个特性 |
| 什么都不用问时 | 不遍历 AST |

实测数据见 [`.agents/docs/development.md`](../../.agents/docs/development.md) 的"门禁的开销"一节。

## 怎样写一个插件

```cpp
export module acme.rules;
import std; import mcxx.msa; import mcxx.plugin;

namespace plugin = mcxx::plugin;

class Rules final : public plugin::Rule {
    std::vector<plugin::Feature> features_ { { .id = "acme-no-goto", .category = plugin::Category::policy, .layer = "syntax",
                                               .summary = "a goto", .fix = "use structured control flow",
                                               .default_level = plugin::Level::deny,
                                               .needs = mcxx::msa::fact::Kinds::gotos } };
public:
    std::string_view name() const override { return "acme.rules"; }
    std::span<const plugin::Feature> features() const override { return features_; }
    void check(const plugin::Context& c, std::vector<plugin::Finding>& out) const override {
        if (!c.wants("acme-no-goto")) return;
        for (const auto& g : c.facts.gotos) out.push_back({ "acme-no-goto", g.range, "`goto " + g.label + "`", g.container });
    }
};
plugin::Registration<Rules> registration;
```

注册是静态的。openkal 构建的是静态程序，不能 `dlopen`（V0.5），插件随包链接进程序就会生效。MC++ 自己的 provider 用 `Registration<R, Origin::builtin>` 注册。

`mcxx features [--json]` 列出这个程序里的全部内容：

- provider 以及它来自内置还是插件；
- 每个特性的类别、默认级别、stable name，以及它替换了谁；
- 每个 profile 包含哪些特性；
- 冲突。

第一方的例子：

- `modules/features/src/iso.cppm`（`mc++.iso`）；
- `plugins/std`（`mc++.policy`、`[[mcpp::cfg]]`）；
- `plugins/libs`（nlohmann::json）。

覆盖的例子在 `modules/features/tests/test_override.cpp`。
