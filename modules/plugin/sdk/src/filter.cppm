// mcxx.plugin:filter -- source filters: a provider that rewrites a file's text before it is parsed.
export module mcxx.plugin:filter;

import std;
import mcxx.msa;
import :feature;
import :rule;

export namespace mcxx::plugin {

// ---- Source filters: text before parsing ---------------------------------------------------------
//
// A source filter sees a file's text before it is parsed, with the target it is compiled for, and
// may return a replacement: `[[mcpp::cfg(windows)]]` (plugins/std cfg) blanks what the target does
// not have. The replacement keeps the text's length and its line breaks, so every position a
// diagnostic, an index or an editor names stays where it was; a host refuses one that does not.

// The target a file is compiled for, in the words a configuration uses.
struct Target {
    std::string triple;               // "x86_64-unknown-linux-gnu"
    std::string os;                   // "linux", "windows", "macos", "ios", "android", "freebsd", "wasi", "none"
    std::string family;               // "unix", "windows" or ""
    std::string arch;                 // "x86_64", "aarch64", "riscv64", "wasm32", ...
    std::string env;                  // "gnu", "musl", "msvc", "" ...
    unsigned pointer_width { 64 };
    std::string endian;               // "little" | "big"
    std::vector<std::string> features;   // the active mcpp features, as their MCPP_FEATURE_<NAME> names ("SIMD_AVX")
    bool debug_assertions { false };  // NDEBUG is not defined
};

// A feature name as mcpp spells its macro: uppercased, every other character an underscore.
std::string feature_macro_name(std::string_view feature);

struct SourceContext {
    std::string_view path;
    const Target& target;
};

struct Filtered {
    std::optional<std::string> text;       // the replacement, or nothing when the text is unchanged
    std::vector<msa::Diagnostic> problems; // what the filter could not understand: errors in the file
    std::vector<Finding> findings;         // uses of the filter's own features (an extension's), gated as a rule's are
};

class SourceFilter : public Provider {
public:
    virtual Filtered filter(const SourceContext& context, std::string_view text) const = 0;
};

} // namespace mcxx::plugin
