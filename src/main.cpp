// mcxx: the MC++ compiler driver (MC5 v0).
//
//   mcxx c++ <compiler arguments>     a C++ compile, in process (so is mcxx named clang++, c++, g++)
//   mcxx cc <compiler arguments>      a C compile (so is mcxx named clang, cc, gcc)
//   mcxx check <compiler arguments>   the same command line, checked only (-fsyntax-only): MC++'s
//                                     feature gates and source filters, with the compiler's own diagnostics
//   mcxx features [--json]            what this program can gate: its providers (MC++'s built-in
//                                     mc++.iso and the plugins linked in), features, profiles, conflicts
//   mcxx version
//
// The compiler is libmc++'s compiling facade (mcxx.backend.compiler): today clang 23.1 in process,
// with the plugins linked into this program (plugins/std, plugins/libs) in every compilation. The
// driver itself names no compiler: all of Clang is behind modules/backend.
import std;
import mcxx.backend.compiler;
import mcxx.base;
import mcxx.msa;
import mcxx.plugin;
import mcxx.features;

namespace {

constexpr std::string_view VERSION { "0.1.0" };

std::string_view base_name(std::string_view path) {
    const auto slash = path.find_last_of("/\\");
    return slash == std::string_view::npos ? path : path.substr(slash + 1);
}

void usage() {
    std::print(std::cerr, "usage: mcxx c++|cc <compiler arguments>\n"
                          "       mcxx check <compiler arguments>      check a compile command (-fsyntax-only)\n"
                          "       mcxx features [--json]               the providers, features and profiles linked in\n"
                          "       mcxx version\n");
}

std::string json_string(std::string_view text) {
    std::string out { "\"" };
    for (const char c : text) {
        if (c == '"' || c == '\\') out += '\\';
        if (static_cast<unsigned char>(c) < 0x20) {
            out += std::format("\\u{:04x}", static_cast<unsigned char>(c));
            continue;
        }
        out += c;
    }
    return out + "\"";
}

// The features a profile does not leave at `allow`.
std::vector<std::string> members(const mcxx::plugin::Catalog& catalog, const std::string& profile) {
    std::vector<std::string> out;
    const std::string names[] { profile };
    for (const auto& e : catalog.features)
        if (const auto level = mcxx::features::profile_level(catalog, *e.feature, names); level && *level != mcxx::plugin::Level::allow)
            out.push_back(e.feature->id);
    return out;
}

int list_features(bool json) {
    namespace plugin = mcxx::plugin;
    const auto catalog = plugin::catalog();
    auto origin = [&](const plugin::Provider* p) { return catalog->origin_of(p) == plugin::Origin::builtin ? "built-in" : "plugin"; };
    auto kind = [&](const plugin::Provider* p) {
        return std::ranges::find(catalog->filters, p) != catalog->filters.end() ? "source filter" : "rule";
    };
    if (json) {
        // MC1 §10 (specs/schema/mc1-catalog.schema.json).
        auto strings = [](const auto& items) {
            std::string out;
            for (const auto& item : items) out += (out.empty() ? "" : ",") + json_string(item);
            return "[" + out + "]";
        };
        std::string out { "{\"mc1-version\":\"0.1.0\",\"providers\":[" };
        for (std::size_t i { 0 }; i < catalog->providers.size(); ++i) {
            const auto* p = catalog->providers[i].provider;
            out += std::format("{}{{\"name\":{},\"origin\":\"{}\",\"kind\":\"{}\"}}", i ? "," : "", json_string(p->name()), origin(p), kind(p));
        }
        out += "],\"replaced\":" + strings(catalog->replaced) + ",\"features\":[";
        for (std::size_t i { 0 }; i < catalog->features.size(); ++i) {
            const auto& e = catalog->features[i];
            const auto& f = *e.feature;
            std::string profiles;
            for (const auto& [name, level] : f.profiles)
                profiles += std::format("{}{{\"profile\":{},\"level\":\"{}\"}}", profiles.empty() ? "" : ",", json_string(name), plugin::to_string(level));
            out += std::format("{}{{\"id\":{},\"category\":\"{}\",\"standard\":{},\"layer\":{},\"provider\":{},\"default\":\"{}\",\"waivable\":{},"
                               "\"summary\":{},\"fix\":{},\"profiles\":[{}],\"needs\":{},\"requires-declaration\":{},\"replaces\":{},\"shadowed\":{}}}",
                               i ? "," : "", json_string(f.id), plugin::to_string(f.category), json_string(f.standard), json_string(f.layer),
                               json_string(e.provider->name()), plugin::to_string(f.default_level), f.waivable, json_string(f.summary), json_string(f.fix),
                               profiles, strings(mcxx::msa::fact::names(f.needs)), json_string(f.requires_declaration), f.replaces, strings(e.shadowed));
        }
        out += "],\"profiles\":[";
        for (std::size_t i { 0 }; i < catalog->profiles.size(); ++i) {
            const auto& p = *catalog->profiles[i].profile;
            out += std::format("{}{{\"name\":{},\"provider\":{},\"summary\":{},\"includes\":{},\"features\":{}}}", i ? "," : "", json_string(p.name),
                               json_string(catalog->profiles[i].provider->name()), json_string(p.summary), strings(p.includes),
                               strings(members(*catalog, p.name)));
        }
        std::println("{}],\"problems\":{}}}", out, strings(catalog->problems));
        return catalog->problems.empty() ? 0 : 1;
    }
    std::println("providers");
    for (const auto& [p, o] : catalog->providers) {
        const auto count = std::ranges::count_if(catalog->features, [&](const auto& e) { return e.provider == p; });
        std::println("  {:<20} {:<9} {:<14} {} feature{}", p->name(), origin(p), kind(p), count, count == 1 ? "" : "s");
    }
    for (const auto& name : catalog->replaced) std::println("  {:<20} replaced", name);
    std::println("\nfeatures");
    std::println("  {:<24} {:<10} {:<6} {:<18} {}", "id", "category", "level", "provider", "standard / summary");
    for (const auto& e : catalog->features) {
        const auto& f = *e.feature;
        std::println("  {:<24} {:<10} {:<6} {:<18} {}{}", f.id, plugin::to_string(f.category), plugin::to_string(f.default_level), e.provider->name(),
                     f.standard.empty() ? "" : f.standard + "  ", f.summary);
        for (const auto& s : e.shadowed) std::println("  {:<24} (replaces {}'s)", "", s);
    }
    std::println("\nprofiles");
    for (const auto& [p, provider] : catalog->profiles) {
        std::string ids;
        for (const auto& id : members(*catalog, p->name)) ids += (ids.empty() ? "" : ", ") + id;
        std::println("  {:<10} {}\n  {:<10} {}", p->name, p->summary, "", ids.empty() ? std::string { "(no feature)" } : ids);
    }
    if (!catalog->problems.empty()) {
        std::println("\nproblems");
        for (const auto& p : catalog->problems) std::println("  {}", p);
    }
    return catalog->problems.empty() ? 0 : 1;
}

} // namespace

