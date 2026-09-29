// mc++.iso: MC++'s built-in feature controls -- ISO C++ language features, each named by the
// standard's stable names, each one a subtraction: a program that does not use it is still ISO
// C++, compiled by any compiler. Written against the plugin SDK like every plugin, registered as
// built-in; a plugin may provide one of these ids instead (Feature::replaces) or stand in for the
// whole provider (Provider::replaces "mc++.iso").
//
// Profiles defined here:
//   safe      the sources of undefined behavior a compiler cannot check for you: pointer
//             arithmetic, new/delete, reinterpret_cast and C-style casts, const_cast, unions,
//             C arrays, C varargs, uninitialized locals, inline asm
//   modules   every dependency by import: no #include outside a global module fragment
//   strict    safe and modules, and goto and macro definitions
//   portable  ISO C++ only: every extension feature (a plugin's, [[mcpp::cfg]]) is denied
// exceptions and rtti are in no profile: a package that does without them says so.
export module mcxx.features.iso;

import std;
import mcxx.msa;
import mcxx.plugin;

export namespace mcxx::features::iso {

inline constexpr std::string_view PROVIDER { "mc++.iso" };

class Rules final : public plugin::Rule {
public:
    Rules();
    std::string_view name() const override { return PROVIDER; }
    std::span<const plugin::Feature> features() const override { return features_; }
    std::span<const plugin::Profile> profiles() const override { return profiles_; }
    void check(const plugin::Context& context, std::vector<plugin::Finding>& out) const override;

private:
    std::vector<plugin::Feature> features_;
    std::vector<plugin::Profile> profiles_;
};

} // namespace mcxx::features::iso
