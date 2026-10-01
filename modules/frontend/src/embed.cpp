// #embed's parameters and elements: mcxx.frontend:embed's definitions.
module mcxx.frontend;

import std;
import :embed;

namespace mcxx::frontend {

namespace {

bool is_kind(const PpToken& t, Kind kind) { return t.kind == kind; }

// `__limit__` is `limit`: the standard's attribute-like freedom for an implementation's own spelling of
// a standard parameter (Clang's, and GCC's).
std::string plain(std::string_view name) {
    if (name.size() > 4 && name.starts_with("__") && name.ends_with("__")) return std::string { name.substr(2, name.size() - 4) };
    return std::string { name };
}

} // namespace

EmbedParameters parse_embed_parameters(std::span<const PpToken> tokens) {
    EmbedParameters out;
    const auto fail = [&](std::size_t at, std::string message) {
        out.error_at = at;
        out.error = std::move(message);
        return out;
    };
    std::size_t i { 0 };
    while (i < tokens.size()) {
        if (tokens[i].kind != Kind::raw_identifier) return fail(i, "expected an embed parameter name");
        EmbedParameter p;
        p.at = i;
        p.name = plain(tokens[i].spelling);
        ++i;
        if (i < tokens.size() && is_kind(tokens[i], Kind::coloncolon)) {
            if (i + 1 >= tokens.size() || tokens[i + 1].kind != Kind::raw_identifier) return fail(i, "expected an identifier after '::' in an embed parameter name");
            p.name = plain(p.name) + "::" + plain(tokens[i + 1].spelling);
            i += 2;
        }
        if (i < tokens.size() && is_kind(tokens[i], Kind::l_paren)) {
            // pp-balanced-token-seq: to the matching ')', brackets and braces balanced inside.
            std::vector<Kind> open;
            std::size_t k { i };
            for (; k < tokens.size(); ++k) {
                const Kind kind { tokens[k].kind };
                if (kind == Kind::l_paren || kind == Kind::l_square || kind == Kind::l_brace) {
                    open.push_back(kind);
                } else if (kind == Kind::r_paren || kind == Kind::r_square || kind == Kind::r_brace) {
                    const Kind want { kind == Kind::r_paren ? Kind::l_paren : kind == Kind::r_square ? Kind::l_square : Kind::l_brace };
                    if (open.empty() || open.back() != want) return fail(k, "unbalanced token in an embed parameter");
                    open.pop_back();
                    if (open.empty()) break;
                }
            }
            if (k == tokens.size()) return fail(i, "missing ')' after an embed parameter");
            p.parenthesized = true;
            p.arguments = tokens.subspan(i + 1, k - i - 1);
            i = k + 1;
        }
        out.list.push_back(std::move(p));
    }
    return out;
}

std::uint64_t embed_count(std::uint64_t size, std::uint64_t offset, std::optional<std::uint64_t> limit) {
    const std::uint64_t left { size > offset ? size - offset : 0 };
    return limit ? std::min(*limit, left) : left;
}

std::string embed_elements(std::string_view resource, std::uint64_t offset, std::uint64_t count) {
    std::string out;
    if (offset >= resource.size()) return out;
    const auto bytes = resource.substr(static_cast<std::size_t>(offset), static_cast<std::size_t>(std::min<std::uint64_t>(count, resource.size() - offset)));
    out.reserve(bytes.size() * 5);
    for (std::size_t i { 0 }; i < bytes.size(); ++i) {
        if (i > 0) out += ", ";
        out += std::to_string(static_cast<unsigned>(static_cast<unsigned char>(bytes[i])));
    }
    return out;
}

} // namespace mcxx::frontend
