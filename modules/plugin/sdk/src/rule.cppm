// mcxx.plugin:rule -- providers and rules: what a rule is asked (Context) and what it reports
// (Finding, Failure).
export module mcxx.plugin:rule;

import std;
import mcxx.msa;
import :feature;

export namespace mcxx::plugin {

struct Finding {
    std::string feature;
    msa::Range range;
    std::string message;
    std::string container;      // the enclosing namespace, for namespace-scoped levels
};

// A provider that could not do what it was asked for a file -- a plugin process that crashed, did
// not answer in time, or answered what MC4 does not allow (MC4 §5). The host reports it at the
// file; the features were not checked there.
struct Failure {
    std::string provider;
    std::string reason;
    std::vector<std::string> features;
};

struct Context {
    std::string_view path;
    std::string_view module;    // "m", "m:p", or "" for a non-module unit
    const msa::fact::Facts& facts;
    // The features this rule is asked for: those it provides that are not `allow` anywhere in the
    // file. Null: all of them (a test, a listing).
    const std::vector<std::string>* wanted { nullptr };
    std::vector<Failure>* failures { nullptr };

    bool wants(std::string_view id) const { return wanted == nullptr || std::ranges::find(*wanted, id) != wanted->end(); }
    // Says that the rule could not check the features it was asked for here.
    void fail(std::string provider, std::string reason) const {
        if (failures == nullptr) return;
        failures->push_back({ std::move(provider), std::move(reason), wanted != nullptr ? *wanted : std::vector<std::string> {} });
    }
};

// What every plugin is: a name, what it can gate, the profiles it defines, and whom it stands in for.
class Provider {
public:
    virtual ~Provider() = default;
    virtual std::string_view name() const = 0;   // "mc++.iso", "mcxx.plugins.json"
    virtual std::span<const Feature> features() const { return {}; }
    virtual std::span<const Profile> profiles() const { return {}; }
    virtual std::span<const AttributeSpec> attributes() const { return {}; }
    // Providers this one replaces, by name: they are not run, and their features and profiles are
    // this one's to provide, or nobody's.
    virtual std::span<const std::string_view> replaces() const { return {}; }
};

// Reads facts, reports findings.
class Rule : public Provider {
public:
    virtual void check(const Context& context, std::vector<Finding>& out) const = 0;
};

} // namespace mcxx::plugin
