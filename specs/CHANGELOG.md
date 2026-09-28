# Changelog

Changes to the specifications in this directory. Each specification is versioned independently.

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
