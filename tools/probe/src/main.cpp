// mcxx-probe: drive mcxx.lsp over libmc++'s backend from the command line.
//
//   mcxx-probe --db DIR --resource DIR --cache DIR [--index] [--facts] [--census] FILE [LINE:COL METHOD]...
//
// Loads DIR/compile_commands.json, opens FILE, waits for its parse, prints its diagnostics, then
// for each LINE:COL METHOD (1-based line and column, e.g. 12:5 hover) prints the service's answer.
// With --index it also waits for the program index before asking; with --facts it prints the file's
// MC3 facts, every kind, as JSON on standard output (specs/mc3-facts.md §4.11); with --census, the
// declarations counted straight off Clang's AST by Clang's kind names (tools/checks/facts.py).
//
//   mcxx-probe --tokens FILE...
//
// prints every token the backend's raw lexer finds in each FILE, comments included, one JSON object
// per line as mcxx-lexdump prints MC++'s own (tools/checks/lexdiff.py); `--tokens --bench N FILE...`
// lexes them N times and prints bytes, tokens and the best time, as `mcxx-lexdump --bench` does.
import std;
import nlohmann.json;
import mcxx.msa;
import mcxx.backend;
import mcxx.lsp;
import mcxx.plugin.wire;

using Json = nlohmann::json;

namespace {

std::vector<mcxx::msa::Command> load(const std::string& db) {
    std::ifstream in { db + "/compile_commands.json" };
    if (!in) throw std::runtime_error("cannot read " + db + "/compile_commands.json");
    const Json entries = Json::parse(in);
    std::vector<mcxx::msa::Command> commands;
    for (const auto& e : entries) {
        mcxx::msa::Command c;
        c.directory = e.value("directory", std::string {});
        c.file = e.value("file", std::string {});
        if (!std::filesystem::path { c.file }.is_absolute()) c.file = (std::filesystem::path { c.directory } / c.file).lexically_normal().string();
        if (e.contains("arguments")) c.arguments = e["arguments"].get<std::vector<std::string>>();
        else {
            std::istringstream words { e.value("command", std::string {}) };
            for (std::string w; words >> w;) c.arguments.push_back(w);
        }
        commands.push_back(std::move(c));
    }
    return commands;
}

std::string read(const std::string& path) {
    std::ifstream in { path, std::ios::binary };
    std::ostringstream text;
    text << in.rdbuf();
    return std::move(text).str();
}

double seconds_since(std::chrono::steady_clock::time_point t) { return std::chrono::duration<double>(std::chrono::steady_clock::now() - t).count(); }

std::string quoted(std::string_view s) { return Json(std::string { s }).dump(-1, ' ', false, Json::error_handler_t::replace); }

int tokens(int argc, char** argv) {
    if (argc > 3 && std::string_view { argv[2] } == "--bench") {
        std::vector<std::string> texts;
        std::size_t bytes { 0 };
        for (int i { 4 }; i < argc; ++i) bytes += texts.emplace_back(read(argv[i])).size();
        double best { 1e9 };
        std::size_t count { 0 };
        for (int r { 0 }; r < std::stoi(argv[3]); ++r) {
            const auto started = std::chrono::steady_clock::now();
            count = 0;
            for (const auto& t : texts) count += mcxx::backend::raw_tokens(t).size();
            best = std::min(best, seconds_since(started));
        }
        std::println("{{\"lexer\":\"clang raw\",\"bytes\":{},\"tokens\":{},\"seconds\":{:.4f},\"mb-per-second\":{:.1f}}}", bytes, count, best,
                     static_cast<double>(bytes) / best / 1e6);
        return 0;
    }
    for (int i { 2 }; i < argc; ++i) {
        const auto text = read(argv[i]);
        for (const auto& t : mcxx::backend::raw_tokens(text))
            std::println("{{\"file\":{},\"kind\":\"{}\",\"line\":{},\"column\":{},\"text\":{}}}", quoted(argv[i]), t.kind, t.line, t.column,
                         quoted(std::string_view { text }.substr(t.begin, t.end - t.begin)));
    }
    return 0;
}

} // namespace

