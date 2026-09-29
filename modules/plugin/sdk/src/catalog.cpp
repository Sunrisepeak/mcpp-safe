// The registry, the catalog it is resolved into, a declaration's subtree of facts, source filters applied.
module mcxx.plugin;

import std;
import mcxx.msa;

namespace mcxx::plugin {

namespace {

struct Registered {
    std::unique_ptr<Provider> provider;
    const Rule* rule { nullptr };
    const SourceFilter* filter { nullptr };
    Origin origin { Origin::plugin };
};

struct Registry {
    std::mutex mutex;
    std::vector<Registered> providers;
    std::uint64_t generation { 1 };
    std::shared_ptr<const Catalog> catalog;
};

Registry& registry() {
    static Registry r;
    return r;
}

void add(Registered entry) {
    auto& r = registry();
    std::lock_guard lock { r.mutex };
    r.providers.push_back(std::move(entry));
    ++r.generation;
    r.catalog.reset();
}

// Of several providers of one name (a feature id, a profile), the one used: the only one that says
// it replaces; else a built-in one; else the first. The others are shadowed; a second provider
// that does not say it replaces is a conflict.
template <class T>
std::size_t choose(const std::vector<std::pair<const T*, const Registered*>>& candidates, bool (*replaces)(const T&), std::string_view what,
                   std::string_view name, std::vector<std::string>& problems) {
    std::vector<std::size_t> replacing;
    for (std::size_t i { 0 }; i < candidates.size(); ++i)
        if (replaces(*candidates[i].first)) replacing.push_back(i);
    auto provider_name = [&](std::size_t i) { return std::string { candidates[i].second->provider->name() }; };
    if (replacing.size() > 1) {
        std::string names;
        for (const auto i : replacing) names += (names.empty() ? "" : ", ") + provider_name(i);
        problems.push_back(std::format("{} `{}` is replaced by several providers ({}); {}'s is used", what, name, names, provider_name(replacing.front())));
    }
    if (!replacing.empty()) return replacing.front();
    std::size_t chosen { 0 };
    for (std::size_t i { 0 }; i < candidates.size(); ++i)
        if (candidates[i].second->origin == Origin::builtin) {
            chosen = i;
            break;
        }
    for (std::size_t i { 0 }; i < candidates.size(); ++i)
        if (i != chosen)
            problems.push_back(std::format("{} `{}` is provided by {} and by {}; {}'s is not used (to take it over, mark it `replaces`)", what, name,
                                           provider_name(chosen), provider_name(i), provider_name(i)));
    return chosen;
}

// MC1 §2: an id's shape, and what its category asks of it.
void check_feature(const Catalog::Entry& e, std::vector<std::string>& problems) {
    const Feature& f { *e.feature };
    std::string_view rest { f.id };
    if (rest.starts_with("lib:") || rest.starts_with("ext:")) rest.remove_prefix(4);
    bool well_formed { !rest.empty() };
    char previous { '-' };
    for (const char c : rest) {
        const bool word { (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') };
        if (!word && (c != '-' && c != '.')) well_formed = false;
        if (!word && previous == '-') well_formed = false;   // a separator after a separator, or first
        previous = word ? 'a' : '-';
    }
    if (previous == '-') well_formed = false;
    if (!well_formed) problems.push_back(std::format("feature id `{}` (of {}) is not lower-case words joined by - or . (MC1-2-1)", f.id, e.provider->name()));
    if (f.category == Category::library && !f.id.starts_with("lib:"))
        problems.push_back(std::format("library feature `{}` (of {}) does not start with lib: (MC1-2.1-2)", f.id, e.provider->name()));
    if (f.category == Category::extension && !f.id.starts_with("ext:"))
        problems.push_back(std::format("extension feature `{}` (of {}) does not start with ext: (MC1-2.1-3)", f.id, e.provider->name()));
    if (f.category == Category::iso && f.standard.empty())
        problems.push_back(std::format("ISO feature `{}` (of {}) names no stable name (MC1-2-3)", f.id, e.provider->name()));
    // An ISO feature is MC++'s to define; a plugin may only provide one instead of it.
    if (f.category == Category::iso && e.origin != Origin::builtin && e.shadowed.empty())
        problems.push_back(std::format("`{}` is category iso, but only MC++ defines ISO features; {} may replace one of them (MC1-2.1-1)", f.id,
                                       e.provider->name()));
}

std::shared_ptr<const Catalog> build(const std::vector<Registered>& providers, std::uint64_t generation) {
    auto c = std::make_shared<Catalog>();
    c->generation = generation;
    std::set<std::string, std::less<>> replaced;
    for (const auto& p : providers)
        for (const auto name : p.provider->replaces()) replaced.emplace(name);
    std::vector<const Registered*> active;
    // Built-in providers first: the order rules run and filters apply in.
    for (const auto origin : { Origin::builtin, Origin::plugin })
        for (const auto& p : providers)
            if (p.origin == origin && !replaced.contains(p.provider->name())) active.push_back(&p);
    c->replaced.assign(replaced.begin(), replaced.end());
    // A provider's name is how configuration, replacement and reports name it: unique (MC4-2-1).
    std::set<std::string_view> names;
    for (const auto* p : active)
        if (!names.insert(p->provider->name()).second)
            c->problems.push_back(std::format("two providers are named {}; name each provider uniquely (MC4-2-1)", p->provider->name()));

    std::map<std::string, std::vector<std::pair<const Feature*, const Registered*>>, std::less<>> by_id;
    std::map<std::string, std::vector<std::pair<const Profile*, const Registered*>>, std::less<>> by_profile;
    for (const auto* p : active) {
        c->providers.push_back({ p->provider.get(), p->origin });
        if (p->rule != nullptr) c->rules.push_back(p->rule);
        if (p->filter != nullptr) c->filters.push_back(p->filter);
        for (const auto& f : p->provider->features()) by_id[f.id].emplace_back(&f, p);
        for (const auto& pr : p->provider->profiles()) by_profile[pr.name].emplace_back(&pr, p);
    }
    for (const auto& [id, candidates] : by_id) {
        const std::size_t i { choose<Feature>(candidates, [](const Feature& f) { return f.replaces; }, "feature", id, c->problems) };
        Catalog::Entry entry { candidates[i].first, candidates[i].second->provider.get(), candidates[i].second->origin, {} };
        for (std::size_t j { 0 }; j < candidates.size(); ++j)
            if (j != i) entry.shadowed.emplace_back(candidates[j].second->provider->name());
        check_feature(entry, c->problems);
        c->features.push_back(std::move(entry));
    }
    std::map<std::string, std::vector<const Registered*>, std::less<>> by_attribute;
    for (const auto* p : active)
        for (const auto& a : p->provider->attributes()) by_attribute[a.name].push_back(p);
    for (const auto& [name, owners] : by_attribute) {
        const Registered* owner { owners.front() };
        for (const auto* o : owners)
            if (o->origin == Origin::builtin) owner = o;
        for (const auto* o : owners)
            if (o != owner)
                c->problems.push_back(std::format("attribute [[{}]] is claimed by {} and by {}; {}'s claim is not used", name, owner->provider->name(),
                                                  o->provider->name(), o->provider->name()));
        const auto spec = std::ranges::find(owner->provider->attributes(), name, &AttributeSpec::name);
        c->attributes.push_back({ &*spec, owner->provider.get() });
    }
    for (const auto& [name, candidates] : by_profile) {
        const std::size_t i { choose<Profile>(candidates, [](const Profile& p) { return p.replaces; }, "profile", name, c->problems) };
        c->profiles.push_back({ candidates[i].first, candidates[i].second->provider.get() });
    }
    return c;
}

bool same_shape(std::string_view a, std::string_view b) {
    if (a.size() != b.size()) return false;
    for (std::size_t i { 0 }; i < a.size(); ++i)
        if ((a[i] == '\n') != (b[i] == '\n')) return false;
    return true;
}

} // namespace

void register_rule(std::unique_ptr<Rule> rule, Origin origin) {
    const Rule* raw { rule.get() };
    add({ std::move(rule), raw, nullptr, origin });
}

void register_source_filter(std::unique_ptr<SourceFilter> filter, Origin origin) {
    const SourceFilter* raw { filter.get() };
    add({ std::move(filter), nullptr, raw, origin });
}

std::shared_ptr<const Catalog> catalog() {
    auto& r = registry();
    std::lock_guard lock { r.mutex };
    if (!r.catalog) r.catalog = build(r.providers, r.generation);
    return r.catalog;
}

const Catalog::Entry* Catalog::find(std::string_view id) const {
    const auto it = std::ranges::lower_bound(features, id, {}, [](const Entry& e) { return std::string_view { e.feature->id }; });
    return it != features.end() && it->feature->id == id ? &*it : nullptr;
}

const Profile* Catalog::profile(std::string_view name) const {
    const auto it = std::ranges::lower_bound(profiles, name, {}, [](const ProfileEntry& e) { return std::string_view { e.profile->name }; });
    return it != profiles.end() && it->profile->name == name ? it->profile : nullptr;
}

Origin Catalog::origin_of(const Provider* provider) const {
    for (const auto& p : providers)
        if (p.provider == provider) return p.origin;
    return Origin::plugin;
}

const Catalog::AttributeEntry* Catalog::attribute(std::string_view name) const {
    const auto it = std::ranges::lower_bound(attributes, name, {}, [](const AttributeEntry& e) { return std::string_view { e.attribute->name }; });
    return it != attributes.end() && it->attribute->name == name ? &*it : nullptr;
}

msa::fact::Facts subtree(const msa::fact::Facts& facts, const msa::Range& range) {
    const auto inside = [&](const msa::fact::Place& p) { return range.begin <= p.range.begin && p.range.end <= range.end; };
    msa::fact::Facts out;
    out.certainty = facts.certainty;
    out.collected = facts.collected;
    const auto copy = [&](const auto& from, auto& to) { std::ranges::copy_if(from, std::back_inserter(to), inside); };
    copy(facts.declarations, out.declarations);
    copy(facts.initializations, out.initializations);
    copy(facts.casts, out.casts);
    copy(facts.allocations, out.allocations);
    copy(facts.pointer_arithmetic, out.pointer_arithmetic);
    copy(facts.gotos, out.gotos);
    copy(facts.macros, out.macros);
    copy(facts.uses, out.uses);
    copy(facts.includes, out.includes);
    copy(facts.suppressions, out.suppressions);
    copy(facts.attributes, out.attributes);
    copy(facts.imports, out.imports);
    return out;
}

void report_imports(const msa::fact::Facts& facts, std::string_view feature, std::string_view what,
                    const std::function<bool(const msa::fact::Declaration&)>& exhibits, std::vector<Finding>& out) {
    for (const auto& im : facts.imports) {
        const msa::fact::Declaration* first { nullptr };
        std::string_view from;
        std::size_t more { 0 };
        std::vector<std::string_view> unknown;
        for (const auto& in : im.interfaces) {
            if (!in.found) {
                unknown.push_back(in.module);
                continue;
            }
            if (in.level(feature) == "deny") continue;
            for (const auto& d : in.exported) {
                if (!exhibits(d)) continue;
                if (first == nullptr) {
                    first = &d;
                    from = in.module;
                } else {
                    ++more;
                }
            }
        }
        const std::string through { from.empty() || from == im.module ? std::string {} : std::format(" (through {})", from) };
        if (first != nullptr)
            out.push_back({ std::string { feature }, im.range,
                            std::format("import of {} brings in {} across its dialect: `{}`{}{}{}", im.module, what, first->qualified_name,
                                        first->type.empty() ? std::string {} : std::format(" (`{}`)", first->type), through,
                                        more == 0 ? std::string {} : std::format(", and {} more", more)),
                            im.container });
        if (!unknown.empty()) {
            std::string names;
            for (const auto n : unknown) names += std::format("{}{}", names.empty() ? "" : ", ", n);
            out.push_back({ std::string { feature }, im.range,
                            std::format("import of {}: no MC2 interface of {} was found beside its BMI or in the store, so whether it brings in {} "
                                        "is not known; build it with mcxx",
                                        im.module, names, what),
                            im.container });
        }
    }
}

const Feature* find_feature(std::string_view id) {
    const auto c = catalog();
    const Catalog::Entry* e { c->find(id) };
    return e != nullptr ? e->feature : nullptr;
}

Filtered apply_source_filters(const SourceContext& context, std::string_view text) {
    Filtered result;
    const auto c = catalog();
    if (c->filters.empty()) return result;
    std::string current { text };
    bool changed { false };
    for (const SourceFilter* filter : c->filters) {
        Filtered one { filter->filter(context, current) };
        for (auto& p : one.problems) result.problems.push_back(std::move(p));
        for (auto& f : one.findings) result.findings.push_back(std::move(f));
        if (!one.text) continue;
        if (!same_shape(current, *one.text)) {
            result.problems.push_back({ {}, msa::Severity::error,
                                        std::format("source filter {} changed the text's length or line breaks; its output is not used", filter->name()),
                                        "mcxx-filter", "MC++ plugin", {} });
            continue;
        }
        current = std::move(*one.text);
        changed = true;
    }
    if (changed) result.text = std::move(current);
    return result;
}

} // namespace mcxx::plugin
