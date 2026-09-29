// MC++'s plugin wire formats: MC3 facts as JSON (specs/mc3-facts.md §4.11) and the MC4 protocol's
// pieces (specs/mc4-plugins.md §6): features, profiles, providers, findings, targets. Reading is
// strict -- a member of the wrong type is an error naming it -- and writing then reading gives the
// same value (MC3-4.11-2).
export module mcxx.plugin.wire;

export import :api;
