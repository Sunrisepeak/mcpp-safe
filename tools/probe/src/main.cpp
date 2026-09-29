// mcxx-probe: drive mcxx.lsp over libmc++'s backend from the command line.
//
//   mcxx-probe --db DIR --resource DIR --cache DIR [--index] [--facts] [--census] FILE [LINE:COL METHOD]...
//
// Loads DIR/compile_commands.json, opens FILE, waits for its parse, prints its diagnostics, then
// for each LINE:COL METHOD (1-based line and column, e.g. 12:5 hover) prints the service's answer.
// With --index it also waits for the program index before asking; with --facts it prints the file's
// MC3 facts, every kind, as JSON on standard output (specs/mc3-facts.md §4.11); with --census, the
// declarations counted straight off Clang's AST by Clang's kind names (tools/checks/facts.py); with
// --symbols, the file's outline (Unit::symbols), one JSON object (tools/checks/syntaxdiff.py).
//
//   mcxx-probe --tokens FILE...
//
// prints every token the backend's raw lexer finds in each FILE, comments included, one JSON object
// per line as mcxx-lexdump prints MC++'s own (tools/checks/lexdiff.py); `--tokens --bench N FILE...`
// lexes them N times and prints bytes, tokens and the best time, as `mcxx-lexdump --bench` does.
//
//   mcxx-probe --read-ifc X.ifc
//
// prints an interface MC++ wrote beside a BMI (MC2, mcxx.ifc) as one JSON object: its module, its
// dialect and its declarations as MC3 facts. With `--ifc X.ifc` a parse of FILE is compared with it:
// the declarations FILE's facts say are not local, item by item, against those read back from X.ifc
// (A1.1.3, tools/checks/ifc.py); one JSON object, exit 1 when they differ.
import std;
import nlohmann.json;
import mcxx.msa;
import mcxx.base;
import mcxx.backend;
import mcxx.lsp;
import mcxx.plugin.wire;
import mcxx.ifc;

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

Json declarations_json(const std::vector<mcxx::msa::fact::Declaration>& declarations, std::string_view path, std::string_view module) {
    mcxx::msa::fact::Facts facts;
    facts.collected = mcxx::msa::fact::Kinds::declarations | mcxx::msa::fact::Kinds::declaration_types;
    facts.declarations = declarations;
    return mcxx::plugin::wire::facts_to_json(facts, path, module)["declarations"];
}

int read_ifc(const std::string& path) {
    const auto unit = mcxx::ifc::load(path);
    if (!unit) {
        std::println(std::cerr, "{}", unit.error());
        return 1;
    }
    Json features = Json::object();
    for (const auto& f : unit->dialect.features) features[f.id] = f.level;
    Json namespaces = Json::object();
    for (const auto& n : unit->dialect.namespaces) namespaces[n.name][n.feature] = n.level;
    Json doc = Json::object();
    doc["module"] = unit->module;
    doc["internal"] = unit->internal;
    doc["source"] = unit->source;
    doc["target"] = unit->target;
    doc["cplusplus"] = unit->cplusplus;
    doc["dialect"] = Json { { "profiles", unit->dialect.profiles }, { "features", features }, { "namespaces", namespaces } };
    doc["reexports"] = unit->reexports;
    doc["declarations"] = declarations_json(unit->declarations, unit->source, unit->module);
    doc["reachable"] = declarations_json(unit->reachable, unit->source, unit->module);   // MC2 1.2.0
    std::println("{}", doc.dump());
    return 0;
}

} // namespace

