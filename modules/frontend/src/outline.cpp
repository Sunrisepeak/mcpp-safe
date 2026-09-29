// Where a declaration is (its name's range, its whole range) and the outline of a file (symbols()).
module mcxx.frontend;

import std;
import mcxx.msa;
import mcxx.base;

namespace mcxx::frontend {

msa::Position position(const Syntax& syntax, std::uint32_t offset) {
    const auto it = std::ranges::upper_bound(syntax.line_starts, offset);
    const auto line = static_cast<std::uint32_t>(it - syntax.line_starts.begin() - 1);
    return { line, offset - *(it - 1) };
}

namespace {

// Positions as Clang gives them: a token in a macro's expansion is where the outermost invocation
// starts; a name ends its spelled length after its start, a range the length of the file's token at
// its last token's place (the macro's name, for one an expansion gave) -- on that token's line,
// however many lines the token spans.
msa::Position after(const Syntax& syntax, std::uint32_t begin, std::uint32_t length) {
    const auto p = position(syntax, begin);
    return { p.line, p.column + length };
}

msa::Position end_of(const Syntax& syntax, const PpToken& t) {
    const std::uint32_t length { t.expanded ? t.macro_end - t.at.begin : t.at.end - t.at.begin };
    return after(syntax, t.at.begin, length);
}

} // namespace

msa::Range selection_range(const Syntax& syntax, const Declaration& d) {
    const auto& tokens = syntax.pp.tokens;
    if (d.name_token >= tokens.size()) return msa::Range { position(syntax, d.name_at.begin), position(syntax, d.name_at.end) };
    const auto& t = tokens[d.name_token];
    const std::uint32_t length { t.expanded ? static_cast<std::uint32_t>(t.spelling.size()) : t.at.end - t.at.begin };
    return msa::Range { position(syntax, t.at.begin), after(syntax, t.at.begin, std::max(1u, length)) };
}

msa::Range whole_range(const Syntax& syntax, const Declaration& d) {
    const auto& tokens = syntax.pp.tokens;
    if (d.last_token >= tokens.size() || d.first_token >= tokens.size()) return msa::Range { position(syntax, d.at.begin), position(syntax, d.at.end) };
    return msa::Range { position(syntax, tokens[d.first_token].at.begin), end_of(syntax, tokens[d.last_token]) };
}

msa::Range token_range(const Syntax& syntax, std::uint32_t first, std::uint32_t last) {
    const auto& tokens = syntax.pp.tokens;
    if (tokens.empty()) return {};
    first = std::min<std::uint32_t>(first, static_cast<std::uint32_t>(tokens.size() - 1));
    last = std::min<std::uint32_t>(std::max(first, last), static_cast<std::uint32_t>(tokens.size() - 1));
    return msa::Range { position(syntax, tokens[first].at.begin), end_of(syntax, tokens[last]) };
}

std::vector<msa::Symbol> symbols(const Syntax& syntax) {
    // Children in the order written: a declaration's index is after its parent's.
    std::vector<std::vector<std::size_t>> children(syntax.declarations.size() + 1);
    for (std::size_t i { 0 }; i < syntax.declarations.size(); ++i) {
        const auto& d = syntax.declarations[i];
        children[static_cast<std::size_t>(d.parent + 1)].push_back(i);
    }
    const auto selection = [&](const Declaration& d) { return selection_range(syntax, d); };
    const auto whole = [&](const Declaration& d) { return whole_range(syntax, d); };
    std::function<void(std::size_t, std::vector<msa::Symbol>&)> walk = [&](std::size_t slot, std::vector<msa::Symbol>& out) {
        for (const auto i : children[slot]) {
            const auto& d = syntax.declarations[i];
            if (!d.listed || d.name.empty() || d.kind == msa::Kind::unknown) continue;
            msa::Symbol s;
            s.name = d.name;
            s.kind = d.kind;
            s.range = whole(d);
            s.selection = selection(d);
            if (d.kind == msa::Kind::namespace_ || d.kind == msa::Kind::class_ || d.kind == msa::Kind::struct_ || d.kind == msa::Kind::union_ ||
                d.kind == msa::Kind::enum_)
                walk(i + 1, s.children);
            out.push_back(std::move(s));
        }
    };
    std::vector<msa::Symbol> out;
    walk(0, out);
    return out;
}

} // namespace mcxx::frontend
