// mcxx.plugin:language -- language providers: the compiler arguments a file's enabled features need
// (MC4 0.4.0). A language feature MC++'s Clang implements behind an option (a C++26 or C++29 core
// language feature, milestone ML) is on in a file when its level there is not `deny`; the provider
// that registers the feature says which arguments turn it on.
export module mcxx.plugin:language;

import std;
import :feature;
import :rule;
import :filter;

export namespace mcxx::plugin {

// What a language provider is asked about one file: where it is, what it is compiled for, and
// whether a feature id is enabled there (its level is not `deny`, by the file's configuration).
struct LanguageContext {
    std::string_view path;
    const Target& target;
    std::function<bool(std::string_view)> enabled;
};

class LanguageProvider : public Provider {
public:
    // The compiler arguments (in Clang's driver spelling) the file's enabled features need, in order;
    // none for a file where none of the provider's features is enabled.
    virtual std::vector<std::string> arguments(const LanguageContext& context) const = 0;
};

} // namespace mcxx::plugin
