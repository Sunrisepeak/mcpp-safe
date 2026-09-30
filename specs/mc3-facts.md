# MC3 — MC++ Semantic API: Positions, Certainty and Facts

| | |
|---|---|
| Specification | MC3 |
| Version | 0.8.0 |
| Status | Draft |
| Schema | [`schema/mc3-facts.schema.json`](schema/mc3-facts.schema.json) |
| Examples | [`examples/mc3-facts.json`](examples/mc3-facts.json) |
| Implementation | `modules/msa` (the C++ interface `mcxx.msa`), `modules/backend/clang` (`:facts`, over Clang 23.1), `modules/frontend` (MC++'s own front end: `macros` and `includes`) |
| License | Apache-2.0 |

## Abstract

The MC++ semantic API (MSA) is what a compiler front end tells the rest of MC++ about a C++ modules program, in values that name no compiler's types: the LSP service, the feature gates (MC1), plugins (MC4) and tools are written against it, and any front end -- Clang today, MC++'s own later -- implements it. This version specifies the part everything above the backend depends on for gating: positions, certainty, and **facts**, what the code of one file declares and does. Facts have a C++ form (`mcxx::msa::fact::Facts`) and a JSON form, which this specification defines and which out-of-process plugins receive (MC4 §6).

## 1. Conventions

- The key words **MUST**, **MUST NOT**, **REQUIRED**, **SHALL**, **SHALL NOT**, **SHOULD**, **SHOULD NOT**, **RECOMMENDED**, **MAY** and **OPTIONAL** are to be interpreted as described in BCP 14 (RFC 2119, RFC 8174) when, and only when, they appear in all capitals.
- JSON is as defined by RFC 8259, encoded in UTF-8. JSON member names are lower-case words joined by `-`.

## 2. Positions and ranges

A position is a 0-based `line` and a 0-based `column` counted in UTF-8 bytes. <a id="MC3-2-1"></a><sup>MC3-2-1</sup> A range is a `begin` and an `end` position; `end` is one past the last byte. <a id="MC3-2-2"></a><sup>MC3-2-2</sup> A consumer that speaks another unit (LSP's UTF-16) converts; the LSP layer of MC++ does (`modules/lsp`).

## 3. Certainty

Every answer of the MSA carries a certainty: `certain`, or `unknown`, which means "ask someone else", never "no". <a id="MC3-3-1"></a><sup>MC3-3-1</sup> A backend that could not compute a kind of fact it was asked for MUST say `unknown` rather than return an empty list as if there were none. <a id="MC3-3-2"></a><sup>MC3-3-2</sup>

## 4. Facts

Facts describe **the file's own code**: a backend MUST NOT report a fact whose place is in a file the unit includes or a module it imports. <a id="MC3-4-1"></a><sup>MC3-4-1</sup> Every fact has a `range` and a `container`: the qualified name of its enclosing namespace, without inline namespaces, `""` for the global namespace. <a id="MC3-4-2"></a><sup>MC3-4-2</sup> Qualified names throughout omit inline namespaces (`std::vector`, not `std::__1::vector`). <a id="MC3-4-3"></a><sup>MC3-4-3</sup> A qualified name or a type MUST NOT depend on how the command line named the file: an unnamed class, union or enum is `(anonymous union)`, `(unnamed struct)`, `(unnamed enum)`, without its place. <a id="MC3-4-4"></a><sup>MC3-4-4</sup>

### 4.1 Kinds

Facts come in kinds. A consumer asks for a set of kinds; a backend MUST fill every kind asked for, and MAY leave the others empty; `collected` says which kinds were asked for. <a id="MC3-4.1-1"></a><sup>MC3-4.1-1</sup>

| Kind | Records | Tier |
|---|---|---|
| `declarations` | §4.2 | T1: what the file declares |
| `declaration-types` | fills `type` and `templates` of every declaration (§4.2) | T1 |
| `initializations` | §4.3 | T2: what the code does |
| `casts` | §4.4 | T2 |
| `allocations` | §4.5 | T2 |
| `pointer-arithmetic` | §4.6 | T2 |
| `gotos` | §4.7 | T2 |
| `macros` | §4.7 | T2 |
| `uses` | §4.8 | T2 |
| `includes` | §4.9 | T2 |
| `suppressions` | §4.10 | waivers (MC1 §7) |
| `attributes` | §4.12 | the attributes providers claim (MC4 §2), where they are written (0.2.0) |
| `imports` | §4.13 | the named modules the file imports, and what their MC2 interfaces say they bring in (0.4.0) |

### 4.2 Declaration

One per variable, field, parameter, function, type alias, class, union, enum, enumerator, namespace, namespace alias and using-declaration (the last three kinds 0.8.0; not an inheriting constructor's `using B::B;`) the file's own code declares, excluding implicit declarations, declarations in a body the compiler writes (`= default`) and template instantiations (a template counts once, as written); a parameter is a function's, not one written in a function type inside another declaration (`std::function<void(int level)>`). <a id="MC3-4.2-1"></a><sup>MC3-4.2-1</sup>

| Member | Type | Description |
|---|---|---|
| `name` | range | Its name. |
| `entity` | string | A stable id of the entity (a USR for Clang). |
| `qualified-name` | string | |
| `kind` | string | `variable`, `field`, `parameter`, `function`, `method`, `constructor`, `type-alias`, `class`, `struct`, `union`, `enum`, `enumerator`, `namespace`, `namespace-alias`, `using-declaration`, ... |
| `type` | string | The declared type (of a variable, member, parameter or alias); a function's or a method's return type (0.5.0) -- a constructor, a destructor and a conversion function have none, their names say it. An enumerator's is its enumeration, a namespace alias's the namespace it names and a using-declaration's what it names, each by the qualified name this specification gives it (`ns::Color`, `std::filesystem`, `mcpplibs::cmdline::detail::Option`: inline namespaces left out) (0.8.0): what an importer needs to find `fs::path` through a module's `namespace fs = std::filesystem;`, or `cmdline::Option` through its `export using detail::Option;`. <a id="MC3-4.2-8"></a><sup>MC3-4.2-8</sup> Filled for every declaration when `declaration-types` was asked for, and otherwise at least for a declaration marked `c-array` or `pointer`. <a id="MC3-4.2-2"></a><sup>MC3-4.2-2</sup> |
| `templates` | string[] | Every class template the declared type names, at any depth (`std::vector`, `nlohmann::basic_json`); with `declaration-types`. |
| `bases` | string[] | A class's direct bases, by the qualified names this specification gives classes (a specialization by its template's name, `std::__atomic_base`); with `declaration-types`, and absent where there are none (0.5.0). It is where a member access finds what a class inherits. <a id="MC3-4.2-5"></a><sup>MC3-4.2-5</sup> |
| `template-parameters` | string[] | A class template's (its class declaration's), an alias template's or (0.7.0) a function template's parameters, in order: a type parameter `class T`, `class ...Ts`, or with its default `class A = std::allocator<T>` -- every name in a default fully qualified (inline namespaces left out), the template's own parameters by their names; a non-type parameter as its type and name, `std::size_t N` (its default left out); a template template parameter `template class C`. An unnamed parameter has no name (`class = void`). With `declaration-types`, and absent where there are none (0.6.0). It is what a member's type written with the parameters stands for in a specialization (`std::expected<R, E>::error` gives an `E`), and the arguments a specialization leaves to their defaults. <a id="MC3-4.2-6"></a><sup>MC3-4.2-6</sup> |
| `parameters` | string[] | A function's, a method's, a constructor's or a conversion function's parameters' types, in order, as Clang's TypePrinter prints each as declared (`const std::string &`, `std::string_view`), followed by ` =` for a parameter with a default argument (`bool =`); a C `...` is not one (`c-variadic` says it). With `declaration-types`, present for every function-like declaration -- empty when it has none -- and for no other; absent in a document before 0.7.0, where it is not known (0.7.0). It is what a call's arguments choose among overloads by. <a id="MC3-4.2-7"></a><sup>MC3-4.2-7</sup> |
| `exported` | boolean | Inside `export`. |
| `c-array` | boolean | Its type is an array type. |
| `pointer` | boolean | Its type holds a pointer type anywhere: itself, an array element, a template argument; for a function, its return type. <a id="MC3-4.2-3"></a><sup>MC3-4.2-3</sup> |
| `union` | boolean | A union's definition. |
| `c-variadic` | boolean | A function with a C `...` parameter. |
| `local` | boolean | Declared in a function's body -- a local variable, a local class and what it declares -- and so not reachable from outside it; a parameter is not local (0.3.0). <a id="MC3-4.2-4"></a><sup>MC3-4.2-4</sup> |

### 4.3 Initialization

One per variable or member initialized by an initializer the file writes, and one per variable of automatic storage whose default-initialization leaves it indeterminate (with `indeterminate` true). <a id="MC3-4.3-1"></a><sup>MC3-4.3-1</sup>

| Member | Type | Description |
|---|---|---|
| `name` | range | The variable's name. |
| `entity`, `variable` | string | Its id and qualified name. |
| `type`, `type-template` | string | The initialized type, and the class template it specializes (or `""`). |
| `form` | string | `default`, `copy` (`= e`), `direct` (`(e)`), `direct-list` (`{e}`), `copy-list` (`= {e}`). |
| `constructor` | string | The constructor chosen, qualified, with its parameter types, or `""`. |
| `initializer-list-constructor` | boolean | That constructor takes a `std::initializer_list` first. |
| `elements` | integer | The braced list's elements. |
| `element-braced` | boolean | A one-element list whose element is itself a braced list. |
| `member-default` | boolean | A default member initializer. |
| `indeterminate` | boolean | No initializer, and the value is indeterminate ([dcl.init.general], [basic.indet]). |

### 4.4 – 4.10 The other records

| Record (kind) | Members |
|---|---|
| Cast (`casts`) | `kind`: `static_cast`, `dynamic_cast`, `const_cast`, `reinterpret_cast`, `c-style`, `functional`; `from`, `to` (types); `reinterprets`: what it does is a reinterpret_cast; `to-scalar`: its target is a scalar type <a id="MC3-4.4-1"></a><sup>MC3-4.4-1</sup> |
| Allocation (`allocations`) | `delete` (boolean), `array` (boolean), `type` |
| PointerArithmetic (`pointer-arithmetic`) | `op`: `+`, `-`, `+=`, `-=`, `++`, `--`, `[]`; `type`: the pointer's type |
| Goto (`gotos`) | `label` (`*` for a computed goto) |
| MacroDefinition (`macros`) | `name` |
| Use (`uses`) | `construct`: `throw`, `try`, `typeid`, `asm`, `va_arg`; `detail` (for `typeid`, the operand's type) <a id="MC3-4.8-1"></a><sup>MC3-4.8-1</sup> |
| Include (`includes`) | `header`, as written with its brackets or quotes; `global-module-fragment`: it is before the module declaration of a module unit <a id="MC3-4.9-1"></a><sup>MC3-4.9-1</sup> |
| Suppression (`suppressions`) | `ids` (string[]), `entity`, `declaration` (qualified name), `reason`; its `range` is the declaration's |

### 4.11 The JSON form

A facts document is an object with `mc3-version` (`"0.4.0"`; a reader also takes `"0.3.0"`, which has no `imports`, `"0.2.0"`, whose declarations have no `local` either, and `"0.1.0"`, which has no `attributes` either), `path` (the file), `module` (`"m"`, `"m:p"` or `""`), `certainty`, `collected` (kind names) and one array per kind, named as in §4.1 (`declarations`, ..., `suppressions`, `attributes`, `imports`; `declaration-types` has no array of its own). It MUST validate against [`schema/mc3-facts.schema.json`](schema/mc3-facts.schema.json). <a id="MC3-4.11-1"></a><sup>MC3-4.11-1</sup> Reading a document and writing it again MUST give the same document (member order aside). <a id="MC3-4.11-2"></a><sup>MC3-4.11-2</sup>

### 4.12 Attribute

One per use of an attribute a provider claims (MC4 §2) on a declaration of the file: `name` (as claimed, `acme::hot`), `arguments` (as written; a string literal's contents without its quotes), `name-range` (where the attribute is written), and the declaration it is on: `entity`, `declaration` (qualified name), `kind`. Its `range` is the declaration's: what the attribute reaches. <a id="MC3-4.12-1"></a><sup>MC3-4.12-1</sup>

### 4.13 Import

One per import of a named module in the file's own code (`import m;`, `import :p;`, `export import m;`; not a header unit, not a module implementation unit's implicit import of its interface): `module` (its full name, `m` or `m:p`), `name` (where the name is written), `exported`, and `interfaces`: what the import brings in, as the modules' MC2 interfaces say -- the module first, then every module it re-exports (MC2 `mcxx::reexport`), each once. Each: `module`, `found` (its interface was read), and when found its dialect -- `profiles`, `levels` (feature id to level, for code in the module) -- and `exported`, its exported T1 declarations as §4.2 has them; their ranges are in that module's file. <a id="MC3-4.13-1"></a><sup>MC3-4.13-1</sup> A backend MUST read them from the interfaces, never from the modules' sources: the .ifc beside the BMI the compile imported, or the one kept for that BMI's content (MC2 §7). <a id="MC3-4.13-2"></a><sup>MC3-4.13-2</sup> An import's `range` is the declaration's; MC1's waiver on an import (MC1 §7) covers it. This is the one fact whose content is another module's: the boundary a rule decides on is at the import, in the file (MC4 `report_imports`).
