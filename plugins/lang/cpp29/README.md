# plugins/lang/cpp29：C++29 的核心语言特性（`mcxx-plugins-lang-cpp29`）

插件 `mcxx.plugins.lang.cpp29`，一个 MC4 language provider。C++29 的每篇核心语言提案是一个 `standard` 类特性，默认 deny（关闭）；一个文件开启其中任意一个，编译时就加 `-std=c++2d`（GNU 方言是 `-std=gnu++2d`），命令已经要求 C++29 或更新的标准时不加。怎么开启、表怎么生成，见 [`plugins/lang`](../README.md)。

Clang 23.1 把 C++26 之后的标准叫 C++2d，`-std=c++2d`、`-std=gnu++2d` 就是它；`c++29` 这个别名要等 C++29 正式发布后才加（`LangStandards.def` 里的 TODO）。Clang 22 还不认识 `c++2d`。

共 17 篇：baseline 2，partial 0，todo 15。

| 特性 id | 提案 | 标题 | Clang 23.1 |
|---|---|---|---|
| `c++29:floating-point-overflow` | [P3899R3](https://wg21.link/P3899R3) | Clarify the behavior of floating-point overflow | todo（No） |
| `c++29:defaulting-postfix-increment-decrement` | [P3668R4](https://wg21.link/P3668R4) | Defaulting postfix increment and decrement operations | todo（No） |
| `c++29:defaulted-assignment-restrictions` | [P2953R5](https://wg21.link/P2953R5) | Adding restrictions to defaulted assignment operator functions | todo（No） |
| `c++29:nondeterministic-pointer-provenance` | [P2434R5](https://wg21.link/P2434R5) | Nondeterministic pointer provenance | todo（No） |
| `c++29:invalid-pointer-operations` | [P3347R6](https://wg21.link/P3347R6) | Invalid pointer operations | todo（No） |
| `c++29:unicode-identifier-recommendations` | [P3658R1](https://wg21.link/P3658R1) | Adjust identifier following new Unicode recommendations | todo（No） |
| `c++29:return-value-and-return-void` | [P3950R1](https://wg21.link/P3950R1) | return_value & return_void are not mutually exclusive | todo（No） |
| `c++29:more-named-universal-character-escapes` | [P3733R1](https://wg21.link/P3733R1) | More named universal character escapes | baseline（Clang 23） |
| `c++29:lexical-order-lambdas` | [P3847R1](https://wg21.link/P3847R1) | Lexical order for lambdas | baseline（Clang 3.1） |
| `c++29:language-linkage-templates` | [P2243R0](https://wg21.link/P2243R0) | Language linkage for templates | todo（No） |
| `c++29:throwing-deallocation-functions-ill-formed` | [P3424R2](https://wg21.link/P3424R2) | Deallocation functions with throwing exception specification are ill-formed | todo（No） |
| `c++29:conditional-noexcept-specifiers-compound-requirements` | [P3822R2](https://wg21.link/P3822R2) | Conditional noexcept specifiers in compound requirements | todo（No） |
| `c++29:contracts-virtual-functions` | [P3097R3](https://wg21.link/P3097R3) | Contracts for C++: virtual functions | todo（No） |
| `c++29:consteval-only-values` | [P4101R1](https://wg21.link/P4101R1) | Consteval-only values for C++26 | todo（No） |
| `c++29:pointer-lifetime-end-zap-proposed-solutions` | [P2414R12](https://wg21.link/P2414R12) | Pointer lifetime-end zap proposed solutions | todo（No） |
| `c++29:pack-indexing-template-names` | [P3670R4](https://wg21.link/P3670R4) | Pack indexing for template names | todo（No） |
| `c++29:embed-offset-parameter` | [P3540R3](https://wg21.link/P3540R3) | #embed offset parameter | todo（No） |