int main(int argc, char** argv) {
    if (argc > 1 && std::string_view { argv[1] } == "--tokens") return tokens(argc, argv);
    std::string db, resource, cache, file;
    bool index { false };
    bool facts { false };
    bool census { false };
    std::vector<std::pair<std::string, std::string>> asks;
    for (int i { 1 }; i < argc; ++i) {
        const std::string a { argv[i] };
        if (a == "--db" && i + 1 < argc) db = argv[++i];
        else if (a == "--resource" && i + 1 < argc) resource = argv[++i];
        else if (a == "--cache" && i + 1 < argc) cache = argv[++i];
        else if (a == "--index") index = true;
        else if (a == "--facts") facts = true;
        else if (a == "--census") census = true;
        else if (file.empty()) file = a;
        else if (i + 1 < argc) {
            asks.emplace_back(a, argv[i + 1]);
            ++i;
        }
    }
    if (db.empty() || file.empty()) {
        std::println(std::cerr, "usage: mcxx-probe --db DIR --resource DIR --cache DIR [--index] [--facts] [--census] FILE [LINE:COL METHOD]...");
        return 2;
    }
    const auto started = std::chrono::steady_clock::now();
    mcxx::msa::Workspace::Options options;
    options.cache_directory = cache.empty() ? std::string { "/tmp/mcxx-probe-cache" } : cache;
    options.resource_directory = resource;
    options.workers = std::max(1u, std::thread::hardware_concurrency() / 2);
    options.log = [&](mcxx::msa::LogLevel, std::string_view category, std::string_view line) {
        std::println(std::cerr, "[{:7.2f}] {}: {}", seconds_since(started), category, line);
    };
    auto workspace = mcxx::backend::make_workspace(options);
    workspace->set_commands(load(db));
    std::println(std::cerr, "[{:7.2f}] {} commands, {} modules", seconds_since(started), workspace->status().units, workspace->status().modules);

    mcxx::lsp::Service service { *workspace, {}, [&](std::string_view method, Json params) {
                                    if (method == "textDocument/publishDiagnostics") {
                                        std::println(std::cerr, "[{:7.2f}] diagnostics v{}: {}", seconds_since(started), params["version"].dump(),
                                                     params["diagnostics"].size());
                                        for (const auto& d : params["diagnostics"])
                                            std::println(std::cerr, "    {}:{} {}", d["range"]["start"]["line"].get<int>() + 1,
                                                         d["range"]["start"]["character"].get<int>() + 1, d.value("message", std::string {}));
                                    }
                                } };
    const std::string path { std::filesystem::absolute(file).lexically_normal().string() };
    const std::string uri { mcxx::lsp::path_to_uri(path) };
    service.open(uri, read(path), 1);
    const auto unit = service.unit(uri, true);
    std::println(std::cerr, "[{:7.2f}] parsed: {} occurrences", seconds_since(started), unit ? unit->occurrences().size() : 0);
    if (facts && unit) std::println("{}", mcxx::plugin::wire::facts_to_json(unit->facts(), path, unit->module_name()).dump());
    if (census && unit) {
        Json counts = Json::object();
        for (const auto& [kind, n] : mcxx::backend::census(*unit)) counts[kind] = n;
        std::println("{}", Json { { "census", counts } }.dump());
    }
    if (index) {
        while (true) {
            const auto s = workspace->status();
            if (!s.busy) break;
            std::this_thread::sleep_for(std::chrono::milliseconds { 500 });
        }
        const auto s = workspace->status();
        std::println(std::cerr, "[{:7.2f}] index: {} of {} units; modules ready {} failed {}", seconds_since(started), s.indexed, s.units, s.modules_ready,
                     s.modules_failed);
        for (const auto& f : s.failures) std::println(std::cerr, "    failed {} ({}): {}", f.module, f.cause, f.reason.substr(0, 300));
        for (const auto& [name, value] : s.counters) std::println(std::cerr, "    count {} = {}", name, value);
    }
    for (const auto& [where, method] : asks) {
        const auto colon = where.find(':');
        const int line { std::stoi(where.substr(0, colon)) - 1 };
        const int character { std::stoi(where.substr(colon + 1)) - 1 };
        Json params { { "textDocument", Json { { "uri", uri } } }, { "position", Json { { "line", line }, { "character", character } } },
                      { "context", Json { { "includeDeclaration", true } } }, { "query", "" } };
        const auto asked = std::chrono::steady_clock::now();
        const std::string full { method.find('/') == std::string::npos ? "textDocument/" + method : method };
        const auto result = service.request(full, params);
        std::println("== {} {} ({:.3f} s)", method, where, seconds_since(asked));
        if (result) std::println("{}", result->dump(2).substr(0, 3000));
        else std::println("error {}: {}", result.error().code, result.error().message);
    }
    return 0;
}
