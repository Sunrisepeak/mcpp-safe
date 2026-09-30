# MC2 — Module Interfaces in the IFC Format

| | |
|---|---|
| Specification | MC2 |
| Version | 1.6.1 |
| Status | Draft |
| Schema | [`schema/mc2-interface.schema.json`](schema/mc2-interface.schema.json) (an interface as read back) |
| Examples | [`examples/mc2-interface.json`](examples/mc2-interface.json) (`conformance/ifc/dialect`'s `dialect.ifc`, read back) |
| Implementation | `modules/ifc` (`mcxx.ifc`: writing and reading), `modules/backend/clang/src/ifc.cppm` (the compile that writes it) |
| License | Apache-2.0 |

## Abstract

When `mcxx` compiles a module unit to a BMI, it writes beside the BMI what MC++ knows of the unit's interface: the declarations its code makes that are not local -- MC3's T1 facts -- and its dialect, the profiles and feature levels its code was gated with. The file is in the IFC format (Microsoft's, <https://github.com/microsoft/ifc-spec>), version 0.43, so any IFC reader can read its declarations; what IFC has no field for is carried in attributes of MC++'s own. An importing module learns what another module exposes, and under which dialect, from this file, without its source -- including what the unit makes reachable without declaring it: the declarations its exported using-declarations name, which is all a module like `std` exports (1.2.0).

## 1. Conventions

- The key words **MUST**, **MUST NOT**, **REQUIRED**, **SHALL**, **SHALL NOT**, **SHOULD**, **SHOULD NOT**, **RECOMMENDED**, **MAY** and **OPTIONAL** are to be interpreted as described in BCP 14 (RFC 2119, RFC 8174) when, and only when, they appear in all capitals.
- IFC names (partitions, sorts, structures) are those of the IFC SDK 0.43.5 (`microsoft/ifc`, `include/ifc/abstract-sgraph.hxx`); its structures are the layout.
- A unit's *T1 declarations* are the `declarations` of its MC3 facts (MC3 §4.2) whose `local` is false, in the facts' order.

## 2. The file

- A compile of a module interface unit or an implementation partition that writes a BMI -- the output of `--precompile`, or the file `-fmodule-output` names -- and ends without an error MUST write `X.ifc` beside the BMI `X.pcm` (the BMI's name with its extension replaced by `.ifc`). <a id="MC2-2-1"></a><sup>MC2-2-1</sup> A file that cannot be written is a warning (`[mcxx-ifc]`), not an error: the build has the outputs it asked for.
- The file MUST be IFC format version 0.43 (`FormatVersion {0, 43}`). <a id="MC2-2-2"></a><sup>MC2-2-2</sup> It is laid out as the SDK reads it: the signature `54 51 45 1A`, the `Header`, the string table (offset 0 is no text), each partition that has entries, and the table of contents, one `PartitionSummaryData` per partition present, whose count is the header's `partition_count`. Every partition and the table of contents start at a multiple of 4.
- The header's `content_hash` MUST be the SHA-256 digest (its 32 bytes, as the SDK's `hash_bytes` gives them) of every byte after it, from offset 36 to the end. <a id="MC2-2-3"></a><sup>MC2-2-3</sup>
- The header says: `unit` -- `Primary` with the module's name, or `Partition` with `module:partition`; `internal_partition` -- true for an implementation partition; `src_path` -- the unit's source file, absolute; `global_scope` -- scope 1; `arch` -- from the target (`X64`, `ARM64`, `X86`, `ARM32`, otherwise `Unknown`); `cplusplus` -- the compile's `__cplusplus`.
- The same interface MUST give the same bytes, and a file that already holds them MUST NOT be written again (it keeps its time, as a BMI a build finds unchanged does). <a id="MC2-2-4"></a><sup>MC2-2-4</sup>

## 3. Declarations

- Every T1 declaration of the unit MUST be one IFC declaration, in the sort the table gives for its kind. <a id="MC2-3-1"></a><sup>MC2-3-1</sup>

| MC3 kind | IFC sort (partition) | Type | Placed in |
|---|---|---|---|
| `namespace` | `Scope` (`decl.scope`) | fundamental `namespace` | its scope; its members in its own scope |
| `class`, `struct`, `union` | `Scope` | fundamental `class`, `struct`, `union` | its scope; its members in its own scope |
| `enum` | `Enumeration` (`decl.enum`) | fundamental `enum` | its scope |
| `enumerator` | `Enumerator` (`decl.enumerator`) | none | its enumeration's sequence (a `Barren` one when no enumeration holds it) |
| `type-alias` | `Alias` (`decl.alias`) | fundamental `typename` | its scope |
| `function` | `Function` (`decl.function`) | none | its scope |
| `method`, `conversion` | `Method` (`decl.method`) | none | its scope |
| `constructor` | `Constructor` (`decl.constructor`) | none | its scope |
| `destructor` | `Destructor` (`decl.destructor`) | none | its scope |
| `field` | `Field` (`decl.field`) | none | its scope |
| `variable` | `Variable` (`decl.variable`) | none | its scope |
| `parameter` | `Parameter` (`decl.parameter`) | none | its function's chart (`chart.unilevel`), in order, `position` from 1; with no function around it, in no chart |
| any other | `Barren` (`decl.barren`) | none | its scope |

- A declaration's scope is the innermost `namespace`, `class`, `struct` or `union` declaration before it whose range holds its range, or the global scope; a parameter's function is the innermost function-like declaration (`function`, `method`, `conversion`, `constructor`, `destructor`) that does. `home_scope` is that declaration.
- Its identity is its name, the last component of its qualified name (an operator's name whole), and the start of its name range as its place: `src.line` entry 0 is no place, the others are the unit's file (`name.source-file` 0) and a line from 1; columns count from 1. A declaration outside `export` has the basic specifier `NonExported`.
- Declared types are not IFC types in MC2 1.x: MC3's text is carried (§4). A static member function is a `Method`.
- (1.6.0) MC3 0.8.0's enumerators, namespace aliases and using-declarations are among a unit's T1 declarations: an enumerator in its enumeration's sequence, a namespace alias or a using-declaration a `Barren` declaration in its scope, whose `type` (§4) is what it names. A 1.0-1.5 interface has none of them (the name a namespace alias or a using-declaration of such a unit introduces is not known to an importer). The unit's own enumerators, T1 declarations now, are no longer among its reachable declarations (§3.1); an exported alias of the unit reaches the class or the enumeration it names when another unit declares it and it is not `std`'s (`using Spec = pm::Spec;` makes `pm::Spec`'s members an importer's); a reachable class's private and protected member aliases are reachable (a public member's type may be written with one), and so are the public members of the partial specializations of a class template that only declares its primary (libc++'s `__optional_destruct_base<_Tp, bool>`, where `std::optional`'s `reset()` is), named under the template. (1.6.1) An exported using-declaration an included file writes whose name is not what it names, and that names one declaration (not an overload set), is reachable too, as a using-declaration: libc++'s std module writes `using std::uint64_t;`, which is the C library's `::uint64_t`, and an importer finds `std::uint64_t` through it.

### 3.1 Reachable declarations (1.2.0)

An importer names more than a unit's own code declares: `export using std::vector;` makes a declaration of another file nameable through the unit, and libc++'s `std` module exports nothing else. So an interface also carries the unit's *reachable declarations*:

- every declaration an exported using-declaration of the unit names (each of its shadows' targets; a template's pattern), every public member of a class among them -- a field, a method, a constructor, a nested class or enumeration, a member alias, a member template's pattern, a static member -- and so on for the members that are classes, and every enumerator of an enumeration among them or among the unit's own exported ones (MC3's T1 declarations have no enumerators); and (1.3.0) the bases of every class among them, recursively, with their public members -- what a member access on such a class finds (`std::atomic`'s `store` is `std::__atomic_base`'s) --, and the class an alias among them names (`using json = basic_json<>`); and (1.4.0) the enumeration an alias among them names, with its enumerators (`json::value_t`). Each once, as an MC3 declaration (§4.2 of MC3) with its type and templates, `exported` true, `local` false and no ranges (they are in other files). <a id="MC2-3.1-1"></a><sup>MC2-3.1-1</sup>
- They come after the T1 declarations, in the table's sorts. A reachable declaration's scope is the declaration (T1 or reachable) whose qualified name is its own's prefix, when that is a namespace or a class, else the global scope; an enumerator is in the enumeration its qualified name is in (a scoped one's), else a `Barren` one. Its place is no place (`src.line` entry 0), and its `mcxx::decl` flags include `reachable`. <a id="MC2-3.1-2"></a><sup>MC2-3.1-2</sup>

## 4. What IFC has no field for: `[[mcxx::decl]]`

- Every T1 declaration MUST carry one attribute `mcxx::decl(...)`, associated with it by the trait `.msvc.trait.decl-attrs` (the SDK's `DeclAttributes`: a `DeclIndex` and an `AttrIndex`, sorted by the declaration index as a 32-bit value). <a id="MC2-4-1"></a><sup>MC2-4-1</sup> Its arguments, in order:

| # | Argument | MC3 member |
|---|---|---|
| 0 | the declaration's position in the facts, from 0 (decimal) | its order |
| 1 | its kind (MC3's name: `type-alias`, `namespace-alias`, ...) | `kind` |
| 2 | its entity | `entity` |
| 3 | its qualified name | `qualified-name` |
| 4 | its container | `container` |
| 5 | its type as text | `type` |
| 6 | its flags, comma-separated: `exported`, `c-array`, `pointer`, `union`, `c-variadic`; `reachable` for a reachable declaration (§3.1) | the booleans |
| 7 | its range, `line:column-line:column` (MC3's, from 0) | `range` |
| 8 | its name's range | `name` |
| 9... | the templates its type names; then (1.3.0), when it has any, an argument `|` -- which no name holds -- and its bases; then (1.4.0), when it has any, an argument `<` -- which neither a name nor a parameter is -- and its template parameters; then (1.5.0), for a function-like declaration whose producer said them, an argument `(` and its parameters (none after it for a function without any) | `templates`, `bases`, `template-parameters`, `parameters` |

- An MC2 attribute is a `CalledAttr` (`attr.called`) whose function is a `ScopedAttr` (`attr.scoped`) of two identifier words, `mcxx` and its name, and whose arguments are a `TupleAttr` (`attr.tuple`, entries in `heap.attr`) of `BasicAttr`s (`attr.basic`), each one word of sort `Literal` (`source::Literal::String`) whose text is a C++ string literal's spelling: `"` and `\` escaped, newline, tab and carriage return as `\n`, `\t`, `\r`, other control characters as three octal digits, anything else as it is. <a id="MC2-4-2"></a><sup>MC2-4-2</sup>

## 5. The dialect

- The global scope's first member MUST be `Barren` declaration 0, an attribute declaration (directive `dir.attribute` 0) whose attribute is a tuple of MC2 attributes: <a id="MC2-5-1"></a><sup>MC2-5-1</sup>

| Item | Arguments | Says |
|---|---|---|
| `mcxx::mc2` | the MC2 version (`"1.2.0"`) | the file is MC2's; exactly one |
| `mcxx::target` | the target triple | the compile's target (absent when unknown) |
| `mcxx::profile` | a profile | one per profile of the unit's package (MC1 §4), in order |
| `mcxx::feature` | a feature id, a level | one per feature of the catalog the unit was compiled with: its level for code in the module, before any namespace's or declaration's (MC1 §6) |
| `mcxx::namespace_feature` | a namespace, a feature id, a level | one per level the package sets for a namespace (MC1 §6) |
| `mcxx::reexport` | a module's full name (`m`, `m:p`) | one per module the unit re-exports (`export import`): what an importer of the unit also sees (1.1.0) |
| `mcxx::import` | a module's full name | one per other module the unit imports: what a unit of the same module that imports it sees too, as an implementation unit sees what its interface imports ([module.import]/7) (1.6.0) |

## 6. Reading

- A reader MUST reject a file whose signature, format version or content hash is not §2's, and any offset or index outside the file; it MUST NOT end the process on a malformed file. <a id="MC2-6-1"></a><sup>MC2-6-1</sup>
- A reader of MC2 1.x MUST reject a file without exactly one `mcxx::mc2` whose major version is 1, a dialect item or an MC2 attribute it does not know, and a declaration whose sort is not its kind's (§3). <a id="MC2-6-2"></a><sup>MC2-6-2</sup> A 1.2 reader reads 1.1 files, which have no reachable declarations, and 1.0 files, which have no `mcxx::reexport` either.
- Read back, the declarations MUST equal the unit's T1 declarations item by item and field by field, in the facts' order (their `local` is false), and the reachable ones what was written, in order. <a id="MC2-6-3"></a><sup>MC2-6-3</sup>

## 7. Finding a BMI's interface

A build may copy a BMI where nothing beside it is copied: mcpp's build caches keep the BMIs of dependencies and of `std`, and a later build of another project gets a copy of the BMI alone. So the interface is also kept by the BMI's content:

- A compile that wrote an interface beside a BMI and then ended without an error MUST keep a copy of it in the store, named by the SHA-256 of the BMI's bytes (`<store>/<64 hex digits>.ifc`). The store is `$MCXX_IFC_STORE`, else `$XDG_CACHE_HOME/mcxx/ifc`, else `~/.cache/mcxx/ifc`; a host may name its own. A copy already there that holds other bytes (the same BMI, an interface another writer gave it) is replaced. <a id="MC2-7-1"></a><sup>MC2-7-1</sup>
- A reader looking for the interface of a BMI MUST read the file beside it, and when there is none, the store's copy for the BMI's content. When neither exists the interface is not known, which is not the same as empty (MC3 §4.13). <a id="MC2-7-2"></a><sup>MC2-7-2</sup> A BMI's digest may be remembered by its path, size and modification time (`<store>/by-path/`), so that a large BMI (`std`) is read whole once.

## 8. The JSON form

`mcxx-probe --read-ifc X.ifc` prints an interface as MC2 reads it: `module`, `internal`, `source`, `target`, `cplusplus`, `dialect` (`profiles`, `features` by id, `namespaces` by namespace then id), `reexports`, `imports` (1.6.0), `declarations` and `reachable` (1.2.0) as MC3 declarations (`schema/mc3-facts.schema.json`). [`schema/mc2-interface.schema.json`](schema/mc2-interface.schema.json) is its schema.

## 9. Rationale

- **Why attributes, not IFC's vendor extensions.** IFC 0.43's vendor sorts are the SDK's own (`VendorSort` is MSVC's structured exception handling), and its reader stops at a `DeclSort::VendorExtension` declaration ("unexpected decl") and at a partition name it does not know. Attributes are standard IFC that every reader accepts and skips; MC2's live in the `mcxx::` namespace, which is MC2's vendor extension. The acceptance item (A1.1.4) names the dialect's place "VendorExtension" in that sense.
- **Why the SDK's structures but not its reader.** The layout is the SDK's by construction (its structures are used to write). Its reader asserts on a malformed file, and its assertion ends the process (`ifc_assert` calls `exit`), which a compiler or an editor reading another module's file cannot allow; MC2's reader checks every offset instead. The SDK's `ifc-printer` is the independent check that the files are IFC (`tools/checks/ifc.py`).
- **Why a store by content, not a path.** A copied BMI keeps its bytes and loses its neighbours; its content is what identifies it (two builds of one source with different options give different BMIs and different interfaces). Hashing a BMI costs a read of it, once per path, size and time.
- **Why types as text.** MC3 gives types as text; IFC's type graph (`type.*`, `expr.*`) is a larger encoding that a later MC2 version fills when a reader needs it (M2.2, `import std;` through IFC).