int main(int argc, char** argv) {
    namespace compiler = mcxx::backend::compiler;
    // MCXX_LOG and MCXX_TRACE apply to a compile too: the gates' spans (gates.facts, gates.rules).
    mcxx::base::trace::configure_from_environment();
    struct Flush {
        ~Flush() { mcxx::base::trace::flush(); }
    } flush;
    // Named as a compiler, or re-invoked by the compiler itself (-cc1, -cc1as): the compiler, whole.
    if (!compiler::mode_for_name(base_name(argv[0])).empty() || (argc > 1 && std::string_view { argv[1] }.starts_with("-cc1")))
        return compiler::run(argc, argv);
    if (argc < 2) {
        usage();
        return 2;
    }
    const std::string_view command { argv[1] };
    std::vector<std::string> rest(argv + 2, argv + argc);
    if (command == "c++" || command == "cc") return compiler::run_as(command == "c++" ? "c++" : "c", argv[0], std::move(rest));
    if (command == "check") {
        rest.push_back("-fsyntax-only");
        return compiler::run_as("c++", argv[0], std::move(rest));
    }
    if (command == "features") return list_features(std::ranges::find(rest, "--json") != rest.end());
    if (command == "version" || command == "--version") {
        if (std::ranges::find(rest, "--json") != rest.end()) {
            // MC5 §5 (specs/schema/mc5-version.schema.json).
            std::string providers;
            for (const auto& p : mcxx::plugin::catalog()->providers) providers += (providers.empty() ? "" : ",") + json_string(p.provider->name());
            const std::string full { compiler::version() };   // "clang 23.1.0"
            std::string_view clang { full };
            if (const auto space = clang.rfind(' '); space != std::string_view::npos) clang.remove_prefix(space + 1);
            std::println("{{\"mcxx\":\"{}\",\"compiler\":{{\"name\":\"clang\",\"version\":{}}},\"specifications\":{{\"mc1\":\"0.1.0\","
                         "\"mc3\":\"0.1.0\",\"mc4\":\"0.1.0\",\"mc4-protocols\":[1],\"mc5\":\"0.1.0\"}},\"providers\":[{}]}}",
                         VERSION, json_string(clang), providers);
            return 0;
        }
        std::println("mcxx {} ({})", VERSION, compiler::version());
        return 0;
    }
    usage();
    return 2;
}
