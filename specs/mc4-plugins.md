# MC4 — Plugins: Providers, Extension Points, Composition and the Out-of-Process Protocol

| | |
|---|---|
| Specification | MC4 |
| Version | 0.1.0 (protocol version 1) |
| Status | Draft |
| Schema | [`schema/mc4-protocol.schema.json`](schema/mc4-protocol.schema.json) |
| Examples | [`examples/mc4-session.jsonl`](examples/mc4-session.jsonl), [`examples/mc4-plugins.toml`](examples/mc4-plugins.toml) |
| Implementation | `modules/plugin/sdk` (providers, catalog), `modules/features` (the built-in provider, gates), `modules/plugin/host` and `modules/plugin/remote` (composition, the protocol) |
| License | Apache-2.0 |

## Abstract

MC++ is a plugin system: MC++'s own feature controls and every plugin are **providers** written against one SDK. A plugin can control (gate features of any category, MC1), extend (transform source text before it is parsed), and override (provide a feature, a whole provider or a profile instead of another, MC++'s own included). This specification defines providers and their extension points, how a compiler resolves the providers it has into one catalog, the two ways a plugin reaches a compiler -- statically composed into it, or as a separate program speaking a versioned protocol -- and what happens when a plugin fails. Both ways have the same semantics: a rule gives the same findings for the same facts either way.

## 1. Conventions

- The key words **MUST**, **MUST NOT**, **REQUIRED**, **SHALL**, **SHALL NOT**, **SHOULD**, **SHOULD NOT**, **RECOMMENDED**, **MAY** and **OPTIONAL** are to be interpreted as described in BCP 14 (RFC 2119, RFC 8174) when, and only when, they appear in all capitals.
- JSON is as defined by RFC 8259, encoded in UTF-8. Facts are MC3 §4.11; features, profiles and levels are MC1.
- The **host** is the program that runs plugins: `mcxx`, or a program that links libmc++ (mcppls).

## 2. Providers and extension points

A provider has a `name`, unique among the providers a host has; the features it declares (MC1 §2); the profiles it defines (MC1 §4); and the names of providers it replaces (§4). <a id="MC4-2-1"></a><sup>MC4-2-1</sup> MC++'s built-in provider is named `mc++.iso`. A provider offers one or more extension points:

| Extension point | In | Out | When |
|---|---|---|---|
| **rule** | a file's path, its module, the facts asked for (MC3), and the ids it is asked for | findings: `feature`, `range`, `message`, `container` | after the file is parsed |
| **source filter** | a file's path, its text, and the target (§2.1) | a replacement text or none, problems, findings of the filter's own features | before the file is parsed |
| **profile** | -- | named sets of levels (MC1 §4) | when a configuration is resolved |

- A rule MUST report findings only for features it declares, and SHOULD report only those it is asked for. <a id="MC4-2-2"></a><sup>MC4-2-2</sup>
- A rule decides whether code uses a feature; it MUST NOT decide the level: that is the configuration's (MC1 §6). <a id="MC4-2-3"></a><sup>MC4-2-3</sup>
- A source filter's replacement MUST have the text's length and a line break exactly where the text has one; a host MUST refuse, and report, a replacement that does not, so that every position stays where it was. <a id="MC4-2-4"></a><sup>MC4-2-4</sup>
- A feature declares the kinds of facts it is decided from (`needs`) and MAY name a declaration without which it cannot occur (`requires-declaration`); a host SHOULD collect only the facts the features it asks for need, and SHOULD NOT ask a feature whose required name the file neither declares nor imports. <a id="MC4-2-5"></a><sup>MC4-2-5</sup>

### 2.1 The target

What a source filter knows about the target, in the words a configuration uses: `triple`; `os` (`linux`, `windows`, `macos`, `ios`, `android`, `freebsd`, `wasi`, `none`, ...); `family` (`unix`, `windows` or `""`); `arch` (`x86_64`, `aarch64`, ...); `env` (`gnu`, `musl`, `msvc`, ...); `pointer-width`; `endian` (`little`, `big`); `features`: the build's active features, each as its macro name without the `MCPP_FEATURE_` prefix; `debug-assertions`: `NDEBUG` is not defined.

