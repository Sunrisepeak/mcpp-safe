// mcxx.serve: framing and the server.
module mcxx.serve;

import std;
import nlohmann.json;
import mcxx.base;
import mcxx.msa;
import mcxx.lsp;
import mcxx.backend;
import mcxx.plugin;
import mcxx.plugin.wire;
import mcxx.features;

namespace mcxx::serve {

namespace fs = std::filesystem;

std::string frame(const Json& message) {
    const std::string body { message.dump(-1, ' ', false, Json::error_handler_t::replace) };
    return std::format("Content-Length: {}\r\n\r\n{}", body.size(), body);
}

std::optional<Json> read_message(std::istream& in) {
    std::size_t length { 0 };
    bool sized { false };
    for (std::string line; std::getline(in, line);) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) {
            if (!sized) return std::nullopt;
            std::string body(length, '\0');
            if (!in.read(body.data(), static_cast<std::streamsize>(length))) return std::nullopt;
            Json message = Json::parse(body, nullptr, false);
            if (message.is_discarded()) return Json::object();   // answered as a parse error
            return message;
        }
        constexpr std::string_view header { "Content-Length:" };
        if (line.starts_with(header)) {
            length = std::stoul(line.substr(header.size()));
            sized = true;
        }
    }
    return std::nullopt;
}

namespace {

std::vector<msa::Command> commands_from(const Json& list) {
    std::vector<msa::Command> commands;
    for (const auto& e : list) {
        msa::Command c;
        c.directory = e.value("directory", std::string {});
        c.file = e.value("file", std::string {});
        if (!fs::path { c.file }.is_absolute()) c.file = (fs::path { c.directory } / c.file).lexically_normal().generic_string();
        if (e.contains("arguments")) c.arguments = e["arguments"].get<std::vector<std::string>>();
        else {
            std::istringstream words { e.value("command", std::string {}) };
            for (std::string w; words >> w;) c.arguments.push_back(w);
        }
        commands.push_back(std::move(c));
    }
    return commands;
}

std::string default_cache() {
    const char* home = std::getenv("HOME");
    return (fs::path { home != nullptr ? home : "/tmp" } / ".cache" / "mcxx" / "serve").generic_string();
}

class Server {
public:
    Server(std::ostream& out, Options options) : out_ { out } {
        msa::Workspace::Options w;
        w.cache_directory = options.cache.empty() ? default_cache() : options.cache;
        w.resource_directory = options.resource;
        w.workers = options.workers != 0 ? options.workers : std::max(1u, std::thread::hardware_concurrency() / 2);
        workspace_ = backend::make_workspace(std::move(w));
        if (!options.database.empty()) {
            std::ifstream in { fs::path { options.database } / "compile_commands.json" };
            if (in) workspace_->set_commands(commands_from(Json::parse(in, nullptr, false)));
        }
        service_ = std::make_unique<lsp::Service>(*workspace_, lsp::Options {}, [this](std::string_view method, Json params) {
            Json message = Json::object();
            message["jsonrpc"] = "2.0";
            message["method"] = std::string { method };
            message["params"] = std::move(params);
            send(message);
        });
    }

    // Handles one message; false after `exit`.
    bool handle(const Json& m) {
        if (!m.is_object() || !m.contains("method")) {
            if (m.is_object() && m.empty()) respond(nullptr, error(-32700, "not JSON"));
            return true;
        }
        const std::string method { m["method"].get<std::string>() };
        const Json params = m.value("params", Json::object());
        const bool request { m.contains("id") };
        if (method == "exit") return false;
        if (!request) {
            notification(method, params);
            return true;
        }
        const Json id = m["id"];
        base::trace::Span span { "serve", "request", method, std::chrono::milliseconds { 1000 } };
        try {
            respond(id, answer(method, params));
        } catch (const std::exception& e) {
            respond(id, error(-32603, e.what()));
        }
        return true;
    }

    bool shut_down() const { return shutdown_; }

private:
    std::ostream& out_;
    std::mutex write_;
    std::unique_ptr<msa::Workspace> workspace_;
    std::unique_ptr<lsp::Service> service_;
    bool shutdown_ { false };

    void send(const Json& message) {
        std::lock_guard lock { write_ };
        out_ << frame(message) << std::flush;
    }

