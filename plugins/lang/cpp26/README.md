# plugins/lang/cpp26：C++26 的核心语言特性（`mcxx-plugins-lang-cpp26`）

插件 `mcxx.plugins.lang.cpp26`，一个 MC4 language provider。C++26 的每篇核心语言提案是一个 `standard` 类特性，默认 deny（关闭）；一个文件开启其中任意一个，编译时就加 `-std=c++2c`（GNU 方言是 `-std=gnu++2c`），命令已经要求 C++26 或更新的标准时不加。怎么开启、表怎么生成，见 [`plugins/lang`](../README.md)。

共 54 篇：baseline 28，partial 1，todo 25。

| 特性 id | 提案 | 标题 | Clang 23.1 |
|---|---|---|---|
| `c++26:remove-undefined-behavior-lexing` | [P2621R2](https://wg21.link/P2621R2) | Remove undefined behavior from lexing | baseline（Clang 3.3） |
| `c++26:non-encodable-string-literals-ill-formed` | [P1854R4](https://wg21.link/P1854R4) | Making non-encodable string literals ill-formed | baseline（Clang 14） |
| `c++26:unevaluated-strings` | [P2361R6](https://wg21.link/P2361R6) | Unevaluated strings | baseline（Clang 18） |
| `c++26:basic-character-set-additions` | [P2558R2](https://wg21.link/P2558R2) | Add @, $, and ` to the basic character set | baseline（Yes） |
| `c++26:constexpr-cast-void` | [P2738R1](https://wg21.link/P2738R1) | constexpr cast from void* | baseline（Clang 17） |
| `c++26:ignorability-standard-attributes` | [P2552R3](https://wg21.link/P2552R3) | On the ignorability of standard attributes | baseline（Yes） |
| `c++26:static-storage-braced-initializers` | [P2752R3](https://wg21.link/P2752R3) | Static storage for braced initializers | todo（No） |
| `c++26:user-generated-static-assert-messages` | [P2741R3](https://wg21.link/P2741R3) | User-generated static_assert messages | baseline（Clang 17） |
| `c++26:placeholder-variables-no-name` | [P2169R4](https://wg21.link/P2169R4) | Placeholder variables with no name | baseline（Clang 18） |
| `c++26:template-parameter-initialization` | [P2308R1](https://wg21.link/P2308R1) | Template parameter initialization | baseline（Clang 18） |
| `c++26:pack-indexing` | [P2662R3](https://wg21.link/P2662R3) | Pack Indexing | baseline（Clang 19） |
| `c++26:remove-deprecated-arithmetic-conversion-enumerations` | [P2864R2](https://wg21.link/P2864R2) | Remove Deprecated Arithmetic Conversion on Enumerations | baseline（Clang 18） |
| `c++26:disallow-binding-returned-glvalue-temporary` | [P2748R5](https://wg21.link/P2748R5) | Disallow Binding a Returned Glvalue to a Temporary | baseline（Clang 19） |
| `c++26:clarifying-rules-brace-elision-aggregate-initialization` | [P3106R1](https://wg21.link/P3106R1) | Clarifying rules for brace elision in aggregate initialization | baseline（Clang 17） |
| `c++26:attributes-structured-bindings` | [P0609R3](https://wg21.link/P0609R3) | Attributes for Structured Bindings | baseline（Clang 19） |
| `c++26:module-declarations-not-macros` | [P3034R1](https://wg21.link/P3034R1) | Module Declarations Shouldn’t be Macros | baseline（Clang 23） |
| `c++26:trivial-infinite-loops-not-undefined-behavior` | [P2809R3](https://wg21.link/P2809R3) | Trivial infinite loops are not Undefined Behavior | baseline（Clang 19） |
| `c++26:erroneous-behaviour-uninitialized-reads` | [P2795R5](https://wg21.link/P2795R5) | Erroneous behaviour for uninitialized reads: P2795R5 | todo（No） |
| `c++26:erroneous-behaviour-p3684` | [P3684R1](https://wg21.link/P3684R1) | Erroneous behaviour for uninitialized reads: P3684R1 | todo（No） |
| `c++26:delete-with-reason` | [P2573R2](https://wg21.link/P2573R2) | = delete('should have a reason'); | baseline（Clang 19） |
| `c++26:variadic-friends` | [P2893R3](https://wg21.link/P2893R3) | Variadic friends | baseline（Clang 20） |
| `c++26:constexpr-placement-new` | [P2747R2](https://wg21.link/P2747R2) | constexpr placement new | baseline（Clang 20） |
| `c++26:delete-incomplete-type-ill-formed` | [P3144R2](https://wg21.link/P3144R2) | Deleting a Pointer to an Incomplete Type Should be Ill-formed | baseline（Clang 19） |
| `c++26:ordering-constraints-involving-fold-expressions` | [P2963R3](https://wg21.link/P2963R3) | Ordering of constraints involving fold expressions | baseline（Clang 19） |
| `c++26:structured-binding-declaration-as-condition` | [P0963R3](https://wg21.link/P0963R3) | Structured binding declaration as a condition | baseline（Clang 21） |
| `c++26:constexpr-structured-bindings` | [P2686R5](https://wg21.link/P2686R5) | constexpr structured bindings | todo（No） |
| `c++26:allowing-exception-throwing-constant-evaluation` | [P3068R6](https://wg21.link/P3068R6) | Allowing exception throwing in constant-evaluation | todo（No） |
| `c++26:remove-deprecated-array-comparisons` | [P2865R6](https://wg21.link/P2865R6) | Remove Deprecated Array Comparisons from C++26 | baseline（Clang 20） |
| `c++26:structured-bindings-can-introduce-pack` | [P1061R10](https://wg21.link/P1061R10) | Structured Bindings can introduce a Pack | baseline（Clang 21） |
| `c++26:oxford-variadic-comma` | [P3176R1](https://wg21.link/P3176R1) | The Oxford variadic comma | baseline（Clang 20） |
| `c++26:trivial-unions` | [P3074R7](https://wg21.link/P3074R7) | Trivial unions | todo（No） |
| `c++26:partial-program-correctness` | [P1494R5](https://wg21.link/P1494R5) | Partial program correctness | todo（No） |
| `c++26:contracts` | [P2900R14](https://wg21.link/P2900R14) | Contracts | todo（No） |
| `c++26:defang-deprecate-memory-order-consume` | [P3475R2](https://wg21.link/P3475R2) | Defang and deprecate memory_order::consume | todo（No） |
| `c++26:concept-variable-template-template-parameters` | [P2841R7](https://wg21.link/P2841R7) | Concept and variable-template template-parameters | todo（No） |
| `c++26:embed` | [P1967R14](https://wg21.link/P1967R14) | #embed | todo（No） |
| `c++26:reflection` | [P2996R13](https://wg21.link/P2996R13) | Reflection: P2996R13 | todo（No） |
| `c++26:annotations-for-reflection` | [P3394R4](https://wg21.link/P3394R4) | Reflection: P3394R4 | todo（No） |
| `c++26:splicing-base-class-subobjects` | [P3293R3](https://wg21.link/P3293R3) | Reflection: P3293R3 | todo（No） |
| `c++26:define-static-objects` | [P3491R3](https://wg21.link/P3491R3) | Reflection: P3491R3 | todo（No） |
| `c++26:function-parameter-reflection` | [P3096R12](https://wg21.link/P3096R12) | Reflection: P3096R12 | todo（No） |
| `c++26:reflection-p3598` | [P3598R0](https://wg21.link/P3598R0) | Reflection: P3598R0 | todo（No） |
| `c++26:attaching-main-global-module` | [P3618R0](https://wg21.link/P3618R0) | Attaching main to the global module | baseline（Clang 21） |
| `c++26:expansion-statements` | [P1306R5](https://wg21.link/P1306R5) | Expansion Statements | partial（Clang 23 (Partial) Iterating expansion statements currently cannot be expanded and will result in a diagnostic, but other types of expansion statements work.） |
| `c++26:constexpr-virtual-inheritance` | [P3533R2](https://wg21.link/P3533R2) | constexpr virtual inheritance | todo（No） |
| `c++26:preprocessing-never-undefined` | [P2843R3](https://wg21.link/P2843R3) | Preprocessing is never undefined | todo（No） |
| `c++26:allow-line-before-module-declarations` | [P3868R1](https://wg21.link/P3868R1) | Allow #line before module declarations | baseline（Clang 21） |
| `c++26:line-directive-existing-practice` | [P4136R2](https://wg21.link/P4136R2) | #line is not in line with existing implementation | baseline（Yes） |
| `c++26:partial-ordering-variadic-templates` | [P4004R1](https://wg21.link/P4004R1) | Reconsider CWG 1395 'Partial ordering of variadic templates reconsidered' | todo（No） |
| `c++26:ctad-type-template-template-parameters` | [P3865R3](https://wg21.link/P3865R3) | CTAD for type template template parameters | todo（No） |
| `c++26:adjustments-union-lifetime-rules` | [P3726R2](https://wg21.link/P3726R2) | Adjustments to Union Lifetime Rules | todo（No） |
| `c++26:constant-evaluations-fixes` | [P4143R0](https://wg21.link/P4143R0) | Constant evaluations fixes | todo（No） |
| `c++26:define-immediate-context` | [P4149R1](https://wg21.link/P4149R1) | Define 'immediate context' | todo（No） |
| `c++26:clarification-placement-new-deallocation` | [P3769R1](https://wg21.link/P3769R1) | Clarification of placement new deallocation | todo（No） |
