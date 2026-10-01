# Changelog

Changes to the specifications in this directory. Each specification is versioned independently.

## 2026-10-01 — MC2 1.7.0: what an exported function returns

- **MC2** 1.7.0 (§3.1): a class or enumeration the unit declares and does not export, named by the
  return type or a parameter type of an exported function or of a public member function of an
  exported class (references and pointers taken off), is among the reachable declarations, with its
  members: [module.reach]/3 makes it reachable, and an importer's call on what such a function
  returns is a member access on it. `mcpplibs.cmdline`'s `App::option(std::string_view)` returns its
  unexported `OptBuilder`; `app.option("x").takes_value().help("...")`, written hundreds of times in
  C-mcppls, named nothing an importer knew. Nothing new to read: a 1.6 reader reads it.

## 2026-09-30 — MC2 1.6.1: what std takes from the C library

- **MC2** 1.6.1 (§3): an exported using-declaration an included file writes whose name is not what
  it names, and that names one declaration (not an overload set), is among the reachable
  declarations, as a using-declaration: libc++'s std module's `using std::uint64_t;` names the C
  library's `::uint64_t`, which the interface reached before without the name `std` gives it.
  Nothing new to read: a 1.6.0 reader reads it.

## 2026-09-30 — MC3 0.8.0, MC2 1.6.0: enumerators and namespace aliases

