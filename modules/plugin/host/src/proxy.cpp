// Proxies in the catalog for what out-of-process plugins provide, and loading them per package.
module mcxx.plugin.host;

import std;
import nlohmann.json;
import mcxx.base;
import mcxx.os;
import mcxx.msa;
import mcxx.plugin;
import mcxx.plugin.wire;
import mcxx.features;

namespace mcxx::plugin::host {

using wire::Json;
namespace fs = std::filesystem;

namespace {

// Where a file's package is: a proxy answers only for its own package's files.
std::string manifest_of(std::string_view path) {
    static std::mutex mutex;
    static std::map<std::string, std::string, std::less<>> cache;
    std::lock_guard lock { mutex };
    if (const auto it = cache.find(path); it != cache.end()) return it->second;
    std::string m { features::find_manifest(path).value_or("") };
    cache.emplace(std::string { path }, m);
    return m;
}

class Described {
public:
    Described(wire::ProviderInfo info, std::shared_ptr<Session> session, std::string manifest)
        : info_ { std::move(info) }, session_ { std::move(session) }, manifest_ { std::move(manifest) } {
        for (const auto& r : info_.replaces) replaces_.emplace_back(r);
    }

protected:
    wire::ProviderInfo info_;
    std::shared_ptr<Session> session_;
    std::string manifest_;
    std::vector<std::string_view> replaces_;

    bool mine(std::string_view path) const { return manifest_of(path) == manifest_; }
};

class RemoteRule final : public Rule, Described {
public:
    using Described::Described;
    std::string_view name() const override { return info_.name; }
    std::span<const Feature> features() const override { return info_.features; }
    std::span<const Profile> profiles() const override { return info_.profiles; }
    std::span<const std::string_view> replaces() const override { return replaces_; }
    std::span<const AttributeSpec> attributes() const override { return info_.attributes; }

    void check(const Context& context, std::vector<Finding>& out) const override {
        if (!mine(context.path)) return;
        base::trace::Span span { "plugins", "check", info_.name };
        Json m = Json::object();
        m["type"] = "check";
        m["provider"] = info_.name;
        m["path"] = std::string { context.path };
        m["module"] = std::string { context.module };
        Json wanted = Json::array();
        if (context.wanted != nullptr)
            for (const auto& id : *context.wanted) wanted.push_back(id);
        else
            for (const auto& f : info_.features) wanted.push_back(f.id);
        m["wanted"] = std::move(wanted);
        m["facts"] = wire::facts_to_json(context.facts, context.path, context.module);
        auto answer = session_->request(std::move(m));
        if (!answer) return context.fail(info_.name, answer.error());
        if ((*answer)["type"] != "findings" || !answer->contains("findings") || !(*answer)["findings"].is_array())
            return context.fail(info_.name, "its answer to check is not findings (MC4-6.3-1)");
        for (const auto& f : (*answer)["findings"]) {
            auto finding = wire::finding_from(f);
            if (!finding) return context.fail(info_.name, std::format("it wrote a finding wrongly: {}", finding.error()));
            out.push_back(std::move(*finding));
        }
    }
};

class RemoteFilter final : public SourceFilter, Described {
public:
    using Described::Described;
    std::string_view name() const override { return info_.name; }
    std::span<const Feature> features() const override { return info_.features; }
    std::span<const Profile> profiles() const override { return info_.profiles; }
    std::span<const std::string_view> replaces() const override { return replaces_; }

    Filtered filter(const SourceContext& context, std::string_view text) const override {
        Filtered result;
        if (!mine(context.path)) return result;
        Json m = Json::object();
        m["type"] = "filter";
        m["provider"] = info_.name;
        m["path"] = std::string { context.path };
        m["target"] = wire::to_json(context.target);
        m["text"] = std::string { text };
        auto failed = [&](std::string why) {
            // A filter that could not run leaves text the build did not mean to compile: an error.
            result.problems.push_back({ {}, msa::Severity::error, std::format("plugin {} failed on this file: {}", info_.name, why), "mcxx-plugin",
                                        "MC++ plugin", {} });
            return result;
        };
        auto answer = session_->request(std::move(m));
        if (!answer) return failed(answer.error());
        if ((*answer)["type"] != "filtered" || !answer->contains("text")) return failed("its answer to filter is not filtered (MC4-6.3-1)");
        if ((*answer)["text"].is_string()) result.text = (*answer)["text"].get<std::string>();
        if (answer->contains("problems") && (*answer)["problems"].is_array())
            for (const auto& p : (*answer)["problems"]) {
                const auto range = wire::range_from(p.value("range", Json::object()));
                result.problems.push_back({ range.value_or(msa::Range {}), msa::Severity::error, p.value("message", std::string {}), "mcxx-filter",
                                            std::format("MC++ plugin {}", info_.name), {} });
            }
        if (answer->contains("findings") && (*answer)["findings"].is_array())
            for (const auto& f : (*answer)["findings"])
                if (auto finding = wire::finding_from(f)) result.findings.push_back(std::move(*finding));
        return result;
    }
};

struct Loaded {
    std::shared_ptr<Session> session;
    std::string problem;   // why it could not be started ("" = running)
};

struct Registry {
    std::mutex mutex;
    std::map<std::string, Loaded, std::less<>> plugins;   // by manifest and name
    std::vector<std::string> composed;
    ~Registry() {
        for (auto& [key, loaded] : plugins)
            if (loaded.session) loaded.session->shutdown();
    }
};

Registry& registry() {
    static Registry r;
    return r;
}

} // namespace

std::vector<std::string> load(const features::Config& config) {
    std::vector<std::string> problems;
    auto& r = registry();
    std::lock_guard lock { r.mutex };
    const fs::path dir { config.manifest.empty() ? fs::path {} : fs::path { config.manifest }.parent_path() };
    for (const auto& entry : config.plugins) {
        if (entry.is_static()) continue;
        const std::string key { config.manifest + '\n' + entry.name };
        if (const auto it = r.plugins.find(key); it != r.plugins.end()) {
            if (!it->second.problem.empty()) problems.push_back(it->second.problem);
            continue;
        }
        std::vector<std::string> command { entry.command };
        // It runs in its package's directory, and a relative program is relative to it (MC4-3-5).
        if (fs::path program { command[0] }; program.is_relative()) command[0] = (dir / program).lexically_normal().generic_string();
        auto session = std::make_shared<Session>(entry.name, command, dir.generic_string(), entry.timeout);
        auto providers = session->start();
        Loaded loaded { session, {} };
        if (!providers) {
            loaded.problem = std::format("plugin {} ({}) could not be started: {}; nothing it gates was checked", entry.name, command[0], providers.error());
            problems.push_back(loaded.problem);
        } else {
            for (auto& info : *providers) {
                const bool rule { std::ranges::find(info.extension_points, "rule") != info.extension_points.end() };
                const bool filter { std::ranges::find(info.extension_points, "source-filter") != info.extension_points.end() };
                if (rule) register_rule(std::make_unique<RemoteRule>(info, session, config.manifest));
                if (filter) register_source_filter(std::make_unique<RemoteFilter>(info, session, config.manifest));
            }
            base::trace::count("plugins.started");
        }
        r.plugins.emplace(key, std::move(loaded));
    }
    return problems;
}

void set_composed(std::vector<std::string> packages) {
    auto& r = registry();
    std::lock_guard lock { r.mutex };
    r.composed = std::move(packages);
}

const std::vector<std::string>& composed() { return registry().composed; }

} // namespace mcxx::plugin::host
