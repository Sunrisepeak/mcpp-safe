// mcxx.plugin.wire:api -- the wire formats' types and functions (mcxx.plugin.wire's interface).
export module mcxx.plugin.wire:api;

import std;
import nlohmann.json;
import mcxx.msa;
import mcxx.plugin;

export namespace mcxx::plugin::wire {

using Json = nlohmann::json;

inline constexpr int PROTOCOL { 1 };
inline constexpr std::string_view MC3_VERSION { "0.5.0" };   // 0.5.0 adds control-flow (MC3 0.9.0), 0.4.0 imports, 0.3.0 a declaration's `local`, 0.2.0 attributes; older documents are read too

template <class T>
using Read = std::expected<T, std::string>;

Json to_json(const msa::Range& range);
Read<msa::Range> range_from(const Json& j);

// A facts document: the facts of `path` (in `module`).
Json facts_to_json(const msa::fact::Facts& facts, std::string_view path, std::string_view module);
Read<msa::fact::Facts> facts_from_json(const Json& j);

Json to_json(const Finding& finding);
Read<Finding> finding_from(const Json& j);
Json to_json(const Feature& feature);
Read<Feature> feature_from(const Json& j);
Json to_json(const Profile& profile);
Read<Profile> profile_from(const Json& j);
Json to_json(const Target& target);
Read<Target> target_from(const Json& j);

// A provider as a plugin describes itself in `welcome`: its extension points, features, profiles,
// and whom it replaces.
struct ProviderInfo {
    std::string name;
    std::vector<std::string> extension_points;   // "rule", "source-filter"
    std::vector<Feature> features;
    std::vector<Profile> profiles;
    std::vector<std::string> replaces;
    std::vector<AttributeSpec> attributes;
};
Json to_json(const ProviderInfo& provider);
Read<ProviderInfo> provider_from(const Json& j);
ProviderInfo describe(const Provider& provider, bool rule, bool filter);

// One protocol message as one line (no line break inside), and back.
std::string line(const Json& message);
Read<Json> message_from(std::string_view line);

} // namespace mcxx::plugin::wire
