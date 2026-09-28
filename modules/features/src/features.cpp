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

std::optional<std::string> find_manifest(std::string_view source_path) {
    std::error_code ec;
    std::filesystem::path dir { std::filesystem::absolute(std::filesystem::path { source_path }, ec).parent_path() };
    while (!dir.empty()) {
        const auto candidate = dir / "mcpp.toml";
        if (std::filesystem::is_regular_file(candidate, ec)) {
            if (auto doc = toml::parse_file(candidate); doc && doc->get_table("package") != nullptr) return candidate.generic_string();
        }
        if (dir == dir.parent_path()) break;
        dir = dir.parent_path();
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
        if (it->second.is_string()) config.profile = it->second.as_string();
        else config.problems.push_back(where + ": profile is not a string");
    }
    if (const auto it = mcxx->find("features"); it != mcxx->end() && it->second.is_table())
        read_levels(it->second.as_table(), config.package, where + ".features", config.problems);
    for (const auto& [key, into] : { std::pair { std::string_view { "modules" }, &config.modules },
                                     std::pair { std::string_view { "namespaces" }, &config.namespaces } }) {
        const auto it = mcxx->find(key);
        if (it == mcxx->end() || !it->second.is_table()) continue;
        for (const auto& [name, table] : it->second.as_table()) {
            if (!table.is_table()) continue;
            read_levels(table.as_table(), (*into)[name], std::format("{}.{}.\"{}\"", where, key, name), config.problems);
        }
    }
    return config;
}

Config read_config(std::string_view manifest_path) {
    std::ifstream in { std::filesystem::path { manifest_path } };
    const std::string text { std::istreambuf_iterator<char> { in }, {} };
    return parse_config(text, std::string { manifest_path });
}

Config config_for(std::string_view source_path) {
    static std::mutex mutex;
    static std::map<std::string, std::pair<std::filesystem::file_time_type, Config>, std::less<>> cache;
    const auto manifest = find_manifest(source_path);
    if (!manifest) return {};
    std::error_code ec;
    const auto stamp = std::filesystem::last_write_time(*manifest, ec);
    std::lock_guard lock { mutex };
    if (const auto it = cache.find(*manifest); it != cache.end() && it->second.first == stamp) return it->second.second;
    Config config { read_config(*manifest) };
    cache.insert_or_assign(*manifest, std::pair { stamp, config });
    return config;
}

Level level_of(const plugin::Feature& feature, const Config& config, std::string_view module, std::string_view container) {
    Level level { feature.default_level };
    for (const auto& [profile, value] : feature.profiles)
        if (profile == config.profile) level = value;
    if (const auto it = config.package.find(feature.id); it != config.package.end()) level = it->second;
    if (!module.empty()) {
        const std::string_view primary { module.substr(0, module.find(':')) };
        for (const auto name : { primary, module }) {
            if (const auto m = config.modules.find(name); m != config.modules.end())
                if (const auto it = m->second.find(feature.id); it != m->second.end()) level = it->second;
        }
    }
    // The innermost configured namespace that contains the code.
    std::size_t best { 0 };
    for (const auto& [name, levels] : config.namespaces) {
        const bool contains { container == name || (container.starts_with(name) && container.substr(name.size()).starts_with("::")) };
        if (!contains || name.size() < best) continue;
        if (const auto it = levels.find(feature.id); it != levels.end()) {
            level = it->second;
            best = name.size();
        }
    }
    return level;
}

Result evaluate(const plugin::Context& context, const Config& config) {
    Result result;
    std::vector<plugin::Finding> findings;
    for (const auto& rule : plugin::rules()) rule->check(context, findings);
    for (const auto& problem : config.problems)
        result.diagnostics.push_back({ {}, msa::Severity::warning, problem, "mcxx-config", "MC++ feature gate", {} });
    for (const auto& s : context.facts.suppressions)
        for (const auto& id : s.ids)
            if (plugin::find_feature(id) == nullptr) result.unknown.emplace_back(id, s.range);
    for (const auto& finding : findings) {
        const plugin::Feature* feature { plugin::find_feature(finding.feature) };
        if (feature == nullptr) continue;
        const Level level { level_of(*feature, config, context.module, finding.container) };
        if (level == Level::allow) continue;
        // The innermost declaration waiving it.
        const msa::fact::Suppression* waiver { nullptr };
        for (const auto& s : context.facts.suppressions) {
            if (!within(s.range, finding.range) || std::ranges::find(s.ids, feature->id) == s.ids.end()) continue;
            if (waiver == nullptr || within(waiver->range, s.range)) waiver = &s;
        }
        if (waiver != nullptr && feature->waivable) {
            result.waived.push_back({ feature->id, std::string { context.path }, finding.range,
                                      waiver->declaration.empty() ? waiver->entity : waiver->declaration, waiver->reason });
            continue;
        }
        std::string message { std::format("{} [{}]", finding.message, feature->id) };
        if (!feature->fix.empty()) message += std::format("; {}", feature->fix);
        message += waiver != nullptr ? std::format("; {} cannot be waived", feature->id)
                                     : std::format("; to allow it here, [[mcpp::allow(\"{}\")]] on the declaration", feature->id);
        result.diagnostics.push_back({ finding.range, level == Level::deny ? msa::Severity::error : msa::Severity::warning, std::move(message),
                                       feature->id, "MC++ feature gate", {} });
    }
    return result;
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
