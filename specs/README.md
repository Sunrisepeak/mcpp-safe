# specs：MC++ 的规范（MC1–MC6）

仿照 mcppls 的 S1–S5：每份规范独立编号版本，规范条目带 id，有 JSON Schema 和示例，并且可以追溯到证据。正文用英文写（规范用语 MUST、SHOULD 等按 RFC 2119 解释），这份索引用中文。

| 规范 | 标题 | 版本 | 状态 | Schema |
|---|---|---|---|---|
| [MC1](mc1-features.md) | 特性注册表、profile 与门禁 | 0.2.0 | 草案 | [config](schema/mc1-config.schema.json)、[catalog](schema/mc1-catalog.schema.json)、[audit](schema/mc1-audit.schema.json) |
| MC2 | IFC 方言信息 | — | M1 | — |
| [MC3](mc3-facts.md) | MSA：位置、certainty、事实 | 0.2.0 | 草案 | [facts](schema/mc3-facts.schema.json) |
| [MC4](mc4-plugins.md) | 插件：provider、扩展点（规则、源码过滤器、profile、属性、区域）、组合、进程外协议 | 0.2.0（协议版本 1） | 草案 | [protocol](schema/mc4-protocol.schema.json) |
| [MC5](mc5-driver.md) | `mcxx` 驱动与工具链契约 | 0.1.1 | 草案 | [version](schema/mc5-version.schema.json) |
| [MC6](mc6-serve.md) | `mcxx serve`：作为进程的语义服务 | 1 | 草案 | [requests](schema/mc6-requests.schema.json) |

## 各规范之间的关系

```
mcpp.toml [package.metadata.mcxx]  ── MC1：配置、profile、优先级、豁免、审计
        │
mcxx（MC5：命令行、环境变量、version --json）
        │  每次编译：源码过滤器 → 解析 → 事实（MC3）→ 门禁（MC1）
        ▼
provider（MC4）：内置的 mc++.iso 与插件；静态组合或进程外协议；解析、覆盖、冲突、故障隔离
```

## 目录

| 路径 | 内容 |
|---|---|
| `mc*.md` | 规范正文 |
| `schema/` | JSON Schema（draft 2020-12） |
| `examples/` | 示例，每一个都能通过对应的 schema。`mc1-catalog.json`、`mc5-version.json`、`mc1-audit.jsonl` 是真实的 mcxx 输出 |
| `CHANGELOG.md` | 每份规范、每个版本的变更 |

## 规范条目 id 与可追溯性

- 每一条用大写 RFC 2119 关键字（MUST、MUST NOT、REQUIRED、SHALL、SHOULD、SHOULD NOT、RECOMMENDED）写成的要求，都在出现的位置带一个 id：`MC<n>-<节>-<序号>`，以上标和锚点显示，例如 [`MC1-6-1`](mc1-features.md#MC1-6-1)。
- id 是稳定的：新条目取所在节的下一个序号，删掉的条目的 id 不再使用。
- [`conformance/traceability.json`](../conformance/traceability.json) 把每个 id 对应到证据，证据有以下几种：
  - `specs.py` 的检查；
  - 单元测试；
  - 门禁 fixture；
  - 源码中执行这条要求的那一行；
  - 极少数情况下，说明为什么无法自动验证。
- 尚未实现的写在 `$pending` 里，写明缺什么。每次运行都会列出来，直到有了证据。

## 检查（A0.8.1）

```
python3 tools/checks/specs.py                        # 需要 jsonschema 包
python3 tools/checks/specs.py --mcxx <mcxx 可执行文件>  # 另外检查真实的 mcxx 输出
```

`specs.py` 检查以下内容：
- JSON 都能解析，schema 都合法；
- 示例都能通过 schema：MC1 的 TOML 示例读成 JSON 后等于 JSON 示例；
- schema 必须拒绝的反例；
- catalog 和 MC1 第 3 节的特性表、第 4 节的 profile 表一致；
- 带 `--mcxx` 时：真实的 `mcxx features --json`、`mcxx version --json` 的输出，未知子命令的退出码，一个特性在 `--precompile` 加 `-c` 两步中只报一次，干净文件不输出任何内容；
- 相对链接能解析；
- 每条要求都有 id，id 唯一，每个 id 都有真实存在的证据。

## 变更规则

- 修改一份规范时，正文、schema、示例和受影响的 fixture 在同一个提交里一起改。
- MINOR 版本只增加可选字段；不兼容的修改要升 MAJOR 版本，并写迁移说明。
- MC4 的协议版本是整数，由握手协商（MC4 §6.2）。