- **MC3** 0.8.0 (§4.2): enumerators, namespace aliases and using-declarations are declarations too;
  an enumerator's `type` is its enumeration, a namespace alias's the namespace it names, a
  using-declaration's what it names, each by its qualified name (MC3-4.2-8). An importer finds what
  a module's interface declares that way: `namespace fs = std::filesystem;`, `enum class Level {
  Notice, Error }`, `export using detail::Option;`.
- **MC2** 1.6.0: the interface carries them (§3): an enumerator in its enumeration's sequence as
  before, a namespace alias or a using-declaration a `Barren` declaration in its scope. An exported
  alias of the unit reaches the class it names when another unit (not `std`) declares it; the unit's
  own enumerators are no longer repeated among its reachable declarations; an exported namespace
  alias an included file writes is reachable (libc++'s `std::views`), and so is a private or
  protected member type alias of a reachable class (libc++'s `directory_entry::_Path`, which
  `path()` returns), and so are the members of a class template's partial specializations when it only
  declares its primary (libc++'s `std::optional::reset()` is `__optional_destruct_base`'s). `mcxx::import`: the unit's other imports, which a unit of its module that
  imports it sees too ([module.import]/7). 1.0-1.5 files are read as before.

## 2026-09-30 — MC3 0.7.0, MC2 1.5.0: parameters

- **MC3** 0.7.0 (§4.2): a function-like declaration's `parameters`, each parameter's type as
  declared, ` =` after one with a default argument (MC3-4.2-7); `template-parameters` for a function
  template too. With them a call chooses among overloads whose return types differ
  (`app.option("name")` is the `std::string_view` one), and a function template's return type written
  as one of its parameters is the argument's (`json.value("k", Json::object())` gives a json).
- **MC2** 1.5.0: the interface carries them, after a `(` among the `mcxx::decl` arguments. 1.0-1.4
  files are read as before (their functions' parameters not known).

## 2026-09-30 — MC3 0.6.0, MC2 1.4.0: template parameters

- **MC3** 0.6.0 (§4.2): a class template's or an alias template's `template-parameters` (MC3-4.2-6),
  each as written with the names in its default fully qualified: `class _Tp`,
  `class _Allocator = std::allocator<_Tp>`, `std::size_t _Size`, `template class _C`. With them a
  member's type written in the template's parameters is known in a specialization
  (`std::expected<R, E>::error()` gives an `E`), and so are the arguments a specialization leaves to
  their defaults (`std::vector<int>` is `std::vector<int, std::allocator<int>>`, which names
  `std::allocator`).
- **MC2** 1.4.0: the interface carries them, after a `<` among the `mcxx::decl` arguments. The
  reachable declarations include the enumeration an alias among them names, with its enumerators
  (`json::value_t::string`), and they start from the unit's own exported using-declarations only: a
  unit that imports `std` no longer carries `std`'s reachable set again (every module interface of
  C-mcppls was about 4 MB, and an importer read 212077 declarations for a file that declares 111).
  1.0-1.3 files are read as before. The store's copy of an interface (§7) is replaced when the same BMI comes
  with another one (a newer writer): MC2-2-4 keeps only a copy that holds the same bytes.

## 2026-09-30 — MC3 0.5.0, MC2 1.3.0: return types and bases

- **MC3** 0.5.0 (§4.2): a function's or a method's `type` is its return type (a constructor, a
  destructor and a conversion function have none); a class's `bases`, its direct bases by the names
  MC3 gives classes (MC3-4.2-5). With both, a name after a call (`f().g`) and a member a class
  inherits are what MC++'s own lookup finds.
- **MC2** 1.3.0: the interface carries them -- `bases` after a `|` among the `mcxx::decl` arguments --
  and the reachable declarations include the bases of the reachable classes, with their public
  members (`std::atomic::store` is `std::__atomic_base`'s). 1.0-1.2 files are read as before.

## 2026-09-29 — MC4 0.3.0: plugin libraries

- **MC4** 0.3.0 (§3, §5): a third way for a plugin to reach a compiler, `library = "..."` -- the
  same package as a static plugin, built as a shared library and loaded into the compiler when a
  package names it; nothing to compose. MC4-3-1 no longer forbids loading: the host binds a
  library's references to its own names first (SDK, MSA, the C and C++ runtime), and one that
  cannot load libraries reports the plugin as one that cannot be started. MC4-3-6 (a relative
  `library` is relative to its manifest), MC4-3-7 (what a library registers while it loads is held
  until its `mcxx_plugin_sdk_abi` is read, and dropped when it is not the host's). MC4-5-1 now
  says which plugins it covers: an out-of-process plugin cannot crash the compilation; a static
  plugin and a library are in its process and trusted as it is. MC1's config schema takes
  `library`; the example names one.

## 2026-09-29 — MC5 0.4.0: diagnostic views

- **MC5** 0.4.0 (§9): `--mcxx-diagnostics=human|agent|clang` (or `MCXX_DIAGNOSTICS`): a compile's
  diagnostics laid out as Rust's for people (headline, place, excerpt, help, where a gate's level is
  set), as one JSON object each for agents (schema `mc5-diagnostic.schema.json`: code, exact range,
  level and its source, fix, waiver as code, fix-its, notes), or in the compiler's own format -- the
  default off a terminal.

## 2026-09-29 — MC1 0.4.0: levels by file

- **MC1** 0.4.0: `[package.metadata.mcxx.files."<glob>"]` -- the levels of the package's files a glob
  matches, relative to the manifest's directory (`*`, `**`, `?`), the most specific pattern winning;
  in §6 between a namespace's level and a module's. A package can deny headers in its module code and
  allow them in the files that wrap a C library.

## 2026-09-29 — MC5 0.3.0: module layout

- **MC5** 0.3.0 (§8, recommended): a module divided by concern -- interface partitions that declare,
  implementation units that define -- and source files under 2000 lines, because with today's
  compilers a body in an interface unit rebuilds every importer; and what MC++ plans so that a
  one-file module builds as fast (a reduced BMI, not rewritten when unchanged).

## 2026-09-29 — MC2 1.2.0: what an importer reaches (M2.2)

- **MC2** 1.2.0 (§3.1): an interface also carries the unit's reachable declarations -- what its
  exported using-declarations name (libc++'s `std` module exports nothing else), the public members of
  the classes among them, recursively, and the enumerators of the enumerations it exports or reaches --
  after the T1 declarations, placed by qualified name, at no place, flagged `reachable`. The JSON form's
  `reachable`. Readers take 1.1.0 and 1.0.0 files, which have none.

## 2026-09-29 — MC1 0.3.0, MC2 1.1.0, MC3 0.4.0: the dialect boundary across imports (M1.2)

- **MC3** 0.4.0: the kind `imports` (§4.13) -- each import of a named module, and what it brings in as
  the modules' MC2 interfaces say (itself and what it re-exports): their dialects and exported T1
  declarations, read from the .ifc, never from a source. Readers take 0.3.0 and older documents,
  which have none.
- **MC2** 1.1.0: `mcxx::reexport` (what an importer also sees); §7, a BMI's interface is also kept in
  a store under the SHA-256 of the BMI's bytes, and a reader that finds none beside a BMI (a build
  copied it, as mcpp's caches do) reads the store's. Readers take 1.0.0 files.
- **MC1** 0.3.0: a waiver on an import, `import m [[mcpp::allow("id")]];` (blanked before Clang reads
  the file), and the manifest's form, `[package.metadata.mcxx.imports."m"] allow = [...]`, for a build
  tool whose scanner does not take an attribute on an import (§5, §7). The catalog's `mc1-version`.
- **MC5** 0.2.0: `mcxx version --json` names MC2's version too. A successful compile keeps the
  interfaces it wrote in MC2's store (MC2 §7).
- **MC4**: `report_imports`, the SDK's helper with which a feature's rule finds what crosses into
  the file at an import from a module whose own dialect does not deny it. Protocol version stays 1.

## 2026-09-29 — MC2 1.0.0: module interfaces in the IFC format

- **MC2** (new): beside every BMI a compile writes, `X.ifc` in IFC format 0.43 -- the unit's T1
  declarations (MC3's, those not local) as IFC declarations in IFC scopes, what IFC has no field for
  in `[[mcxx::decl(...)]]` attributes, and the unit's dialect (profiles, every feature's level for the
  module, the namespaces' levels) in an attribute declaration. A reader rejects what is not MC2 1.x;
  read back, the declarations equal the facts field by field. Schema: an interface as read back.
- **MC3** §4 (MC3-4-4): a qualified name or a type does not depend on how the command line named the
  file -- an unnamed class, union or enum is `(anonymous union)`, `(unnamed struct)`, without its place.
  Before, the editor (an absolute path) and a build (a relative one) gave different names.

## 2026-09-29 — MC3 0.3.0: a declaration's `local`

- **MC3** §4.2: `local`, a declaration in a function's body (a local variable, a local class and what it
  declares), not reachable from outside it; a parameter is not local. What an interface carries is
  what is not local (MC2). Readers take 0.2.0 and 0.1.0 documents, whose declarations have none.

## 2026-09-29 — MC5 0.1.1: a GCC command's module switches

- **MC5** §6: the derived arguments of a GCC command leave out GCC's C++20 modules switches
  (`-fmodules`, `-fmodule-only`, `-fmodule-header`, `-fdeps-*`): to Clang `-fmodules` is its header
  modules, and a GCC-built program (C-mcpp) did not parse in the semantic services with it.

## 2026-09-29 — MC1 0.2.0, MC3 0.2.0, MC4 0.2.0: attributes and regions (M1.9)

- **MC4**: two extension points. A provider claims attributes (`acme::hot`): the compiler accepts
  them and records each use; a rule reads the declaration's facts. A region is an attribute that
  names a profile. Profiles may set levels for features by id. Additive: protocol version stays 1.
- **MC3**: the kind `attributes` (§4.12). Readers take 0.1.0 documents, which have none.
- **MC1**: within a region, its profile's level where stricter, below a declaration's waiver (§6);
  the catalog lists the claimed attributes.

## 2026-09-29 — MC6 1: `mcxx serve`

LSP's base protocol on standard input and output; LSP's document notifications and the service's
requests; MC++'s requests `mcxx/setCommands`, `mcxx/facts` (MC3), `mcxx/gates` (MC1), `mcxx/catalog`;
the host's client restarts a process that ended and replays the commands and the open documents.

## 2026-09-29 — MC1 0.1.0, MC3 0.1.0, MC4 0.1.0 (protocol 1), MC5 0.1.0: first drafts

- **MC1**: features and their categories (`iso`, `policy`, `library`, `pitfall`, `extension`; only
  MC++ defines `iso` features, each with its ISO stable names, and every category but `extension` is
  subtraction); the 15 built-in features of `mc++.iso`; profiles `safe` (the sources of undefined
  behavior a compiler does not check), `modules`, `strict`, `portable`, several at once;
  `[package.metadata.mcxx]`; precedence; `[[mcpp::allow]]`; the audit record; diagnostics; the catalog
  (`mcxx features --json`).
- **MC3**: positions, certainty, and the facts of a file by kind, with their JSON form.
- **MC4**: providers and extension points (rules, source filters, profiles); resolution and overriding
  (`replaces`, conflicts); static composition (`mcxx compose`) and the out-of-process protocol, version
  1 (JSON lines over standard input and output: `hello`/`welcome`, `check`, `filter`, `shutdown`);
  failures. Composition, the protocol and failures are specified ahead of their implementation
  (pending in conformance/traceability.json).
- **MC5**: invocation, the command line (Clang's), environment, `mcxx version --json`, the arguments
  of a unit in the semantic services, and the toolchain contract.
