// mcxx.driver: the commands.
module;

#include <cerrno>
#include <cstring>
#include <stdlib.h>
#include <unistd.h>

module mcxx.driver;

import std;
import mcxx.backend.compiler;
import mcxx.base;
import mcxx.msa;
import mcxx.plugin;
import mcxx.plugin.host;
import mcxx.features;
import mcxx.serve;
import mcxx.backend;

namespace mcxx::driver {

namespace {

namespace fs = std::filesystem;

std::string_view base_name(std::string_view path) {
    const auto slash = path.find_last_of("/\\");
    return slash == std::string_view::npos ? path : path.substr(slash + 1);
}

void usage() {
    std::print(std::cerr, "usage: mcxx c++|cc <compiler arguments>\n"
                          "       mcxx check <compiler arguments>      check a compile command (-fsyntax-only)\n"
                          "       mcxx features [--json]               the providers, features and profiles linked in\n"
                          "       mcxx compose [--manifest FILE]       build this package's compiler, with its static plugins\n"
                          "       mcxx serve [--db DIR] [--resource DIR] [--cache DIR]   the semantic service on standard input and output (MC6)\n"
                          "       mcxx version [--json]\n");
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
        std::println("{}", mcxx::features::catalog_json(*catalog));
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


// ---- composition (MC4 §3) ----------------------------------------------------------------------

// Where MC++'s sources are, for a composed program to build from: $MCXX_SOURCE_ROOT, else the tree
// this driver was built from.
fs::path source_root() {
    if (const char* root = std::getenv("MCXX_SOURCE_ROOT"); root != nullptr && *root != '\0') return root;
    // modules/driver/src/commands.cpp -> the repository
    return fs::path { std::source_location::current().file_name() }.parent_path().parent_path().parent_path().parent_path();
}

std::vector<features::PluginEntry> static_plugins(const features::Config& config) {
    std::vector<features::PluginEntry> out;
    for (const auto& e : config.plugins)
        if (e.is_static()) out.push_back(e);
    std::ranges::sort(out, {}, &features::PluginEntry::name);
    return out;
}

std::string toml_string(std::string_view text) {
    std::string out { "\"" };
    for (const char c : text) {
        if (c == '"' || c == '\\') out += '\\';
        out += c;
    }
    return out + "\"";
}

// Writes `text` unless the file already holds it: an unchanged composition leaves every file, and so
// the build, as it was (MC4-3-3).
void write_if_changed(const fs::path& file, std::string_view text) {
    if (std::ifstream in { file }; in) {
        const std::string old { std::istreambuf_iterator<char> { in }, {} };
        if (old == text) return;
    }
    std::ofstream { file } << text;
}

// The sources of a program the driver's own sources would give, with the package's static plugins.
void write_composition(const fs::path& dir, const features::Config& config, const std::vector<features::PluginEntry>& plugins,
                       const std::string& key) {
    const fs::path root { source_root() };
    const fs::path package_dir { fs::path { config.manifest }.parent_path() };
    fs::create_directories(dir / "src");
    std::string deps;
    std::string names;
    for (const auto& p : plugins) {
        if (!p.path.empty()) {
            // A path is keyed by the package's own name: the entry's is the package's alias.
            const fs::path at { (package_dir / p.path).lexically_normal() };
            std::string package { p.name };
            if (auto doc = base::toml::parse_file(at / "mcpp.toml"))
                if (const auto* table = doc->get_table("package"))
                    if (const auto it = table->find("name"); it != table->end() && it->second.is_string()) package = it->second.as_string();
            deps += std::format("{} = {{ path = {} }}\n", toml_string(package), toml_string(at.generic_string()));
        } else {
            deps += std::format("{} = {}\n", toml_string(p.name), toml_string(p.version));
        }
        names += std::format("{}{}", names.empty() ? "" : ", ", toml_string(p.name));
    }
    write_if_changed(dir / "mcpp.toml", std::format(
        "# Generated by `mcxx compose` for {} (key {}): the MC++ driver with this package's static\n"
        "# plugins. Do not edit; it is written again when the plugin set changes.\n"
        "[package]\nname = \"mcxx-composed\"\nversion = \"{}\"\n\n"
        "[indices]\nllvm = {{ path = {} }}\n\n"
        "[targets.mcxx]\nkind = \"bin\"\nmain = \"src/main.cpp\"\n\n"
        "[build]\nldflags = [\"-Wl,--error-limit=0\"]\n\n"
        "[dependencies]\nopenkal-llvm-runtime = \"0.15.2\"\n"
        "mcxx-driver = {{ path = {} }}\nmcxx-plugins-std = {{ path = {} }}\nmcxx-plugins-libs = {{ path = {} }}\n{}\n"
        "# Version settlement, as in MC++'s own root manifest.\n[dependencies.llvm]\nclang-dev = \"23.1.0.3\"\n",
        config.manifest, key, VERSION, toml_string((root / "index/llvm").generic_string()), toml_string((root / "modules/driver").generic_string()),
        toml_string((root / "plugins/std").generic_string()), toml_string((root / "plugins/libs").generic_string()), deps));
    write_if_changed(dir / "src/main.cpp", std::format(
        "// Generated by `mcxx compose`: mcxx with the static plugins of {}.\n"
        "import mcxx.driver;\n\nint main(int argc, char** argv) {{ return mcxx::driver::run(argc, argv, {{ {} }}, \"{}\"); }}\n",
        config.manifest, names, key));
    if (std::ifstream pin { root / ".xlings.json" }; pin) write_if_changed(dir / ".xlings.json", std::string { std::istreambuf_iterator<char> { pin }, {} });
}

// The composed compiler of a key, if it has been built.
std::optional<fs::path> composed_binary(const std::string& key) {
    const fs::path link { compose_cache() / key / "bin" / "mcxx" };
    std::error_code ec;
    if (fs::exists(link, ec)) return link;
    return std::nullopt;
}

int compose(const std::vector<std::string>& args) {
    std::optional<std::string> manifest;
    for (std::size_t i { 0 }; i < args.size(); ++i)
        if (args[i] == "--manifest" && i + 1 < args.size()) manifest = fs::absolute(args[++i]).lexically_normal().generic_string();
    if (!manifest) manifest = features::find_manifest((fs::current_path() / "x").generic_string());
    if (!manifest) {
        std::println(std::cerr, "mcxx compose: no mcpp.toml with a [package] here or above");
        return 2;
    }
    const features::Config config { features::read_config(*manifest) };
    for (const auto& p : config.problems) std::println(std::cerr, "warning: {}", p);
    const auto plugins = static_plugins(config);
    if (plugins.empty()) {
        std::println("{} declares no static plugins: mcxx itself compiles it", *manifest);
        return 0;
    }
    const std::string key { composition_key(*manifest) };
    const fs::path dir { compose_cache() / key };
    write_composition(dir, config, plugins, key);
    base::trace::Span span { "compose", "build", key };
    std::println(std::cerr, "mcxx compose: building {} in {}", key, dir.generic_string());
    // mcpp builds it; an unchanged set and unchanged sources are not relinked (MC4-3-3).
    const int status { std::system(std::format("cd '{}' && mcpp build", dir.generic_string()).c_str()) };
    if (status != 0) {
        std::println(std::cerr, "mcxx compose: the build failed ({})", status);
        return 1;
    }
    std::optional<fs::path> newest;
    std::error_code ec;
    for (const auto& e : fs::recursive_directory_iterator { dir / "target", ec })
        if (e.is_regular_file() && e.path().filename() == "mcxx" && e.path().parent_path().filename() == "bin")
            if (!newest || fs::last_write_time(e.path()) > fs::last_write_time(*newest)) newest = e.path();
    if (!newest) {
        std::println(std::cerr, "mcxx compose: the build left no mcxx under {}", (dir / "target").generic_string());
        return 1;
    }
    fs::create_directories(dir / "bin");
    fs::remove(dir / "bin" / "mcxx", ec);
    fs::create_symlink(*newest, dir / "bin" / "mcxx", ec);
    std::println("{}", (dir / "bin" / "mcxx").generic_string());
    return 0;
}

// The first source file of a compile command line.
std::optional<std::string> source_of(int argc, char** argv) {
    static constexpr std::string_view extensions[] { ".cpp", ".cc", ".cxx", ".c++", ".cppm", ".ixx", ".mpp", ".cxxm", ".c++m", ".c" };
    for (int i { 1 }; i < argc; ++i) {
        const std::string_view a { argv[i] };
        if (a.starts_with('-')) continue;
        for (const auto ext : extensions)
            if (a.ends_with(ext)) return std::string { a };
    }
    return std::nullopt;
}

// A compile whose package declares static plugins this program was not composed with: handed to
// the package's composed compiler when there is one, refused when not (MC4-3-4). Nothing to do
// (std::nullopt) when this program is the right one.
std::optional<int> hand_over(int argc, char** argv, const std::string& composition) {
    const auto source = source_of(argc, argv);
    if (!source) return std::nullopt;
    const auto manifest = features::find_manifest(*source);
    if (!manifest) return std::nullopt;
    const std::string key { composition_key(*manifest) };
    if (key.empty() || key == composition) return std::nullopt;
    if (const auto binary = composed_binary(key)) {
        if (const char* seen = std::getenv("MCXX_COMPOSED"); seen != nullptr && key == seen) {
            std::println(std::cerr, "mcxx: the composed compiler {} does not say it was composed for {}", binary->generic_string(), key);
            return 1;
        }
        ::setenv("MCXX_COMPOSED", key.c_str(), 1);
        ::execv(binary->c_str(), argv);
        std::println(std::cerr, "mcxx: cannot run {}: {}", binary->generic_string(), std::strerror(errno));
        return 1;
    }
    std::string names;
    for (const auto& p : static_plugins(features::read_config(*manifest))) names += std::format("{}{}", names.empty() ? "" : ", ", p.name);
    std::println(std::cerr, "mcxx: {} declares static plugins ({}) this mcxx was not composed with; run `mcxx compose` in {} (MC4-3-4)", *manifest, names,
                 fs::path { *manifest }.parent_path().generic_string());
    return 1;
}

} // namespace

std::string composition_key(std::string_view manifest) {
    const features::Config config { features::read_config(manifest) };
    const auto plugins = static_plugins(config);
    if (plugins.empty()) return {};
    const fs::path package_dir { fs::path { std::string { manifest } }.parent_path() };
    std::string input { std::format("mcxx {}\n{}\n", VERSION, source_root().generic_string()) };
    for (const auto& p : plugins)
        input += std::format("{}\t{}\t{}\n", p.name, p.path.empty() ? std::string {} : (package_dir / p.path).lexically_normal().generic_string(), p.version);
    return base::sha256_hex(input).substr(0, 16);
}

std::filesystem::path compose_cache() {
    if (const char* dir = std::getenv("MCXX_COMPOSE_CACHE"); dir != nullptr && *dir != '\0') return dir;
    if (const char* xdg = std::getenv("XDG_CACHE_HOME"); xdg != nullptr && *xdg != '\0') return std::filesystem::path { xdg } / "mcxx" / "compose";
    const char* home = std::getenv("HOME");
    return std::filesystem::path { home != nullptr ? home : "/tmp" } / ".cache" / "mcxx" / "compose";
}

// mcxx check -p DB [--cache DIR] [--resource DIR] FILE...: each file as the build database compiles it
// (whatever compiler the database names: GCC's commands too), parsed by the semantic backend, which
// builds the module interfaces the files import itself, into the cache, kept between runs. Every gate
// finding and compiler diagnostic is printed as a compiler prints it; 1 when there is an error (A1.5:
// a blocking check for a program no mcxx builds).
int check_database(std::vector<std::string> rest, const char* self) {
    std::string database, cache, resource;
    std::vector<std::string> files;
    for (std::size_t i { 0 }; i < rest.size(); ++i) {
        const std::string& a { rest[i] };
        if (a == "-p" && i + 1 < rest.size()) database = rest[++i];
        else if (a.starts_with("-p=")) database = a.substr(3);
        else if (a == "--cache" && i + 1 < rest.size()) cache = rest[++i];
        else if (a == "--resource" && i + 1 < rest.size()) resource = rest[++i];
        else files.push_back(fs::absolute(a).lexically_normal().generic_string());
    }
    auto commands = mcxx::serve::read_database(database);
    if (commands.empty() || files.empty()) {
        std::println(std::cerr, "mcxx check: {}", commands.empty() ? std::format("no build database at `{}`", database) : std::string { "no file to check" });
        return 2;
    }
    if (cache.empty()) {
        const char* home { std::getenv("HOME") };
        cache = (fs::path { home != nullptr ? home : "/tmp" } / ".cache" / "mcxx" / "check").generic_string();
    }
    if (resource.empty()) {   // the compiler's resource directory beside the program (the payload's layout)
        std::error_code ec;
        const auto dir = fs::weakly_canonical(fs::path { self }, ec).parent_path();
        for (const auto& candidate : { dir / "../lib/clang/23", dir / "lib/clang/23" })
            if (fs::is_directory(candidate, ec)) resource = candidate.lexically_normal().generic_string();
    }
    msa::Workspace::Options options;
    options.cache_directory = cache;
    options.resource_directory = resource;
    options.background_index = false;
    auto workspace = mcxx::backend::make_workspace(std::move(options));
    workspace->set_commands(std::move(commands));
    int errors { 0 };
    for (const auto& file : files) {
        std::ifstream in { file, std::ios::binary };
        std::string text { std::istreambuf_iterator<char> { in }, {} };
        const auto unit = workspace->parse(file, std::move(text), 1);
        if (!unit) {
            std::println(std::cerr, "{}: error: the build database has no command for it", file);
            ++errors;
            continue;
        }
        for (const auto& d : unit->diagnostics()) {
            if (d.severity != msa::Severity::error && d.severity != msa::Severity::warning) continue;
            const bool error { d.severity == msa::Severity::error };
            errors += error ? 1 : 0;
            std::println(std::cerr, "{}:{}:{}: {}: {}{}", file, d.range.begin.line + 1, d.range.begin.column + 1, error ? "error" : "warning", d.message,
                         d.code.empty() || plugin::find_feature(d.code) == nullptr || d.message.contains(std::format("[{}]", d.code))
                             ? std::string {}
                             : std::format(" [{}]", d.code));
        }
    }
    return errors > 0 ? 1 : 0;
}

int run(int argc, char** argv, std::vector<std::string> composed, std::string composition) {
    namespace compiler = mcxx::backend::compiler;
    plugin::host::set_composed(std::move(composed));
    // MCXX_LOG and MCXX_TRACE apply to a compile too: the gates' spans (gates.facts, gates.rules).
    mcxx::base::trace::configure_from_environment();
    struct Flush {
        ~Flush() { mcxx::base::trace::flush(); }
    } flush;
    // Re-invoked by the compiler itself (-cc1, -cc1as): the compiler, whole.
    if (argc > 1 && std::string_view { argv[1] }.starts_with("-cc1")) return compiler::run(argc, argv);
    // Named as a compiler: the compiler, whole -- by the package's composed compiler when the
    // package declares static plugins this one was not composed with (MC4-3-4).
    if (!compiler::mode_for_name(base_name(argv[0])).empty()) {
        if (const auto handed = hand_over(argc, argv, composition)) return *handed;
        return compiler::run(argc, argv);
    }
    if (argc < 2) {
        usage();
        return 2;
    }
    const std::string_view command { argv[1] };
    std::vector<std::string> rest(argv + 2, argv + argc);
    if (command == "c++" || command == "cc" || command == "check")
        if (const auto handed = hand_over(argc, argv, composition)) return *handed;
    if (command == "c++" || command == "cc") return compiler::run_as(command == "c++" ? "c++" : "c", argv[0], std::move(rest));
    if (command == "check") {
        if (!rest.empty() && (rest.front() == "-p" || rest.front().starts_with("-p="))) return check_database(std::move(rest), argv[0]);
        rest.push_back("-fsyntax-only");
        return compiler::run_as("c++", argv[0], std::move(rest));
    }
    if (command == "features") return list_features(std::ranges::find(rest, "--json") != rest.end());
    if (command == "compose") return compose(rest);
    if (command == "serve") {
        mcxx::serve::Options options;
        for (std::size_t i { 0 }; i + 1 < rest.size(); ++i) {
            if (rest[i] == "--db") options.database = rest[++i];
            else if (rest[i] == "--resource") options.resource = rest[++i];
            else if (rest[i] == "--cache") options.cache = rest[++i];
        }
        // Clang's resource directory beside the program, as the compiler finds it (the payload's layout).
        if (options.resource.empty()) {
            std::error_code ec;
            const auto self = fs::weakly_canonical(fs::path { argv[0] }, ec).parent_path();
            for (const auto& candidate : { self / "../lib/clang/23", self / "lib/clang/23" })
                if (fs::is_directory(candidate, ec)) options.resource = candidate.lexically_normal().generic_string();
        }
        std::ios::sync_with_stdio(false);
        return mcxx::serve::run(std::cin, std::cout, std::move(options));
    }
    if (command == "version" || command == "--version") {
        if (std::ranges::find(rest, "--json") != rest.end()) {
            // MC5 §5 (specs/schema/mc5-version.schema.json).
            std::string providers;
            for (const auto& p : plugin::catalog()->providers) providers += (providers.empty() ? "" : ",") + json_string(p.provider->name());
            const std::string full { compiler::version() };   // "clang 23.1.0"
            std::string_view clang { full };
            if (const auto space = clang.rfind(' '); space != std::string_view::npos) clang.remove_prefix(space + 1);
            std::println("{{\"mcxx\":\"{}\",\"compiler\":{{\"name\":\"clang\",\"version\":{}}},\"specifications\":{{\"mc1\":\"0.2.0\","
                         "\"mc3\":\"0.2.0\",\"mc4\":\"0.2.0\",\"mc4-protocols\":[1],\"mc5\":\"0.1.1\",\"mc6\":1}},\"providers\":[{}]}}",
                         VERSION, json_string(clang), providers);
            return 0;
        }
        std::println("mcxx {} ({})", VERSION, compiler::version());
        return 0;
    }
    usage();
    return 2;
}

} // namespace mcxx::driver
