// [[mcpp::cfg(predicate)]]: a declaration that exists for some targets only, as Rust's #[cfg].
//
//   [[mcpp::cfg(windows)]] void open_console();                  // family (or os) windows
//   [[mcpp::cfg(target_os = "linux")]] int use_epoll();
//   [[mcpp::cfg(any(unix, target_os = "wasi"))]] ...
//   [[mcpp::cfg(all(target_arch = "x86_64", not(debug_assertions)))]] ...
//   [[mcpp::cfg(feature = "simd")]] ...                          // an active mcpp feature
//
// Keys (the target_ prefix is optional): target_os, target_family, target_arch, target_env,
// target_pointer_width, target_endian; feature. Bare names: windows, unix (families), linux, macos,
// ios, android, freebsd, wasi (systems), debug_assertions (NDEBUG undefined).
//
// It is a source filter: before the file is parsed, a declaration the target does not satisfy is
// blanked -- characters become spaces, line breaks stay -- so the compiler never sees it (it may
// name what only another platform declares) and every position after it is unchanged. When the
// target satisfies it, only the attribute is blanked. Where it applies: declarations and
// statements that end with `;` or a block; `export` before it goes with it. Not on `import`: a
// build's dependency scan reads imports before any compiler plugin runs.
//
// Each use is reported as a finding of `ext:cfg` (category extension), gated like any feature:
// allowed by default, denied under profile portable.
export module mcxx.plugins.cfg;

import std;
import mcxx.msa;
import mcxx.plugin;

export namespace mcxx::plugins::cfg {

// Whether the target satisfies a predicate (the text inside cfg(...)); an error says what is wrong.
std::expected<bool, std::string> evaluate(std::string_view predicate, const plugin::Target& target);

// Using it makes a program MC++'s, not ISO C++: an extension (profile portable denies it).
inline constexpr std::string_view FEATURE { "ext:cfg" };

class Filter final : public plugin::SourceFilter {
public:
    std::string_view name() const override { return "mcxx.plugins.cfg"; }
    std::span<const plugin::Feature> features() const override { return features_; }
    plugin::Filtered filter(const plugin::SourceContext& context, std::string_view text) const override;

private:
    std::vector<plugin::Feature> features_ { plugin::Feature {
        .id = std::string { FEATURE },
        .category = plugin::Category::extension,
        .layer = "text",
        .summary = "[[mcpp::cfg(...)]]: a declaration for some targets only",
        .fix = "use a platform module selected by the build (mcpp's per-target sources) or if constexpr on mcxx.os constants",
        .needs = msa::fact::Kinds::none,
    } };
};

} // namespace mcxx::plugins::cfg