## 3. Composition: how a plugin reaches a compiler

| Way | What a plugin is | How it runs |
|---|---|---|
| **static** | an mcpp package that imports `mcxx.plugin` and registers its providers at static initialization | linked into a compiler with libmc++; in process, no serialization |
| **out of process** | a program that speaks the protocol of §6, built with any toolchain | the host starts it and talks to it over its standard input and output |

A host MUST NOT load plugins with `dlopen`: MC++'s compilers are static programs. <a id="MC4-3-1"></a><sup>MC4-3-1</sup>

A package declares the plugins its code is compiled with in `[package.metadata.mcxx.plugins]` ([`examples/mc4-plugins.toml`](examples/mc4-plugins.toml)):

```toml
[package.metadata.mcxx.plugins]
acme-rules = { path = "tools/acme-rules" }               # static: an mcpp package (or version = "1.2.0")
gcc-lint   = { command = ["tools/gcc-lint/bin/gcc-lint"], timeout-ms = 5000 }   # out of process
```

- An entry with `path` or `version` is static; one with `command` is out of process; an entry with both or neither is an error. <a id="MC4-3-2"></a><sup>MC4-3-2</sup> The entry's name is how the package calls the plugin; a `version` entry's name is the plugin's package name, and a `path` entry's package is whatever the manifest at the path names.
- **`mcxx compose`** builds the package's compiler: a temporary mcpp workspace holding a program that links the MC++ driver, its standard plugins and the package's static plugins, cached under a hash of that set; it MUST NOT relink when the set and the plugins' sources are unchanged. <a id="MC4-3-3"></a><sup>MC4-3-3</sup>
- A compiler invoked for a file whose package declares static plugins it does not have MUST run the composed compiler for that package when there is one, and otherwise MUST fail and say to run `mcxx compose`; it MUST NOT compile without them. <a id="MC4-3-4"></a><sup>MC4-3-4</sup>
- Out-of-process plugins are started by the host that needs them, in the directory of the manifest that declares them; a relative `command[0]` is relative to that directory. <a id="MC4-3-5"></a><sup>MC4-3-5</sup>

## 4. Resolution and overriding

A host resolves every provider it has into one **catalog** before it gates anything, and again when a provider is added:

