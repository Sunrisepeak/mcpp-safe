// mcxx.frontend:embed -- what #embed and __has_embed (P1967R14, with P3540R3's offset) are made of that
// does not depend on the preprocessor's state: the embed-parameter-seq read from tokens, and the elements
// a resource gives under its parameters. The directive itself (the search, the macro expansion, the limit's
// constant-expression, the replacement) is :preprocess's definition.
//
//   const auto parsed = parse_embed_parameters(tokens);        // limit(3) prefix(0,) clang::offset(1)
//   const auto count = embed_count(resource.size(), offset, limit);
//   const std::string elements = embed_elements(resource, offset, count);    // "104, 101, 108"
export module mcxx.frontend:embed;

import std;
import :preprocess;

export namespace mcxx::frontend {

// One embed-parameter ([cpp.pre]): `limit ( pp-balanced-token-seq )`, `prefix (...)`, `suffix (...)`,
// `if_empty (...)`, `offset (...)`, and the prefixed `identifier :: identifier [( ... )]` an implementation
// may support (`clang::offset`).
struct EmbedParameter {
    std::string name;                        // `limit`, `offset`, `clang::offset`; `__limit__` is `limit`
    std::size_t at { 0 };                    // the name's index in the tokens
    bool parenthesized { false };
    std::span<const PpToken> arguments;      // the tokens inside the parentheses, nothing for none
    bool prefixed() const { return name.find("::") != std::string::npos; }
};

struct EmbedParameters {
    std::vector<EmbedParameter> list;
    std::optional<std::size_t> error_at;     // a token that is no embed-parameter-seq, and why
    std::string error;
};

// Reads the tokens after the resource name as an embed-parameter-seq. The names are not judged (which
// are standard, which are supported, which twice): `error` is for what is no sequence at all.
EmbedParameters parse_embed_parameters(std::span<const PpToken> tokens);

// The resource-count ([cpp.embed.gen]): max(min(limit, size - offset), 0) with a limit, max(size - offset, 0)
// without; `offset` is the resource-offset.
std::uint64_t embed_count(std::uint64_t size, std::uint64_t offset, std::optional<std::uint64_t> limit);

// The comma-delimited list of integer literals of type int that `count` bytes of the resource from `offset`
// are: "104, 101, 108".
std::string embed_elements(std::string_view resource, std::uint64_t offset, std::uint64_t count);

} // namespace mcxx::frontend
