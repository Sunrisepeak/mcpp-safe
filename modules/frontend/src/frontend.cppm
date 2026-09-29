// mcxx.frontend: MC++'s own C++ front end (plan P4), beside the Clang backend and without Clang or
// LLVM. F1: lexing (:lex, token for token Clang's raw lexer -- tools/checks/lexdiff.py), phase 4 for
// a module unit (:preprocess), the declarations and the outline (:syntax -- syntaxdiff.py), and the
// MSA facts those give (facts()).
//
//   auto tokens = mcxx::frontend::lex(text);
//   auto pp = mcxx::frontend::preprocess(text, { .file = path });
//   auto facts = mcxx::frontend::facts(pp);   // MC3: macros and includes, for the gates (MC1)
export module mcxx.frontend;

import std;
import mcxx.msa;
export import :unicode;
export import :lex;
export import :preprocess;
export import :syntax;

export namespace mcxx::frontend {

// MC3 facts from what the preprocessor saw: the kinds `macros` (each #define in a group taken; its
// range the name) and `includes` (each #include in a group taken, once per header as written; its
// range the directive's line up to the header's end), as the Clang backend reports them. Certainty
// is `unknown` when the preprocessing was not certain (MC3-3-2).
msa::fact::Facts facts(const Preprocessed& pp);

} // namespace mcxx::frontend

namespace mcxx::frontend {

namespace {

msa::Range range(const Where& at) {
    return { { at.line - 1, at.column - 1 }, { at.line - 1, at.column - 1 + (at.end - at.begin) } };
}

} // namespace

msa::fact::Facts facts(const Preprocessed& pp) {
    msa::fact::Facts out;
    out.collected = msa::fact::Kinds::macros | msa::fact::Kinds::includes;
    out.certainty = pp.certain ? msa::Certainty::certain : msa::Certainty::unknown;
    for (const auto& m : pp.macros) out.macros.push_back({ { range(m.at), {} }, m.name });
    std::set<std::string_view> seen;
    for (const auto& i : pp.includes) {
        if (!seen.insert(i.header).second) continue;   // a header entered once, as a guarded one is
        msa::fact::Include include;
        include.range = { { i.at.line - 1, 0 }, { i.at.line - 1, i.at.end - i.at.begin } };
        include.header = i.header;
        include.global_module_fragment = i.global_module_fragment;
        out.includes.push_back(std::move(include));
    }
    return out;
}

} // namespace mcxx::frontend
