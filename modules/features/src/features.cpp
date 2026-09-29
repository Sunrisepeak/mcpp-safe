module mcxx.features;

import std;
import mcxx.msa;
import mcxx.plugin;
import mcxx.base;

namespace mcxx::features {
namespace {

namespace toml = mcxx::base::toml;

void read_levels(const toml::Table& table, LevelMap& into, std::string_view where, std::vector<std::string>& problems) {
    for (const auto& [id, value] : table) {
        if (!value.is_string()) {
            problems.push_back(std::format("{}: {} is not a level (allow, warn or deny)", where, id));
            continue;
        }
        if (const auto level = plugin::parse_level(value.as_string())) into.insert_or_assign(id, *level);
        else problems.push_back(std::format("{}: {} = \"{}\" is not a level (allow, warn or deny)", where, id, value.as_string()));
    }
}

bool within(const msa::Range& outer, const msa::Range& inner) { return outer.begin <= inner.begin && inner.end <= outer.end; }

std::string json_string(std::string_view text) {
    std::string out { "\"" };
    for (const char c : text) {
        if (c == '"' || c == '\\') out += '\\';
        if (c == '\n') {
            out += "\\n";
            continue;
        }
        out += c;
    }
    return out + "\"";
}

} // namespace

// Upward by mcxx.base's path rules: std::filesystem's are the C library's, POSIX on every system, to
// which a Windows path (`C:/Users/x/src/a.cpp`) is relative.
std::optional<std::string> find_manifest(std::string_view source_path) {
    std::error_code ec;
    std::string path { source_path };
    if (!base::is_absolute_path(path)) path = base::join_path(std::filesystem::current_path(ec).generic_string(), path);
    std::string dir { base::parent_path(path) };
    while (!dir.empty()) {
        const std::string candidate { base::join_path(dir, "mcpp.toml") };
        if (std::filesystem::is_regular_file(candidate, ec)) {
            if (auto doc = toml::parse_file(candidate); doc && doc->get_table("package") != nullptr) return candidate;
        }
        const std::string up { base::parent_path(dir) };
        if (up == dir) break;
        dir = up;
    }
    return std::nullopt;
}

Config parse_config(std::string_view manifest_text, std::string manifest_path) {
    Config config;
    config.manifest = std::move(manifest_path);
    auto doc = toml::parse(manifest_text);
    if (!doc) {
        config.problems.push_back(std::format("{}:{}: {}", config.manifest, doc.error().where.line, doc.error().message));
        return config;
    }
    const toml::Table* mcxx { doc->get_table("package.metadata.mcxx") };
    if (mcxx == nullptr) return config;
    const std::string where { std::format("{} [package.metadata.mcxx]", config.manifest) };
    if (const auto it = mcxx->find("profile"); it != mcxx->end()) {
        if (it->second.is_string()) {
            config.profiles.push_back(it->second.as_string());
        } else if (it->second.is_array()) {
            for (const auto& v : it->second.as_array()) {
                if (v.is_string()) config.profiles.push_back(v.as_string());
                else config.problems.push_back(where + ": profile holds something that is not a profile's name");
            }
        } else {
            config.problems.push_back(where + ": profile is not a name or a list of names");
        }
    }
    if (const auto it = mcxx->find("plugins"); it != mcxx->end()) {
        if (!it->second.is_table()) config.problems.push_back(where + ": plugins is not a table");
        else
            for (const auto& [name, value] : it->second.as_table()) {
                const std::string at { std::format("{}.plugins.{}", where, name) };
                if (!value.is_table()) {
                    config.problems.push_back(at + " is not a table");
                    continue;
                }
                const auto& t = value.as_table();
                PluginEntry entry { .name = name };
                for (const auto& [key, v] : t) {
                    if (key == "path" && v.is_string()) entry.path = v.as_string();
                    else if (key == "library" && v.is_string()) entry.library = v.as_string();
                    else if (key == "version" && v.is_string()) entry.version = v.as_string();
                    else if (key == "timeout-ms" && v.is_int() && v.as_int() > 0) entry.timeout = std::chrono::milliseconds { v.as_int() };
                    else if (key == "command" && v.is_array()) {
                        for (const auto& part : v.as_array())
                            if (part.is_string()) entry.command.push_back(part.as_string());
                        if (entry.command.size() != v.as_array().size() || entry.command.empty())
                            config.problems.push_back(at + ": command is not a list of strings");
                    } else config.problems.push_back(std::format("{}: {} is not something a plugin entry has", at, key));
                }
                // Exactly one way to reach it (MC4-3-2).
                const int ways { !entry.path.empty() + !entry.version.empty() + !entry.command.empty() + !entry.library.empty() };
                if (ways != 1) {
                    config.problems.push_back(std::format("{}: a plugin is static (path or version), a library (library) or out of process "
                                                          "(command), exactly one of them (MC4-3-2)", at));
                    continue;
                }
                config.plugins.push_back(std::move(entry));
            }
    }
    if (const auto it = mcxx->find("features"); it != mcxx->end() && it->second.is_table())
        read_levels(it->second.as_table(), config.package, where + ".features", config.problems);
    for (const auto& [key, into] : { std::pair { std::string_view { "modules" }, &config.modules },
                                     std::pair { std::string_view { "namespaces" }, &config.namespaces },
                                     std::pair { std::string_view { "files" }, &config.files } }) {
        const auto it = mcxx->find(key);
        if (it == mcxx->end() || !it->second.is_table()) continue;
        for (const auto& [name, table] : it->second.as_table()) {
            if (!table.is_table()) continue;
            read_levels(table.as_table(), (*into)[name], std::format("{}.{}.\"{}\"", where, key, name), config.problems);
        }
    }
    if (const auto it = mcxx->find("imports"); it != mcxx->end()) {
        if (!it->second.is_table()) config.problems.push_back(where + ": imports is not a table");
        else
            for (const auto& [name, value] : it->second.as_table()) {
                const std::string at { std::format("{}.imports.\"{}\"", where, name) };
                if (!value.is_table()) {
                    config.problems.push_back(at + " is not a table");
                    continue;
                }
                ImportAllowance allowance;
                bool listed { false };
                for (const auto& [key, v] : value.as_table()) {
                    if (key == "allow" && v.is_array()) {
                        listed = true;
                        for (const auto& id : v.as_array()) {
                            if (id.is_string()) allowance.ids.push_back(id.as_string());
                            else config.problems.push_back(at + ": allow holds something that is not a feature id");
                        }
                    } else if (key == "reason" && v.is_string()) {
                        allowance.reason = v.as_string();
                    } else {
                        config.problems.push_back(std::format("{}: {} is not something an import's entry has (allow, reason)", at, key));
                    }
                }
                if (!listed) config.problems.push_back(at + ": allow, the list of feature ids, is missing");
                config.imports.emplace(name, std::move(allowance));
            }
    }
    return config;
}

Config read_config(std::string_view manifest_path) {
    std::ifstream in { std::filesystem::path { manifest_path } };
    const std::string text { std::istreambuf_iterator<char> { in }, {} };
    return parse_config(text, std::string { manifest_path });
}

std::optional<Level> profile_level(const plugin::Catalog& catalog, const plugin::Feature& feature, std::span<const std::string> profiles) {
    std::optional<Level> level;
    auto raise = [&](Level l) { level = level ? std::max(*level, l) : l; };
    std::vector<std::string_view> seen;
    // A profile's own levels, and those of the profiles it includes (each once: includes may cycle).
    auto visit = [&](this auto& self, std::string_view name) -> void {
        if (std::ranges::find(seen, name) != seen.end()) return;
        seen.push_back(name);
        for (const auto& [profile, value] : feature.profiles)
            if (profile == name) raise(value);
        const plugin::Profile* profile { catalog.profile(name) };
        if (profile == nullptr) return;
        for (const auto& [category, value] : profile->categories)
            if (category == feature.category) raise(value);
        for (const auto& [id, value] : profile->features)
            if (id == feature.id) raise(value);
        for (const auto& inner : profile->includes) self(inner);
    };
    for (const auto& name : profiles) visit(name);
    return level;
}

namespace {

bool contains_namespace(std::string_view container, std::string_view name) {
    return container == name || (container.starts_with(name) && container.substr(name.size()).starts_with("::"));
}

bool profile_known(const plugin::Catalog& catalog, std::string_view name) {
    if (catalog.profile(name) != nullptr) return true;
    for (const auto& e : catalog.features)
        for (const auto& [profile, level] : e.feature->profiles)
            if (profile == name) return true;
    return false;
}

// A `files` pattern against a path, both '/'-separated: `*` any characters but '/', `**` any number of
// whole path components (`src/**` is everything under src, `**/*.cppm` every interface unit), `?` one
// character but '/'.
bool glob_match(std::string_view pattern, std::string_view path) {
    if (pattern.empty()) return path.empty();
    if (pattern.starts_with("**")) {
        std::string_view rest { pattern.substr(2) };
        if (rest.starts_with('/')) rest.remove_prefix(1);
        else if (!rest.empty()) return false;   // `**` stands alone as a component
        if (rest.empty()) return true;
        for (std::size_t at { 0 };;) {
            if (glob_match(rest, path.substr(at))) return true;
            const std::size_t slash { path.find('/', at) };
            if (slash == std::string_view::npos) return false;
            at = slash + 1;
        }
    }
    if (pattern.front() == '*') {
        for (std::size_t n { 0 }; n <= path.size(); ++n) {
            if (glob_match(pattern.substr(1), path.substr(n))) return true;
            if (n < path.size() && path[n] == '/') break;
        }
        return false;
    }
    if (path.empty()) return false;
    if (pattern.front() == '?') return path.front() != '/' && glob_match(pattern.substr(1), path.substr(1));
    return pattern.front() == path.front() && glob_match(pattern.substr(1), path.substr(1));
}

// How specific a pattern is: its characters that are not wildcards.
std::size_t specificity(std::string_view pattern) { return static_cast<std::size_t>(std::ranges::count_if(pattern, [](char c) { return c != '*' && c != '?'; })); }

void check_ids(const plugin::Catalog& catalog, const LevelMap& levels, std::string_view where, std::vector<std::string>& problems) {
    for (const auto& [id, level] : levels)
        if (catalog.find(id) == nullptr) problems.push_back(std::format("{}: `{}` is not a feature any linked provider declares", where, id));
}

} // namespace

const Plan::Gate* Plan::gate(std::string_view id) const {
    const plugin::Catalog::Entry* entry { catalog->find(id) };
    return entry == nullptr ? nullptr : &gates[static_cast<std::size_t>(entry - catalog->features.data())];
}

std::string Plan::relative(std::string_view path) const {
    if (config.manifest.empty()) return {};
    const auto rel { base::relative_path(base::normalize_path(path), base::parent_path(config.manifest)) };
    return rel ? *rel : std::string {};
}

Level Plan::level(const Gate& gate, std::string_view module, std::string_view container, std::string_view file) const {
    return explain(gate, module, container, file).first;
}

std::pair<Level, std::string> Plan::explain(const Gate& gate, std::string_view module, std::string_view container, std::string_view file) const {
    const std::string_view id { gate.entry->feature->id };
    const std::string in { config.manifest.empty() ? std::string {} : std::format(" ({})", config.manifest) };
    Level level { gate.base };
    std::string from { "the feature's default" };
    if (const auto it = config.package.find(id); it != config.package.end()) from = "the package's [package.metadata.mcxx.features]" + in;
    else if (level != gate.entry->feature->default_level || !config.profiles.empty()) {
        for (const auto& p : config.profiles) {
            const std::string one[] { p };
            if (const auto l = profile_level(*catalog, *gate.entry->feature, one); l && *l == level) {
                from = std::format("profile `{}`{}", p, in);
                break;
            }
        }
    }
    if (!module.empty()) {
        const std::string_view primary { module.substr(0, module.find(':')) };
        for (const auto name : { primary, module }) {
            if (const auto m = config.modules.find(name); m != config.modules.end())
                if (const auto it = m->second.find(id); it != m->second.end()) {
                    level = it->second;
                    from = std::format("module `{}`{}", name, in);
                }
            if (name == module) break;
        }
    }
    // The most specific pattern that matches the file and sets this feature (MC1 0.4.0).
    if (!file.empty()) {
        std::optional<std::size_t> most;
        for (const auto& [pattern, levels] : config.files) {
            const auto it = levels.find(id);
            if (it == levels.end() || !glob_match(pattern, file)) continue;
            if (const std::size_t s { specificity(pattern) }; !most || s >= *most) {
                level = it->second;
                from = std::format("files `{}`{}", pattern, in);
                most = s;
            }
        }
    }
    // The innermost configured namespace that contains the code and sets this feature.
    std::size_t best { 0 };
    for (const auto& [name, levels] : config.namespaces) {
        if (!contains_namespace(container, name) || name.size() < best) continue;
        if (const auto it = levels.find(id); it != levels.end()) {
            level = it->second;
            from = std::format("namespace `{}`{}", name, in);
            best = name.size();
        }
    }
    return { level, from };
}

Plan make_plan(Config config) {
    Plan plan;
    plan.catalog = plugin::catalog();
    plan.config = std::move(config);
    const auto& catalog = *plan.catalog;
    const Config& c = plan.config;
    plan.problems = c.problems;
    for (const auto& p : catalog.problems) plan.problems.push_back(p);
    const std::string where { c.manifest.empty() ? std::string { "[package.metadata.mcxx]" } : std::format("{} [package.metadata.mcxx]", c.manifest) };
    for (const auto& name : c.profiles)
        if (!profile_known(catalog, name)) plan.problems.push_back(std::format("{}: profile `{}` is not one any linked provider defines", where, name));
    check_ids(catalog, c.package, where + ".features", plan.problems);
    for (const auto& [name, levels] : c.modules) check_ids(catalog, levels, std::format("{}.modules.\"{}\"", where, name), plan.problems);
    for (const auto& [name, levels] : c.namespaces) check_ids(catalog, levels, std::format("{}.namespaces.\"{}\"", where, name), plan.problems);
    for (const auto& [pattern, levels] : c.files) check_ids(catalog, levels, std::format("{}.files.\"{}\"", where, pattern), plan.problems);
    for (const auto& [name, allowance] : c.imports)
        for (const auto& id : allowance.ids)
            if (catalog.find(id) == nullptr)
                plan.problems.push_back(std::format("{}.imports.\"{}\": `{}` is not a feature any linked provider declares", where, name, id));

    plan.wanted.resize(catalog.rules.size());
    plan.gates.reserve(catalog.features.size());
    for (const auto& entry : catalog.features) {
        const plugin::Feature& f { *entry.feature };
        Plan::Gate gate { &entry, f.default_level, false };
        if (const auto by_profile = profile_level(catalog, f, c.profiles)) gate.base = *by_profile;
        if (const auto it = c.package.find(f.id); it != c.package.end()) gate.base = it->second;
        gate.maybe = gate.base != Level::allow;
        // A region (an attribute naming a profile, MC4 §2) may raise the feature where it is written.
        for (const auto& a : catalog.attributes) {
            if (a.attribute->region.empty()) continue;
            const std::string region[] { a.attribute->region };
            if (const auto l = profile_level(catalog, f, region); l && *l != Level::allow) gate.maybe = true;
        }
        for (const auto* scopes : { &c.modules, &c.namespaces, &c.files })
            for (const auto& [name, levels] : *scopes)
                if (const auto it = levels.find(f.id); it != levels.end() && it->second != Level::allow) gate.maybe = true;
        plan.gates.push_back(gate);
        if (!gate.maybe) continue;
        plan.gated = true;
        plan.needs |= f.needs;
        if (const auto rule = std::ranges::find(catalog.rules, entry.provider); rule != catalog.rules.end())
            plan.wanted[static_cast<std::size_t>(rule - catalog.rules.begin())].push_back(f.id);
    }
    // Waivers are read whenever anything is gated, and regions where a provider declares one.
    if (plan.gated) plan.needs |= msa::fact::Kinds::suppressions;
    if (plan.gated && std::ranges::any_of(catalog.attributes, [](const auto& a) { return !a.attribute->region.empty(); }))
        plan.needs |= msa::fact::Kinds::attributes;
    return plan;
}

std::shared_ptr<const Plan> plan_for(std::string_view source_path) {
    struct Cached {
        std::filesystem::file_time_type stamp;
        std::uint64_t generation;
        std::shared_ptr<const Plan> plan;
    };
    static std::mutex mutex;
    static std::map<std::string, Cached, std::less<>> cache;
    static const std::shared_ptr<const Plan> empty_plan { std::make_shared<const Plan>(make_plan({})) };
    const auto manifest = find_manifest(source_path);
    const std::uint64_t generation { plugin::catalog()->generation };
    if (!manifest) {
        if (empty_plan->catalog->generation == generation) return empty_plan;
        return std::make_shared<const Plan>(make_plan({}));
    }
    std::error_code ec;
    const auto stamp = std::filesystem::last_write_time(*manifest, ec);
    std::lock_guard lock { mutex };
    if (const auto it = cache.find(*manifest); it != cache.end() && it->second.stamp == stamp && it->second.generation == generation) return it->second.plan;
    auto plan = std::make_shared<const Plan>(make_plan(read_config(*manifest)));
    cache.insert_or_assign(*manifest, Cached { stamp, generation, plan });
    return plan;
}

Selection select(const Plan& plan, const std::function<bool(std::string_view)>& declared) {
    Selection s { plan.wanted, msa::fact::Kinds::none, false };
    std::map<std::string_view, bool, std::less<>> known;
    for (auto& ids : s.wanted) {
        std::erase_if(ids, [&](const std::string& id) {
            const plugin::Feature& f { *plan.gate(id)->entry->feature };
            if (f.requires_declaration.empty()) return false;
            auto [it, fresh] = known.try_emplace(f.requires_declaration, false);
            if (fresh) it->second = declared(f.requires_declaration);
            return !it->second;
        });
        for (const auto& id : ids) s.needs |= plan.gate(id)->entry->feature->needs;
    }
    // A feature not asked of a rule (a source filter's) still counts as gated.
    for (const auto& g : plan.gates)
        if (g.maybe && std::ranges::find(plan.catalog->rules, g.entry->provider) == plan.catalog->rules.end()) s.gated = true;
    for (const auto& ids : s.wanted) s.gated = s.gated || !ids.empty();
    if (s.gated) s.needs |= msa::fact::Kinds::suppressions;
    if (s.gated && msa::fact::contains(plan.needs, msa::fact::Kinds::attributes)) s.needs |= msa::fact::Kinds::attributes;
    return s;
}

Result evaluate(const plugin::Context& context, const Plan& plan, std::span<const plugin::Finding> prior) {
    return evaluate(context, plan, Selection { plan.wanted, plan.needs, plan.gated }, prior);
}

Result evaluate(const plugin::Context& context, const Plan& plan, const Selection& selection, std::span<const plugin::Finding> prior) {
    Result result;
    for (const auto& problem : plan.problems)
        result.diagnostics.push_back({ {}, msa::Severity::warning, problem, "mcxx-config", "MC++ feature gate", {} });
    if (!selection.gated && prior.empty()) return result;
    const auto& catalog = *plan.catalog;
    std::vector<plugin::Finding> findings { prior.begin(), prior.end() };
    std::vector<plugin::Finding> one;
    std::vector<plugin::Failure> failures;
    for (std::size_t i { 0 }; i < catalog.rules.size(); ++i) {
        const auto& wanted = selection.wanted[i];
        if (wanted.empty()) continue;
        plugin::Context scoped { context.path, context.module, context.facts, &wanted, &failures };
        one.clear();
        catalog.rules[i]->check(scoped, one);
        // A rule speaks for the features it was asked for, not for another provider's (MC4-2-2).
        for (auto& f : one)
            if (std::ranges::find(wanted, f.feature) != wanted.end()) findings.push_back(std::move(f));
    }
    // A provider that could not check (MC4 §5): an error when a feature it was asked for is denied
    // here -- a gate that could not be checked does not pass -- and a warning otherwise.
    for (const auto& f : failures) {
        bool denied { false };
        for (const auto& id : f.features) {
            const Plan::Gate* g { plan.gate(id) };
            if (g == nullptr) continue;
            denied = denied || plan.level(*g, context.module, "", plan.relative(context.path)) == Level::deny;
            for (const auto& [name, levels] : plan.config.namespaces)
                if (const auto it = levels.find(id); it != levels.end() && it->second == Level::deny) denied = true;
        }
        std::string ids;
        for (const auto& id : f.features) ids += std::format("{}{}", ids.empty() ? "" : ", ", id);
        result.diagnostics.push_back({ {}, denied ? msa::Severity::error : msa::Severity::warning,
                                       std::format("plugin {} failed on this file: {}; not checked here: {}", f.provider, f.reason, ids.empty() ? "-" : ids),
                                       "mcxx-plugin", "MC++ plugin", {} });
    }
    for (const auto& s : context.facts.suppressions)
        for (const auto& id : s.ids)
            if (catalog.find(id) == nullptr) result.unknown.emplace_back(id, s.range);
    const std::string file { plan.relative(context.path) };
    for (const auto& finding : findings) {
        const Plan::Gate* gate { plan.gate(finding.feature) };
        if (gate == nullptr || !gate->maybe) continue;
        const plugin::Feature& feature { *gate->entry->feature };
        auto [level, level_from] = plan.explain(*gate, context.module, finding.container, file);
        // Inside a region, its profile's level where that is stricter (MC1 §6, MC4 §2).
        for (const auto& a : context.facts.attributes) {
            const auto* entry = catalog.attribute(a.name);
            if (entry == nullptr || entry->attribute->region.empty() || !within(a.range, finding.range)) continue;
            const std::string region[] { entry->attribute->region };
            if (const auto l = profile_level(catalog, feature, region); l && *l > level) {
                level = *l;
                level_from = std::format("the region of [[{}]] (profile `{}`)", a.name, entry->attribute->region);
            }
        }
        if (level == Level::allow) continue;
        // The innermost declaration waiving it.
        const msa::fact::Suppression* waiver { nullptr };
        for (const auto& s : context.facts.suppressions) {
            if (!within(s.range, finding.range) || std::ranges::find(s.ids, feature.id) == s.ids.end()) continue;
            if (waiver == nullptr || within(waiver->range, s.range)) waiver = &s;
        }
        if (waiver != nullptr && feature.waivable) {
            result.waived.push_back({ feature.id, std::string { context.path }, finding.range,
                                      waiver->declaration.empty() ? waiver->entity : waiver->declaration, waiver->reason });
            continue;
        }
        std::string message { std::format("{} [{}]", finding.message, feature.id) };
        // At an import (M1.2): what crosses is waived on the import, `import m [[mcpp::allow("id")]];`.
        const auto import = std::ranges::find(context.facts.imports, finding.range, &msa::fact::Import::range);
        const std::string fix { import == context.facts.imports.end() ? feature.fix : std::string {} };
        const std::string how { waiver != nullptr                      ? std::string {}
                                : import != context.facts.imports.end() ? std::format("import {} [[mcpp::allow(\"{}\", \"<why>\")]];", import->module, feature.id)
                                                                        : std::format("[[mcpp::allow(\"{}\", \"<why>\")]]", feature.id) };
        if (!fix.empty()) message += std::format("; {}", fix);
        message += waiver != nullptr                      ? std::format("; {} cannot be waived", feature.id)
                   : import != context.facts.imports.end() ? std::format("; to allow it here, `import {} [[mcpp::allow(\"{}\")]];`", import->module, feature.id)
                                                           : std::format("; to allow it here, [[mcpp::allow(\"{}\")]] on the declaration", feature.id);
        msa::Diagnostic d { finding.range, level == Level::deny ? msa::Severity::error : msa::Severity::warning, std::move(message), feature.id, "MC++ feature gate", {} };
        d.headline = finding.message;
        d.fix = fix;
        d.waiver = how;
        d.level = std::string { plugin::to_string(level) };
        d.level_from = std::move(level_from);
        result.diagnostics.push_back(std::move(d));
    }
    return result;
}

Result evaluate(const plugin::Context& context, const Config& config) { return evaluate(context, make_plan(config)); }

std::vector<std::string> profile_members(const plugin::Catalog& catalog, std::string_view profile) {
    std::vector<std::string> out;
    const std::string names[] { std::string { profile } };
    for (const auto& e : catalog.features)
        if (const auto level = profile_level(catalog, *e.feature, names); level && *level != Level::allow) out.push_back(e.feature->id);
    return out;
}

std::string catalog_json(const plugin::Catalog& catalog) {
    auto origin = [&](const plugin::Provider* p) { return catalog.origin_of(p) == plugin::Origin::builtin ? "built-in" : "plugin"; };
    auto kind = [&](const plugin::Provider* p) { return std::ranges::find(catalog.filters, p) != catalog.filters.end() ? "source filter" : "rule"; };
        auto strings = [](const auto& items) {
            std::string out;
            for (const auto& item : items) out += (out.empty() ? "" : ",") + json_string(item);
            return "[" + out + "]";
        };
        std::string out { "{\"mc1-version\":\"0.4.0\",\"providers\":[" };
        for (std::size_t i { 0 }; i < catalog.providers.size(); ++i) {
            const auto* p = catalog.providers[i].provider;
            out += std::format("{}{{\"name\":{},\"origin\":\"{}\",\"kind\":\"{}\"}}", i ? "," : "", json_string(p->name()), origin(p), kind(p));
        }
        out += "],\"replaced\":" + strings(catalog.replaced) + ",\"features\":[";
        for (std::size_t i { 0 }; i < catalog.features.size(); ++i) {
            const auto& e = catalog.features[i];
            const auto& f = *e.feature;
            std::string profiles;
            for (const auto& [name, level] : f.profiles)
                profiles += std::format("{}{{\"profile\":{},\"level\":\"{}\"}}", profiles.empty() ? "" : ",", json_string(name), plugin::to_string(level));
            out += std::format("{}{{\"id\":{},\"category\":\"{}\",\"standard\":{},\"layer\":{},\"provider\":{},\"default\":\"{}\",\"waivable\":{},"
                               "\"summary\":{},\"fix\":{},\"profiles\":[{}],\"needs\":{},\"requires-declaration\":{},\"replaces\":{},\"shadowed\":{}}}",
                               i ? "," : "", json_string(f.id), plugin::to_string(f.category), json_string(f.standard), json_string(f.layer),
                               json_string(e.provider->name()), plugin::to_string(f.default_level), f.waivable, json_string(f.summary), json_string(f.fix),
                               profiles, strings(msa::fact::names(f.needs)), json_string(f.requires_declaration), f.replaces, strings(e.shadowed));
        }
        out += "],\"profiles\":[";
        for (std::size_t i { 0 }; i < catalog.profiles.size(); ++i) {
            const auto& p = *catalog.profiles[i].profile;
            out += std::format("{}{{\"name\":{},\"provider\":{},\"summary\":{},\"includes\":{},\"features\":{}}}", i ? "," : "", json_string(p.name),
                               json_string(catalog.profiles[i].provider->name()), json_string(p.summary), strings(p.includes),
                               strings(profile_members(catalog, p.name)));
        }
    out += "],\"attributes\":[";
    for (std::size_t i { 0 }; i < catalog.attributes.size(); ++i) {
        const auto& a = *catalog.attributes[i].attribute;
        out += std::format("{}{{\"name\":{},\"provider\":{},\"summary\":{},\"region\":{}}}", i ? "," : "", json_string(a.name),
                           json_string(catalog.attributes[i].provider->name()), json_string(a.summary), json_string(a.region));
    }
    return std::format("{}],\"problems\":{}}}", out, strings(catalog.problems));
}

void append_audit(std::string_view file, const std::vector<Waiver>& waivers) {
    if (file.empty() || waivers.empty()) return;
    static std::mutex mutex;
    std::lock_guard lock { mutex };
    std::ofstream out { std::filesystem::path { file }, std::ios::app };
    for (const auto& w : waivers) {
        out << std::format("{{\"feature\":{},\"path\":{},\"line\":{},\"column\":{},\"declaration\":{},\"reason\":{}}}\n", json_string(w.feature),
                           json_string(w.path), w.range.begin.line + 1, w.range.begin.column + 1, json_string(w.entity), json_string(w.reason));
    }
}

} // namespace mcxx::features
