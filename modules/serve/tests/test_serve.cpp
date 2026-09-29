// mcxx serve end to end: this test program is its own server (`--serve`), driven through the
// supervising Client -- the handshake, a document's diagnostics, MC++'s requests (facts, gates, the
// catalog) -- and killed, to see the Client start it again and replay what it was told (A1.3.2).
import std;
import nlohmann.json;
import mcxx.testing;
import mcxx.serve;
import mcxx.plugins.std;

extern "C" int kill(int pid, int signal);

namespace serve = mcxx::serve;
using serve::Json;
namespace fs = std::filesystem;

namespace {

struct Program {
    fs::path root { fs::temp_directory_path() / std::format("mcxx-serve-{}", std::random_device {}()) };
    std::string file;
    Program() {
        fs::create_directories(root / "src");
        std::ofstream { root / "mcpp.toml" } << "[package]\nname = \"t\"\nversion = \"0.1.0\"\n[package.metadata.mcxx.features]\ngoto = \"deny\"\n"
                                                 "new-delete = \"deny\"\nreinterpret-cast = \"deny\"\nraw-pointers = \"deny\"\nmacros = \"deny\"\n";
        file = (root / "src/a.cpp").generic_string();
        std::ofstream { file } << text();
    }
    ~Program() {
        std::error_code ec;
        fs::remove_all(root, ec);
    }
    static std::string text() { return "int f(int n) {\n    if (n) goto out;\n    return 1;\nout:\n    return 0;\n}\n"; }
    Json commands() const {
        Json c = Json::object();
        c["directory"] = root.generic_string();
        c["file"] = file;
        c["arguments"] = Json::array({ "clang++", "-std=c++23", "-c", file });
        Json p = Json::object();
        p["commands"] = Json::array({ c });
        return p;
    }
    Json open() const {
        Json doc = Json::object();
        doc["uri"] = "file://" + file;
        doc["languageId"] = "cpp";
        doc["version"] = 1;
        doc["text"] = text();
        Json p = Json::object();
        p["textDocument"] = doc;
        return p;
    }
    Json document() const {
        Json p = Json::object();
        p["textDocument"]["uri"] = "file://" + file;
        return p;
    }
};

struct Seen {
    std::mutex mutex;
    std::condition_variable changed;
    std::vector<Json> diagnostics;
    bool wait_for_goto(std::chrono::seconds limit) {
        std::unique_lock lock { mutex };
        return changed.wait_for(lock, limit, [&] {
            for (const auto& d : diagnostics)
                for (const auto& x : d.value("diagnostics", Json::array()))
                    if (x.value("code", std::string {}) == "goto") return true;
            return false;
        });
    }
};

} // namespace

