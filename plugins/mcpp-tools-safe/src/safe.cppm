// mcxx.check: MC++'s gates as build-graph nodes (E-PLG-1).
//
//   // build.mcpp of a program built with GCC
//   import std;
//   import mcpp;
//   import mcxx.check;
//   int main() { return mcxx::check::sources() ? 0 : 1; }
//
// One edge: `mcxx check -p <the build's compile database> <files>`, blocking, its inputs the files and
// the manifest (whose [package.metadata.mcxx] says what is gated), its output a stamp mcpp writes when
// the check passes. mcxx reads each file as the database compiles it -- GCC's commands too -- and
// builds the module interfaces the files import itself, into a cache kept beside the stamp, so a run
// after an edit reuses what did not change. One edge rather than one per file: the files share those
// interfaces, built once per run.
export module mcxx.check;

import std;
import mcpp;

export namespace mcxx::check {

struct options {
    // Where the stamp and mcxx's interface cache go.
    std::string out_dir = std::string(mcpp::out_dir()) + "/mcxx-check";
    // The check gates compilation: a gate's error stops the build before anything is compiled.
    bool blocking = true;
    // mcxx. Empty: the consumer's dependency graph (`llvm = { version = "23.1.0-mcxx", tools =
    // ["mcxx"] }` in [build-dependencies]), then the MCXX environment variable.
    std::string program;
    // Passed to mcxx check as they are.
    std::vector<std::string> args;
};

struct edge {
    std::string              id;
    std::string              description;
    std::vector<std::string> command;
    std::vector<std::string> inputs;
    std::vector<std::string> outputs;
    bool                     blocking = true;
};

// The C++ sources under `dir` (relative to the manifest), sorted: .cpp, .cppm, .cc, .cxx, .ixx.
inline std::vector<std::string> find_sources(std::string_view dir = "src") {
    std::vector<std::string> out;
    const std::filesystem::path root { mcpp::manifest_dir() };
    std::error_code ec;
    for (auto it = std::filesystem::recursive_directory_iterator { root / dir, ec }; !ec && it != std::filesystem::recursive_directory_iterator {};
         it.increment(ec)) {
        const auto ext = it->path().extension().string();
        if (it->is_regular_file() && (ext == ".cpp" || ext == ".cppm" || ext == ".cc" || ext == ".cxx" || ext == ".ixx"))
            out.push_back(std::filesystem::relative(it->path(), root).generic_string());
    }
    std::ranges::sort(out);
    return out;
}

inline std::optional<edge> plan(std::span<const std::string> files, options opt = {}) {
    const std::string root { mcpp::manifest_dir() };
    if (root.empty()) {
        std::println(std::cerr, "mcxx.check: no mcpp build context -- this runs from build.mcpp");
        return std::nullopt;
    }
    if (files.empty()) {
        std::println(std::cerr, "mcxx.check: no files to check");
        return std::nullopt;
    }
    std::string exe { opt.program };
    if (exe.empty())
        if (const char* p = mcpp::dep_bin("llvm", "mcxx"); p && *p) exe = p;
    if (exe.empty())
        if (const char* p = mcpp::env_or("MCXX"); p && *p) exe = p;
    if (exe.empty()) {
        std::println(std::cerr, "mcxx.check: no mcxx. Declare the toolchain package that provides it, so its version is yours to pin:\n"
                                "  [build-dependencies]\n"
                                "  llvm = {{ version = \"23.1.0-mcxx\", tools = [\"mcxx\"] }}\n"
                                "  (or set options::program, or MCXX, to its path)");
        return std::nullopt;
    }
    edge e;
    e.id = "mcxx-check";
    e.command = { exe, "check", "-p", "${mcpp.compile_db}", "--cache", opt.out_dir + "/cache" };
    for (const auto& a : opt.args) e.command.push_back(a);
    for (const auto& f : files) {
        e.command.push_back(root + "/" + f);
        e.inputs.push_back(root + "/" + f);
    }
    e.inputs.push_back(root + "/mcpp.toml");   // what is gated
    e.outputs = { opt.out_dir + "/mcxx-check.stamp" };
    e.blocking = opt.blocking;
    e.description = std::format("mcxx check: {} file{} ({})", files.size(), files.size() == 1 ? "" : "s", opt.blocking ? "blocking" : "beside the build");
    return e;
}

inline bool submit(const edge& e) {
    mcpp::action a;
    a.id          = e.id.c_str();
    a.role        = "check";
    a.description = e.description.c_str();
    a.blocking    = e.blocking;
    for (const auto& c : e.command) a.arg(c.c_str());
    for (const auto& i : e.inputs)  a.input(i.c_str());
    for (const auto& o : e.outputs) a.output(o.c_str());
    a.submit();
    return true;
}

// Check these files (relative to the manifest).
inline bool check(std::span<const std::string> files, options opt = {}) {
    const auto e = plan(files, std::move(opt));
    return e && submit(*e);
}

// Check every source under src/.
inline bool sources(options opt = {}) {
    const auto files = find_sources();
    return check(files, std::move(opt));
}

} // namespace mcxx::check
