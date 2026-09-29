// mcxx.frontend's own definitions: the facts F1 gives, the annotations of imports.
module mcxx.frontend;

import std;
import mcxx.msa;

namespace mcxx::frontend {

namespace {

// A declaration's kind as MSA's facts have it.
msa::Kind fact_kind(const Declaration& d) { return d.kind; }

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

msa::fact::Facts facts(const Syntax& syntax) {
    auto out = facts(syntax.pp);
    // Not declaration_types: a type is given where the tokens say it (not a deduced one, not the
    // templates an alias names), and a gate needing types is not F1's to decide.
    out.collected = msa::fact::Kinds::macros | msa::fact::Kinds::includes | msa::fact::Kinds::declarations | msa::fact::Kinds::gotos |
                    msa::fact::Kinds::allocations | msa::fact::Kinds::casts | msa::fact::Kinds::uses | msa::fact::Kinds::suppressions;
    const auto& ds = syntax.declarations;
    // Each declaration's namespace (container) and qualified name (names_of: what name lookup uses
    // too). And whether it is exported: inside `export`, or inside what is (a parameter, a member, a
    // local), as Clang's isInExportDeclContext has it.
    const Names names { names_of(syntax) };
    const auto& container = names.container;
    const auto& qualified = names.qualified;
    std::vector<bool> exported(ds.size());
    for (std::size_t i { 0 }; i < ds.size(); ++i) exported[i] = ds[i].exported || (ds[i].parent >= 0 && exported[static_cast<std::size_t>(ds[i].parent)]);
    for (std::size_t i { 0 }; i < ds.size(); ++i) {
        const auto& d = ds[i];
        const auto whole = whole_range(syntax, d);
        // Declarations: what MSA's facts hold (variables, fields, parameters, functions, aliases, class
        // and enum definitions, namespaces).
        const bool record { d.kind == msa::Kind::class_ || d.kind == msa::Kind::struct_ || d.kind == msa::Kind::union_ || d.kind == msa::Kind::enum_ };
        const bool declared { !d.binding && (d.kind == msa::Kind::variable || d.kind == msa::Kind::field || d.kind == msa::Kind::parameter ||
                              d.kind == msa::Kind::function || d.kind == msa::Kind::method || d.kind == msa::Kind::constructor ||
                              d.kind == msa::Kind::destructor || d.kind == msa::Kind::conversion || d.kind == msa::Kind::type_alias ||
                              (record && d.definition) || d.kind == msa::Kind::namespace_) };
        if (declared) {
            msa::fact::Declaration f;
            f.range = whole;
            f.name = selection_range(syntax, d);
            // An alias template's outline is at its `using`, as Clang's is; the alias itself (a fact)
            // is at its name.
            if (d.kind == msa::Kind::type_alias && d.name_token + 1 < syntax.pp.tokens.size() && syntax.pp.tokens[d.name_token].spelling == "using")
                f.name = token_range(syntax, d.name_token + 1, d.name_token + 1);
            f.container = container[i];
            f.qualified_name = qualified[i];
            f.kind = fact_kind(d);
            f.exported = exported[i];
            f.pointer = d.pointer;
            f.c_array = d.c_array;
            // va_list: an array of one struct on x86-64 Linux (the SysV ABI), a pointer on macOS and Windows.
            if (d.va_list) (syntax.pp.target.starts_with("x86_64") && syntax.pp.target.find("linux") != std::string::npos ? f.c_array : f.pointer) = true;
            f.c_variadic = d.c_variadic;
            f.is_union = d.kind == msa::Kind::union_;
            // Its type as written (MC3's `type`), where the tokens say it: a function's only when it
            // returns a pointer (its return type), as the Clang backend gives it.
            const bool function_kind { d.kind == msa::Kind::function || d.kind == msa::Kind::method || d.kind == msa::Kind::constructor ||
                                       d.kind == msa::Kind::destructor || d.kind == msa::Kind::conversion };
            if (!function_kind) f.type = type_text(syntax, d);
            else if (d.pointer) f.type = type_text(syntax, d, true);
            // Inside a function: a parameter's parent is the function, a local's too (or a local class's);
            // or inside a lambda, whose call operator is a function (one initializing a variable, say).
            if (d.kind != msa::Kind::parameter) f.local = d.in_lambda;
            if (d.kind != msa::Kind::parameter && !f.local)
                for (auto p = d.parent; p >= 0; p = ds[static_cast<std::size_t>(p)].parent) {
                    const auto k = ds[static_cast<std::size_t>(p)].kind;
                    if (k == msa::Kind::function || k == msa::Kind::method || k == msa::Kind::constructor || k == msa::Kind::destructor ||
                        k == msa::Kind::conversion) {
                        f.local = true;
                        break;
                    }
                }
            out.declarations.push_back(std::move(f));
        }
        for (const auto& [ids, reason] : d.allows) {
            msa::fact::Suppression w;
            w.range = whole;
            w.container = container[i];
            w.declaration = qualified[i];
            std::string_view rest { ids };
            while (!rest.empty()) {
                const auto comma = rest.find(',');
                std::string_view id { rest.substr(0, comma) };
                while (!id.empty() && id.front() == ' ') id.remove_prefix(1);
                while (!id.empty() && id.back() == ' ') id.remove_suffix(1);
                if (!id.empty()) w.ids.emplace_back(id);
                if (comma == std::string_view::npos) break;
                rest.remove_prefix(comma + 1);
            }
            w.reason = reason;
            out.suppressions.push_back(std::move(w));
        }
    }
    for (const auto& c : syntax.constructs) {
        const msa::Range r { token_range(syntax, c.first_token, c.last_token) };
        const std::string at { c.owner >= 0 && static_cast<std::size_t>(c.owner) < ds.size() ? container[static_cast<std::size_t>(c.owner)] : std::string {} };
        using W = Construct::What;
        switch (c.what) {
        case W::goto_: out.gotos.push_back({ { r, at }, c.detail }); break;
        case W::new_: out.allocations.push_back({ { r, at }, false, c.array, c.detail }); break;
        case W::delete_: out.allocations.push_back({ { r, at }, true, c.array, c.detail }); break;
        case W::cast: {
            msa::fact::Cast cast;
            cast.range = r;
            cast.container = at;
            cast.kind = c.detail == "static_cast"    ? msa::fact::CastKind::static_cast_
                        : c.detail == "dynamic_cast" ? msa::fact::CastKind::dynamic_cast_
                        : c.detail == "const_cast"   ? msa::fact::CastKind::const_cast_
                                                     : msa::fact::CastKind::reinterpret_cast_;
            cast.to = c.to;
            cast.reinterprets = cast.kind == msa::fact::CastKind::reinterpret_cast_;
            out.casts.push_back(std::move(cast));
            break;
        }
        case W::throw_: out.uses.push_back({ { r, at }, "throw", {} }); break;
        case W::try_: out.uses.push_back({ { r, at }, "try", {} }); break;
        case W::typeid_: out.uses.push_back({ { r, at }, "typeid", c.detail }); break;
        case W::asm_: out.uses.push_back({ { r, at }, "asm", {} }); break;
        case W::va_arg: out.uses.push_back({ { r, at }, "va_arg", {} }); break;
        }
    }
    return out;
}

namespace {

// A string literal's contents, if it is a plain one without escapes worth reading (feature ids,
// reasons): its quotes taken off.
std::optional<std::string> literal_text(std::string_view text, const Token& t) {
    if (t.kind != Kind::string_literal) return std::nullopt;
    const std::string s { spelling(text, t) };
    if (s.size() < 2 || s.front() != '"' || s.back() != '"') return std::nullopt;
    return s.substr(1, s.size() - 2);
}

} // namespace

std::vector<ImportAnnotation> import_annotations(std::string_view text) {
    std::vector<ImportAnnotation> out;
    const auto tokens = lex(text);
    const auto word = [&](std::size_t i, std::string_view w) {
        return i < tokens.size() && tokens[i].kind == Kind::raw_identifier && text.substr(tokens[i].begin, tokens[i].end - tokens[i].begin) == w;
    };
    const auto is = [&](std::size_t i, Kind k) { return i < tokens.size() && tokens[i].kind == k; };
    const auto position = [&](const Token& t) { return msa::Position { t.line - 1, t.column - 1 }; };
    bool directive { false };
    for (std::size_t i { 0 }; i < tokens.size(); ++i) {
        if (tokens[i].start_of_line) directive = tokens[i].kind == Kind::hash;
        if (directive || !word(i, "import")) continue;
        // A declaration: first on its line, or after `export`, `;` or a brace.
        const bool exported { i > 0 && word(i - 1, "export") };
        const std::size_t first { exported ? i - 1 : i };
        if (!(tokens[first].start_of_line || (first > 0 && (is(first - 1, Kind::semi) || is(first - 1, Kind::r_brace) || is(first - 1, Kind::l_brace)))))
            continue;
        std::size_t j { i + 1 };
        std::string named;
        if (is(j, Kind::colon)) {
            named = ":";
            ++j;
        }
        while (is(j, Kind::raw_identifier)) {
            named.append(text.substr(tokens[j].begin, tokens[j].end - tokens[j].begin));
            ++j;
            if (is(j, Kind::period) && is(j + 1, Kind::raw_identifier)) {
                named += '.';
                ++j;
            } else if (is(j, Kind::colon) && !named.starts_with(":") && is(j + 1, Kind::raw_identifier)) {
                named += ':';
                ++j;
            } else {
                break;
            }
        }
        if (named.empty() || named == ":" || !is(j, Kind::l_square) || !is(j + 1, Kind::l_square)) continue;
        // [[ mcpp::allow("...", "...") , ... ]] and nothing else.
        ImportAnnotation a;
        a.module = named;
        a.begin = tokens[j].begin;
        std::size_t k { j + 2 };
        bool ok { true };
        while (ok) {
            if (!(word(k, "mcpp") && is(k + 1, Kind::coloncolon) && word(k + 2, "allow") && is(k + 3, Kind::l_paren))) {
                ok = false;
                break;
            }
            k += 4;
            std::vector<std::string> args;
            while (ok) {
                const auto value = k < tokens.size() ? literal_text(text, tokens[k]) : std::nullopt;
                if (!value) {
                    ok = false;
                    break;
                }
                args.push_back(*value);
                ++k;
                if (is(k, Kind::comma)) ++k;
                else break;
            }
            if (!ok || !is(k, Kind::r_paren) || args.empty() || args.size() > 2) {
                ok = false;
                break;
            }
            ++k;
            for (auto id : std::views::split(std::string_view { args[0] }, ',')) {
                std::string_view v { id.begin(), id.end() };
                while (!v.empty() && v.front() == ' ') v.remove_prefix(1);
                while (!v.empty() && v.back() == ' ') v.remove_suffix(1);
                if (!v.empty()) a.ids.emplace_back(v);
            }
            if (args.size() == 2) a.reason = args[1];
            if (is(k, Kind::comma)) {
                ++k;
                continue;
            }
            break;
        }
        if (!ok || !is(k, Kind::r_square) || !is(k + 1, Kind::r_square) || !is(k + 2, Kind::semi)) continue;
        a.end = tokens[k + 1].end;
        a.range = { position(tokens[first]), { tokens[k + 2].line - 1, tokens[k + 2].column } };
        out.push_back(std::move(a));
        i = k + 2;
    }
    return out;
}

std::string blank_import_annotations(std::string_view text, std::span<const ImportAnnotation> annotations) {
    std::string out { text };
    for (const auto& a : annotations)
        for (std::uint32_t b { a.begin }; b < a.end && b < out.size(); ++b)
            if (out[b] != '\n' && out[b] != '\r') out[b] = ' ';
    return out;
}

} // namespace mcxx::frontend