int main(int argc, char** argv) {
    if (argc > 1 && std::string_view { argv[1] } == "--serve") {
        serve::Options options;
        options.cache = argc > 2 ? argv[2] : "";
        options.workers = 2;
        return serve::run(std::cin, std::cout, options);
    }
    using namespace mcxx::testing;
    const std::string self { fs::read_symlink("/proc/self/exe").generic_string() };

    "a message is framed as LSP's base protocol says, and read back"_test = [] {
        Json m = Json::object();
        m["jsonrpc"] = "2.0";
        m["method"] = "x";
        std::stringstream s { serve::frame(m) + serve::frame(m) };
        expect(serve::frame(m).starts_with("Content-Length: "));
        expect(serve::read_message(s) == m && serve::read_message(s) == m && !serve::read_message(s));
    };

    "exit without shutdown ends with 1, after shutdown with 0"_test = [] {
        Json init = Json::object();
        init["jsonrpc"] = "2.0";
        init["id"] = 1;
        init["method"] = "initialize";
        init["params"] = Json::object();
        Json shutdown = init;
        shutdown["id"] = 2;
        shutdown["method"] = "shutdown";
        Json exit = Json::object();
        exit["jsonrpc"] = "2.0";
        exit["method"] = "exit";
        std::stringstream in1 { serve::frame(init) + serve::frame(exit) }, out1;
        std::stringstream in2 { serve::frame(init) + serve::frame(shutdown) + serve::frame(exit) }, out2;
        serve::Options o;
        o.cache = (fs::temp_directory_path() / "mcxx-serve-exit").generic_string();
        expect(serve::run(in1, out1, o) == 1);
        expect(serve::run(in2, out2, o) == 0);
        expect(out2.str().contains("\"experimental\"") && out2.str().contains("mcxx/facts")) << "initialize names MC6's requests";
    };

    Program program;
    Seen seen;
    const std::string cache { (program.root / ".cache").generic_string() };
    serve::Client client { { self, "--serve", cache }, [&](std::string_view method, const Json& params) {
                              if (method != "textDocument/publishDiagnostics") return;
                              std::lock_guard lock { seen.mutex };
                              seen.diagnostics.push_back(params);
                              seen.changed.notify_all();
                          } };

    "a document's gate findings arrive as diagnostics; facts, gates and the catalog answer"_test = [&] {
        auto set = client.request("mcxx/setCommands", program.commands());
        expect(fatal(set.has_value())) << (set ? "" : set.error());
        client.notify("textDocument/didOpen", program.open());
        expect(seen.wait_for_goto(std::chrono::seconds { 60 })) << "a goto, denied by the package, is published";
        auto facts = client.request("mcxx/facts", program.document());
        expect(fatal(facts.has_value())) << (facts ? "" : facts.error());
        expect((*facts)["mc3-version"] == "0.2.0" && (*facts)["gotos"].size() == 1);
        auto gates = client.request("mcxx/gates", program.document());
        expect(fatal(gates.has_value()));
        const auto& list = (*gates)["features"];
        const auto g = std::ranges::find_if(list, [](const Json& f) { return f["id"] == "goto"; });
        expect(g != list.end() && (*g)["level"] == "deny" && (*g)["gated"] == true);
        auto catalog = client.request("mcxx/catalog", Json::object());
        expect(catalog.has_value() && (*catalog)["mc1-version"] == "0.2.0" && (*catalog)["features"].size() > 15);
        auto symbols = client.request("textDocument/documentSymbol", program.document());
        expect(symbols.has_value() && symbols->is_array() && !symbols->empty()) << "LSP's own requests are the service's";
    };

    "an edit's syntax-level gate findings arrive within 100 ms, before its parse (A1.8.3)"_test = [&] {
        // goto, new and delete, reinterpret_cast, a written `*` declarator, a macro: what MC++'s own front
        // end reads without a parse.
        const std::string edited { "#define LIMIT 4\nint f(int n) {\n    int* p = new int { n };\n    if (n) goto out;\n"
                                   "    long a = reinterpret_cast<long>(p);\n    delete p;\n    return 1;\nout:\n    return 0;\n}\n" };
        Json change = Json::object();
        change["textDocument"]["uri"] = "file://" + program.file;
        change["textDocument"]["version"] = 2;
        Json whole = Json::object();
        whole["text"] = edited;
        change["contentChanges"] = Json::array({ whole });
        const auto sent = std::chrono::steady_clock::now();
        client.notify("textDocument/didChange", change);
        const std::set<std::string> wanted { "goto", "new-delete", "reinterpret-cast", "raw-pointers", "macros" };
        std::chrono::steady_clock::duration took {};
        std::set<std::string> got;
        {
            std::unique_lock lock { seen.mutex };
            const bool arrived { seen.changed.wait_for(lock, std::chrono::seconds { 10 }, [&] {
                for (const auto& d : seen.diagnostics) {
                    // At once, before the parse: published without a version (it is not the version's answer).
                    got.clear();
                    for (const auto& x : d.value("diagnostics", Json::array())) got.insert(x.value("code", std::string {}));
                    if (std::ranges::includes(got, wanted)) {
                        took = std::chrono::steady_clock::now() - sent;
                        return true;
                    }
                }
                return false;
            }) };
            expect(fatal(arrived)) << "every one of goto, new-delete, reinterpret-cast, raw-pointers, macros";
        }
        const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(took).count();
        std::println(std::cerr, "  the edit's syntax-level findings after {} ms", ms);
        expect(ms <= 100) << std::format("{} ms", ms);
    };

    "killed, the server is started again and told again what it was told"_test = [&] {
        const int before { client.pid() };
        expect(fatal(before > 0));
        kill(before, 9);
        std::this_thread::sleep_for(std::chrono::milliseconds { 200 });
        auto lost = client.request("mcxx/facts", program.document());   // may meet the dead one, or the new one
        auto facts = lost.has_value() ? lost : client.request("mcxx/facts", program.document());
        expect(fatal(facts.has_value())) << (facts ? "" : facts.error());
        expect(client.restarts() == 1 && client.pid() != before) << client.restarts();
        expect((*facts)["gotos"].size() == 1) << "the commands and the open document were replayed";
    };

    return report();
}
