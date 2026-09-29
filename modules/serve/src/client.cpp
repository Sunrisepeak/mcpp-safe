// mcxx.serve: the host's side (A1.3.2). One `mcxx serve` process at a time: a reader thread takes its
// messages -- answers to the waiting requests, notifications to the host -- and when the process is
// gone, the next call starts another, shakes hands, and replays the commands and the open documents.
module mcxx.serve;

import std;
import nlohmann.json;
import mcxx.base;
import mcxx.plugin.host;

namespace mcxx::serve {

namespace {

using Clock = std::chrono::steady_clock;

struct Pending {
    std::optional<Json> answer;
    std::string failure;
    bool done { false };
};

} // namespace

struct Client::State {
    std::vector<std::string> command;
    Notify notify;
    std::chrono::milliseconds timeout;

    std::mutex mutex;
    std::condition_variable answered;
    std::optional<plugin::host::Process> process;
    std::thread reader;
    bool alive { false };
    std::int64_t next { 1 };
    std::map<std::int64_t, std::shared_ptr<Pending>> pending;
    int restarts { -1 };   // the first start is not a restart
    int pid { -1 };

    // What a new process is told again.
    std::optional<Json> commands;
    std::map<std::string, Json, std::less<>> open;   // uri -> the didOpen params, with the latest text

    ~State() { stop(); }

    void stop() {
        {
            std::lock_guard lock { mutex };
            if (process) process->kill();
        }
        if (reader.joinable()) reader.join();
        std::lock_guard lock { mutex };
        process.reset();
        alive = false;
    }

    bool write(const Json& message) {   // with mutex held
        return process && process->write(frame(message));
    }

    void read_loop() {
        for (;;) {
            std::optional<Json> message;
            std::string gone;
            {
                // The process is only read here; its descriptors outlive this loop (stop joins first).
                auto* p = &*process;
                const auto far = Clock::now() + std::chrono::hours { 24 * 365 };
                std::size_t length { 0 };
                for (;;) {
                    auto line = p->read_line(far);
                    if (!line) {
                        gone = line.error();
                        break;
                    }
                    std::string_view l { *line };
                    if (l.ends_with('\r')) l.remove_suffix(1);
                    if (l.empty()) {
                        auto body = p->read_exactly(length, far);
                        if (!body) gone = body.error();
                        else message = Json::parse(*body, nullptr, false);
                        break;
                    }
                    if (l.starts_with("Content-Length:")) length = std::stoul(std::string { l.substr(15) });
                }
            }
            if (!gone.empty()) {
                std::lock_guard lock { mutex };
                alive = false;
                for (auto& [id, waiting] : pending) {
                    waiting->failure = "the server stopped: " + gone;
                    waiting->done = true;
                }
                pending.clear();
                answered.notify_all();
                base::trace::info("serve", "mcxx serve (pid {}) stopped: {}", pid, gone);
                return;
            }
            if (!message || message->is_discarded()) continue;
            if (message->contains("id") && !message->contains("method")) {
                std::lock_guard lock { mutex };
                const auto it = pending.find((*message)["id"].get<std::int64_t>());
                if (it == pending.end()) continue;
                if (message->contains("error")) it->second->failure = (*message)["error"].value("message", std::string { "an error" });
                else it->second->answer = message->value("result", Json(nullptr));
                it->second->done = true;
                pending.erase(it);
                answered.notify_all();
            } else if (message->contains("method") && notify) {
                notify((*message)["method"].get<std::string>(), message->value("params", Json::object()));
            }
        }
    }

    // Starts the process if there is none, shakes hands, replays. With mutex held; false when it
    // cannot be started.
    bool ensure(std::unique_lock<std::mutex>& lock, std::string& why) {
        if (alive) return true;
        lock.unlock();
        if (reader.joinable()) reader.join();
        lock.lock();
        if (alive) return true;
        process.reset();
        auto started = plugin::host::Process::start(command, "", base::trace::enabled("serve", base::trace::Level::debug));
        if (!started) {
            why = started.error();
            return false;
        }
        process.emplace(std::move(*started));
        pid = process->pid();
        alive = true;
        ++restarts;
        reader = std::thread { [this] { read_loop(); } };
        // The handshake, then what the host had said.
        Json init = Json::object();
        init["processId"] = nullptr;
        init["rootUri"] = nullptr;
        init["capabilities"] = Json::object();
        auto result = call(lock, "initialize", init);
        if (!result) {
            why = result.error();
            return false;
        }
        say("initialized", Json::object());
        if (commands) (void)call(lock, "mcxx/setCommands", *commands);
        for (const auto& [uri, params] : open) say("textDocument/didOpen", params);
        base::trace::info("serve", "mcxx serve started (pid {}), {} documents reopened", pid, open.size());
        return true;
    }

    void say(const std::string& method, const Json& params) {   // with mutex held
        Json message = Json::object();
        message["jsonrpc"] = "2.0";
        message["method"] = method;
        message["params"] = params;
        write(message);
    }

    std::expected<Json, std::string> call(std::unique_lock<std::mutex>& lock, const std::string& method, const Json& params) {
        const std::int64_t id { next++ };
        auto waiting = std::make_shared<Pending>();
        pending.emplace(id, waiting);
        Json message = Json::object();
        message["jsonrpc"] = "2.0";
        message["id"] = id;
        message["method"] = method;
        message["params"] = params;
        if (!write(message)) {
            pending.erase(id);
            return std::unexpected("the server does not read its input");
        }
        if (!answered.wait_until(lock, Clock::now() + timeout, [&] { return waiting->done; })) {
            pending.erase(id);
            return std::unexpected(std::format("{} did not answer in {} ms", method, timeout.count()));
        }
        if (!waiting->failure.empty()) return std::unexpected(waiting->failure);
        return *waiting->answer;
    }
};

Client::Client(std::vector<std::string> command, Notify notify, std::chrono::milliseconds timeout) : state_ { std::make_unique<State>() } {
    state_->command = std::move(command);
    state_->notify = std::move(notify);
    state_->timeout = timeout;
}

Client::~Client() {
    if (!state_) return;
    std::unique_lock lock { state_->mutex };
    if (state_->alive) {
        (void)state_->call(lock, "shutdown", Json(nullptr));
        state_->say("exit", Json::object());
    }
}

std::expected<Json, std::string> Client::request(std::string method, Json params) {
    std::unique_lock lock { state_->mutex };
    std::string why;
    if (!state_->ensure(lock, why)) return std::unexpected("cannot start mcxx serve: " + why);
    if (method == "mcxx/setCommands") state_->commands = params;
    return state_->call(lock, method, params);
}

void Client::notify(std::string method, Json params) {
    std::unique_lock lock { state_->mutex };
    const std::string uri { params.contains("textDocument") ? params["textDocument"].value("uri", std::string {}) : std::string {} };
    if (method == "textDocument/didOpen") state_->open[uri] = params;
    else if (method == "textDocument/didClose") state_->open.erase(uri);
    else if (method == "textDocument/didChange") {
        const auto it = state_->open.find(uri);
        const auto& changes = params.value("contentChanges", Json::array());
        if (it != state_->open.end() && !changes.empty() && !changes.back().contains("range")) {
            it->second["textDocument"]["text"] = changes.back().value("text", std::string {});
            it->second["textDocument"]["version"] = params["textDocument"].value("version", 0);
        }
    }
    std::string why;
    if (!state_->ensure(lock, why)) return;   // replayed when it can be started
    state_->say(method, params);
}

int Client::restarts() const { return std::max(0, state_->restarts); }

int Client::pid() const { return state_->pid; }

} // namespace mcxx::serve