int main(int argc, char** argv) {
    mcxx::base::trace::configure_from_environment();   // MCXX_LOG, MCXX_TRACE, whichever path runs
    if (argc > 1 && std::string_view { argv[1] } == "--tokens") return tokens(argc, argv);
    if (argc == 3 && std::string_view { argv[1] } == "--read-ifc") return read_ifc(argv[2]);
    std::string db, resource, cache, file, ifc;
    bool index { false };
    bool facts { false };
    bool census { false };
    bool symbols { false };
    bool references { false };
    std::vector<std::pair<std::string, std::string>> asks;
    for (int i { 1 }; i < argc; ++i) {
        const std::string a { argv[i] };
        if (a == "--db" && i + 1 < argc) db = argv[++i];
        else if (a == "--resource" && i + 1 < argc) resource = argv[++i];
        else if (a == "--cache" && i + 1 < argc) cache = argv[++i];
        else if (a == "--index") index = true;
        else if (a == "--facts") facts = true;
        else if (a == "--census") census = true;
        else if (a == "--symbols") symbols = true;
        else if (a == "--references") references = true;
        else if (a == "--ifc" && i + 1 < argc) ifc = argv[++i];
        else if (file.empty()) file = a;
        else if (i + 1 < argc) {
            asks.emplace_back(a, argv[i + 1]);
            ++i;
        }
    }
    if (db.empty() || file.empty()) {
        std::println(std::cerr, "usage: mcxx-probe --db DIR --resource DIR --cache DIR [--index] [--facts] [--census] [--symbols] [--references] [--ifc X.ifc] FILE [LINE:COL METHOD]...");
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
    if (!ifc.empty()) {
        if (!unit) {
            std::println(std::cerr, "{}: no parse to compare with", path);
            return 1;
        }
        const auto written = mcxx::ifc::load(ifc);
        Json result = Json::object();
        result["ifc"] = ifc;
        result["file"] = path;
        std::vector<std::string> problems;
        if (!written) problems.push_back(written.error());
        else {
            const auto expected = mcxx::ifc::interface_declarations(unit->facts());
            result["declarations"] = expected.size();
            problems = mcxx::ifc::differences(expected, written->declarations);
            if (written->module != unit->module_name()) problems.push_back(std::format("module {} != {}", unit->module_name(), written->module));
            if (written->source != path) problems.push_back(std::format("source {} != {}", path, written->source));
        }
        result["differences"] = problems;
        std::println("{}", result.dump());
        if (!problems.empty()) return 1;
    }
    if (symbols && unit) {
        Json list = Json::array();
        const std::function<void(const std::vector<mcxx::msa::Symbol>&, int)> walk = [&](const std::vector<mcxx::msa::Symbol>& ss, int parent) {
            for (const auto& s : ss) {
                const int self { static_cast<int>(list.size()) };
                Json x = Json::object();
                x["kind"] = std::string { mcxx::msa::to_string(s.kind) };
                x["name"] = s.name;
                x["range"] = Json::array({ s.range.begin.line, s.range.begin.column, s.range.end.line, s.range.end.column });
                x["selection"] = Json::array({ s.selection.begin.line, s.selection.begin.column, s.selection.end.line, s.selection.end.column });
                x["parent"] = parent;
                list.push_back(std::move(x));
                walk(s.children, self);
            }
        };
        walk(unit->symbols(), -1);
        Json doc = Json::object();
        doc["symbols"] = std::move(list);
        std::println("{}", doc.dump());
    }
    // Each name the file writes that names a declaration, and what it names (tools/checks/refsdiff.py,
    // M2.1's name lookup): Clang's occurrences that are references, not a declaration, not implied.
    if (references && unit) {
        Json list = Json::array();
        const auto path = unit->path();
        for (const auto& o : unit->occurrences()) {
            using namespace mcxx::msa;
            if ((o.roles & role::reference) == 0 || (o.roles & (role::declaration | role::definition | role::implicit)) != 0) continue;
            const auto e = unit->entity(o.entity);
            if (!e) continue;
            Json x = Json::object();
            x["range"] = Json::array({ o.range.begin.line, o.range.begin.column, o.range.end.line, o.range.end.column });
            x["name"] = o.name;
            x["target"] = e->qualified_name;
            x["kind"] = std::string { to_string(e->kind) };
            // Where what it names is declared, when that is this file.
            if (e->declaration && e->declaration->path == path)
                x["declaration"] = Json::array({ e->declaration->range.begin.line, e->declaration->range.begin.column });
            list.push_back(std::move(x));
        }
        Json doc = Json::object();
        doc["references"] = std::move(list);
        std::println("{}", doc.dump());
    }
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
