# Changelog

Changes to the specifications in this directory. Each specification is versioned independently.

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