1. A provider that another provider names in its `replaces` is removed, with its features and profiles. <a id="MC4-4-1"></a><sup>MC4-4-1</sup>
2. For each feature id, of the remaining providers that declare it: the one whose feature says `replaces`; if several do, the first of them, and a conflict; if none does, MC++'s built-in provider if it is one of them, else the first registered, and every other one is a conflict. <a id="MC4-4-2"></a><sup>MC4-4-2</sup>
3. Profiles by name, the same way (a profile's `replaces`). <a id="MC4-4-3"></a><sup>MC4-4-3</sup>

- A provider whose feature is not chosen MUST NOT be asked for it. <a id="MC4-4-4"></a><sup>MC4-4-4</sup>
- Every conflict MUST be reported -- in the catalog's `problems` (MC1 §10) and as a warning of every gated compilation -- and never resolved silently. <a id="MC4-4-5"></a><sup>MC4-4-5</sup>
- Only MC++'s built-in provider defines features of category `iso`; a plugin's `iso` feature MUST replace one (MC1-2.1-1). <a id="MC4-4-6"></a><sup>MC4-4-6</sup>

## 5. Failures

A plugin that crashes, exits, does not answer within its time limit, or answers what this specification does not allow MUST NOT make the compilation crash or hang. <a id="MC4-5-1"></a><sup>MC4-5-1</sup> The host reports, at the file, which plugin failed, on which file, and why; the features that plugin was asked for are then not checked for that file. <a id="MC4-5-2"></a><sup>MC4-5-2</sup> The report is an error when one of those features is at `deny` for the file (a gate that could not be checked does not pass), and a warning otherwise. <a id="MC4-5-3"></a><sup>MC4-5-3</sup> The diagnostic's code is `mcxx-plugin`. <a id="MC4-5-4"></a><sup>MC4-5-4</sup> A plugin a package declares that cannot be started or does not complete its handshake, or a static plugin the compiler was not composed with, gates what is unknown: every file of the package gets an error in a compilation (an editor, which cannot compose, MAY make it a warning). <a id="MC4-5-5"></a><sup>MC4-5-5</sup>

## 6. The out-of-process protocol (version 1)

### 6.1 Transport

The host starts the plugin's `command` with no arguments beyond those given, and exchanges **messages**: JSON objects, one per line (no line break inside a message), on the plugin's standard input (host to plugin) and standard output (plugin to host). <a id="MC4-6.1-1"></a><sup>MC4-6.1-1</sup> The plugin's standard error is its log; a host MAY show it. The plugin MUST NOT write anything but messages to its standard output. <a id="MC4-6.1-2"></a><sup>MC4-6.1-2</sup> Every message has `type`; a request has an `id`, an integer unique within the session, and its answer carries the same `id`. <a id="MC4-6.1-3"></a><sup>MC4-6.1-3</sup> Every message MUST validate against [`schema/mc4-protocol.schema.json`](schema/mc4-protocol.schema.json). <a id="MC4-6.1-4"></a><sup>MC4-6.1-4</sup>

### 6.2 Handshake and version negotiation

1. The host sends `hello`: `{"type":"hello","id":0,"host":"mcxx 0.1.0","protocols":[1]}` -- the protocol versions it speaks.
2. The plugin answers `welcome` with the version it chose, which MUST be one the host offered, and describes itself: its providers (`name`, `extension-points`, `features`, `profiles`, `replaces`), in the fields of MC1 §10. <a id="MC4-6.2-1"></a><sup>MC4-6.2-1</sup> A plugin that speaks none of the offered versions answers `error` with `code` `"protocol"` and the versions it speaks, and exits. <a id="MC4-6.2-2"></a><sup>MC4-6.2-2</sup>

The host adds the plugin's providers to its catalog (§4) as it does a static plugin's. <a id="MC4-6.2-3"></a><sup>MC4-6.2-3</sup>

### 6.3 Requests

| Request | Members | Answer |
|---|---|---|
| `check` | `provider`, `path`, `module`, `wanted` (ids), `facts` (MC3 §4.11) | `findings`: `findings` (MC4 §2 rule out) |
| `filter` | `provider`, `path`, `target` (§2.1), `text` | `filtered`: `text` (string or `null`), `problems` (`range`, `message`), `findings` |
| `shutdown` | -- | none: the plugin exits |

- A plugin answers each request once, in any order, with the request's `id`; it answers a request it cannot serve with `error` (`id`, `code`, `message`). <a id="MC4-6.3-1"></a><sup>MC4-6.3-1</sup>
- A host MAY keep a plugin running across files and send it requests one after another; it sends `shutdown` before it exits. <a id="MC4-6.3-2"></a><sup>MC4-6.3-2</sup>
- For the same facts and wanted ids, a rule MUST give the same findings in process and out of process. <a id="MC4-6.3-3"></a><sup>MC4-6.3-3</sup>

A session ([`examples/mc4-session.jsonl`](examples/mc4-session.jsonl), `>` host to plugin, `<` plugin to host):

```
> {"type":"hello","id":0,"host":"mcxx 0.1.0","protocols":[1]}
< {"type":"welcome","id":0,"protocol":1,"providers":[{"name":"acme.naming","extension-points":["rule"],"features":[...],"profiles":[],"replaces":[]}]}
> {"type":"check","id":1,"provider":"acme.naming","path":"/w/src/a.cpp","module":"","wanted":["acme-snake-case"],"facts":{...}}
< {"type":"findings","id":1,"findings":[{"feature":"acme-snake-case","range":{...},"message":"`MyValue` is not snake_case","container":"app"}]}
> {"type":"shutdown","id":2}
```