    static std::expected<Json, lsp::Error> error(int code, std::string message) { return std::unexpected(lsp::Error { code, std::move(message) }); }

    void respond(const Json& id, const std::expected<Json, lsp::Error>& result) {
        Json message = Json::object();
        message["jsonrpc"] = "2.0";
        message["id"] = id;
        if (result) message["result"] = *result;
        else {
            Json e = Json::object();
            e["code"] = result.error().code;
            e["message"] = result.error().message;
            message["error"] = std::move(e);
        }
        send(message);
    }

    void notification(const std::string& method, const Json& p) {
        const auto uri = [&] { return p.contains("textDocument") ? p["textDocument"].value("uri", std::string {}) : std::string {}; };
        if (method == "textDocument/didOpen") service_->open(uri(), p["textDocument"].value("text", std::string {}), p["textDocument"].value("version", 0));
        else if (method == "textDocument/didChange") service_->change(uri(), p.value("contentChanges", Json::array()), p["textDocument"].value("version", 0));
        else if (method == "textDocument/didClose") service_->close(uri());
        else if (method == "textDocument/didSave") service_->saved(uri());
        else if (method == "workspace/didChangeWatchedFiles")
            for (const auto& c : p.value("changes", Json::array())) service_->changed_on_disk(c.value("uri", std::string {}));
        // initialized, $/cancelRequest, $/setTrace and the like: nothing to do
    }

    std::expected<Json, lsp::Error> answer(const std::string& method, const Json& p) {
        if (method == "initialize") {
            Json result = Json::object();
            result["capabilities"] = lsp::Service::capabilities();
            Json info = Json::object();
            info["name"] = "mcxx";
            info["version"] = "0.1.0";
            result["serverInfo"] = std::move(info);
            Json mc6 = Json::object();
            mc6["version"] = 1;
            mc6["requests"] = Json::array({ "mcxx/setCommands", "mcxx/facts", "mcxx/gates", "mcxx/catalog" });
            result["capabilities"]["experimental"]["mcxx"] = std::move(mc6);
            return result;
        }
        if (method == "shutdown") {
            shutdown_ = true;
            return Json(nullptr);
        }
        if (method == "mcxx/setCommands") {
            workspace_->set_commands(commands_from(p.value("commands", Json::array())));
            service_->refresh();
            return Json(nullptr);
        }
        if (method == "mcxx/catalog") return Json::parse(features::catalog_json(*plugin::catalog()));
        if (method == "mcxx/facts" || method == "mcxx/gates") {
            const std::string uri { p.contains("textDocument") ? p["textDocument"].value("uri", std::string {}) : std::string {} };
            if (uri.empty()) return error(-32602, "a textDocument with a uri is required");
            const auto unit = service_->unit(uri, true);
            const std::string path { lsp::uri_to_path(uri) };
            if (method == "mcxx/facts") {
                if (!unit) return error(-32602, "the document is not open");
                return plugin::wire::facts_to_json(unit->facts(), path, unit->module_name());
            }
            return gates(path, unit ? std::string { unit->module_name() } : std::string {});
        }
        return service_->request(method, p);
    }

    // What gates the file: the configuration it is under, and each feature's level there (MC6 §4).
    static Json gates(const std::string& path, const std::string& module) {
        const auto plan = features::plan_for(path);
        Json result = Json::object();
        result["manifest"] = plan->config.manifest;
        result["module"] = module;
        result["profiles"] = plan->config.profiles;
        result["problems"] = plan->problems;
        Json list = Json::array();
        for (const auto& g : plan->gates) {
            Json f = Json::object();
            f["id"] = g.entry->feature->id;
            f["category"] = std::string { plugin::to_string(g.entry->feature->category) };
            f["level"] = std::string { plugin::to_string(plan->level(g, module, "")) };
            f["gated"] = g.maybe;
            list.push_back(std::move(f));
        }
        result["features"] = std::move(list);
        return result;
    }
};

} // namespace

int run(std::istream& in, std::ostream& out, Options options) {
    Server server { out, std::move(options) };
    while (auto message = read_message(in))
        if (!server.handle(*message)) return server.shut_down() ? 0 : 1;
    return server.shut_down() ? 0 : 1;
}

} // namespace mcxx::serve
