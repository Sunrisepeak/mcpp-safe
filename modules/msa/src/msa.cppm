// The MC++ semantic API (MSA, spec MC3): what a compiler front end knows about a C++ modules
// program, in values that name no compiler's types.
//
// Everything above a backend -- the LSP service, rules, mcppls -- is written against this module
// only; `mcxx.backend.clang` implements it over Clang today, and MC++'s own front end will implement it
// later. A backend's type never appears here (architecture plan P1).
//
// Positions: 0-based lines, columns in UTF-8 bytes. The LSP layer converts to UTF-16.
export module mcxx.msa;

export import :basics;
export import :entities;
export import :facts;
export import :service;
