// The host side of MC++'s out-of-process plugins (specs/mc4-plugins.md §3, §5, §6).
//
// load(config) starts every out-of-process plugin a package declares ([package.metadata.mcxx.plugins],
// entries with `command`), once per process, shakes hands, and registers each provider it describes
// as a proxy in the catalog: a rule proxy sends the file's facts and gets findings back, a filter
// proxy sends the text. A plugin that crashes, does not answer within its time limit, or answers
// what MC4 does not allow is killed and reported (plugin::Context::fail), never a crash or a hang
// of the compiler. A proxy answers only for files of the package that declared it.
module;

#include <cerrno>
#include <csignal>
#include <fcntl.h>
#include <poll.h>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>

extern "C" char** environ;

export module mcxx.plugin.host;

import std;
import nlohmann.json;
import mcxx.base;
import mcxx.msa;
import mcxx.plugin;
import mcxx.plugin.wire;
import mcxx.features;

export namespace mcxx::plugin::host {

// Starts and registers the out-of-process plugins `config` declares that are not running yet.
// Returns what could not be started, one line each, for the gates to report at the file (MC4-5-5):
// such a plugin gates what nobody knows, so its package's files fail.
std::vector<std::string> load(const features::Config& config);

// The static plugins (package names) the running compiler was composed with (mcxx compose).
void set_composed(std::vector<std::string> packages);
const std::vector<std::string>& composed();

// A child process spoken to by lines: exposed for tests.
class Process {
public:
    // Started in `directory` ("" = this process's), with its standard error shown or discarded.
    static std::expected<Process, std::string> start(const std::vector<std::string>& command, const std::string& directory, bool show_errors);
    Process(Process&& other) noexcept;
    Process& operator=(Process&&) = delete;
    ~Process();

    bool write(std::string_view data);
    // The next line (without its line break), or why there is none: it did not come by the
    // deadline, the process closed its output, or it exited.
    std::expected<std::string, std::string> read_line(std::chrono::steady_clock::time_point deadline);
    // Exactly `size` bytes (a framed message's body), or why not, as read_line.
    std::expected<std::string, std::string> read_exactly(std::size_t size, std::chrono::steady_clock::time_point deadline);
    int pid() const { return pid_; }
    void kill();

private:
    Process() = default;
    int pid_ { -1 };
    int in_ { -1 };    // its standard input: we write
    int out_ { -1 };   // its standard output: we read
    std::string buffer_;
    std::string exit_description();
};

// One plugin process and its protocol state.
class Session {
public:
    Session(std::string name, std::vector<std::string> command, std::string directory, std::chrono::milliseconds timeout);
    // Starts the process and shakes hands; the providers it describes, or why not.
    std::expected<std::vector<wire::ProviderInfo>, std::string> start();
    // Sends a request (its id is set here) and waits for the answer with that id; an `error`
    // answer, a timeout, a crash or anything MC4 does not allow is the returned error. After a
    // failure other than an `error` answer, the process is gone and every later request fails.
    std::expected<wire::Json, std::string> request(wire::Json message);
    void shutdown();
    const std::string& name() const { return name_; }

private:
    std::string name_;
    std::vector<std::string> command_;
    std::string directory_;
    std::chrono::milliseconds timeout_;
    std::mutex mutex_;
    std::optional<Process> process_;
    std::string dead_;   // why it is gone
    std::int64_t next_id_ { 1 };

    std::expected<wire::Json, std::string> exchange(const wire::Json& message, std::int64_t id);
    std::string fail(std::string why);
};

} // namespace mcxx::plugin::host

namespace mcxx::plugin::host {

namespace {

using wire::Json;
namespace fs = std::filesystem;

void ignore_sigpipe() {
    static std::once_flag once;
    std::call_once(once, [] { std::signal(SIGPIPE, SIG_IGN); });   // a dead plugin's pipe is an error, not a signal
}

} // namespace

// ---- Process ----------------------------------------------------------------------------------

std::expected<Process, std::string> Process::start(const std::vector<std::string>& command, const std::string& directory, bool show_errors) {
    if (command.empty()) return std::unexpected("no command");
    ignore_sigpipe();
    int to_child[2], from_child[2];
    if (::pipe2(to_child, O_CLOEXEC) != 0) return std::unexpected(std::format("pipe: {}", std::strerror(errno)));
    if (::pipe2(from_child, O_CLOEXEC) != 0) {
        ::close(to_child[0]);
        ::close(to_child[1]);
        return std::unexpected(std::format("pipe: {}", std::strerror(errno)));
    }
    posix_spawn_file_actions_t actions;
    posix_spawn_file_actions_init(&actions);
    posix_spawn_file_actions_adddup2(&actions, to_child[0], 0);
    posix_spawn_file_actions_adddup2(&actions, from_child[1], 1);
    if (!show_errors) posix_spawn_file_actions_addopen(&actions, 2, "/dev/null", O_WRONLY, 0);
    if (!directory.empty()) posix_spawn_file_actions_addchdir_np(&actions, directory.c_str());
    std::vector<char*> argv;
    for (const auto& a : command) argv.push_back(const_cast<char*>(a.c_str()));
    argv.push_back(nullptr);
    pid_t pid { -1 };
    const int rc { ::posix_spawn(&pid, argv[0], &actions, nullptr, argv.data(), environ) };
    posix_spawn_file_actions_destroy(&actions);
    ::close(to_child[0]);
    ::close(from_child[1]);
    if (rc != 0) {
        ::close(to_child[1]);
        ::close(from_child[0]);
        return std::unexpected(std::format("cannot start {}: {}", command[0], std::strerror(rc)));
    }
    Process p;
    p.pid_ = pid;
    p.in_ = to_child[1];
    p.out_ = from_child[0];
    return p;
}

Process::Process(Process&& other) noexcept
    : pid_ { std::exchange(other.pid_, -1) }, in_ { std::exchange(other.in_, -1) }, out_ { std::exchange(other.out_, -1) },
      buffer_ { std::move(other.buffer_) } {}

Process::~Process() { kill(); }

bool Process::write(std::string_view data) {
    while (!data.empty() && in_ >= 0) {
        const ssize_t n { ::write(in_, data.data(), data.size()) };
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) return false;
        data.remove_prefix(static_cast<std::size_t>(n));
    }
    return in_ >= 0;
}

std::string Process::exit_description() {
    if (pid_ < 0) return "it is not running";
    int status { 0 };
    // Give an exiting process a moment to be reaped, so the report says how it ended.
    for (int i { 0 }; i < 20; ++i) {
        const pid_t r { ::waitpid(pid_, &status, WNOHANG) };
        if (r == pid_) {
            pid_ = -1;
            if (WIFEXITED(status)) return std::format("it exited with status {}", WEXITSTATUS(status));
            if (WIFSIGNALED(status)) return std::format("it was killed by signal {}", WTERMSIG(status));
            return "it ended";
        }
        std::this_thread::sleep_for(std::chrono::milliseconds { 5 });
    }
    return "it closed its output";
}

std::expected<std::string, std::string> Process::read_line(std::chrono::steady_clock::time_point deadline) {
    for (;;) {
        if (const auto nl = buffer_.find('\n'); nl != std::string::npos) {
            std::string line { buffer_.substr(0, nl) };
            buffer_.erase(0, nl + 1);
            return line;
        }
        if (out_ < 0) return std::unexpected("it is not running");
        const auto left = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - std::chrono::steady_clock::now()).count();
        if (left <= 0) return std::unexpected("it did not answer in time");
        pollfd fd { out_, POLLIN, 0 };
        const int ready { ::poll(&fd, 1, static_cast<int>(std::min<long long>(left, 1000 * 60))) };
        if (ready < 0 && errno == EINTR) continue;
        if (ready == 0) continue;   // the deadline is checked above
        char chunk[65536];
        const ssize_t n { ::read(out_, chunk, sizeof chunk) };
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) return std::unexpected(exit_description());
        buffer_.append(chunk, static_cast<std::size_t>(n));
    }
}

std::expected<std::string, std::string> Process::read_exactly(std::size_t size, std::chrono::steady_clock::time_point deadline) {
    for (;;) {
        if (buffer_.size() >= size) {
            std::string out { buffer_.substr(0, size) };
            buffer_.erase(0, size);
            return out;
        }
        if (out_ < 0) return std::unexpected("it is not running");
        const auto left = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - std::chrono::steady_clock::now()).count();
        if (left <= 0) return std::unexpected("it did not answer in time");
        pollfd fd { out_, POLLIN, 0 };
        const int ready { ::poll(&fd, 1, static_cast<int>(std::min<long long>(left, 1000 * 60))) };
        if (ready < 0 && errno == EINTR) continue;
        if (ready == 0) continue;
        char chunk[65536];
        const ssize_t n { ::read(out_, chunk, sizeof chunk) };
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) return std::unexpected(exit_description());
        buffer_.append(chunk, static_cast<std::size_t>(n));
    }
}

void Process::kill() {
    if (in_ >= 0) ::close(std::exchange(in_, -1));
    if (out_ >= 0) ::close(std::exchange(out_, -1));
    if (pid_ > 0) {
        int status { 0 };
        if (::waitpid(pid_, &status, WNOHANG) == 0) {
            ::kill(pid_, SIGKILL);
            ::waitpid(pid_, &status, 0);
        }
        pid_ = -1;
    }
}

// ---- Session ----------------------------------------------------------------------------------

Session::Session(std::string name, std::vector<std::string> command, std::string directory, std::chrono::milliseconds timeout)
    : name_ { std::move(name) }, command_ { std::move(command) }, directory_ { std::move(directory) }, timeout_ { timeout } {}

std::string Session::fail(std::string why) {
    if (process_) process_->kill();
    process_.reset();
    dead_ = why;
    base::trace::info("plugins", "plugin {} failed: {}", name_, why);
    base::trace::count("plugins.failed");
    return why;
}

std::expected<Json, std::string> Session::exchange(const Json& message, std::int64_t id) {
    if (!process_) return std::unexpected(dead_.empty() ? std::string { "it is not running" } : dead_);
    if (!process_->write(wire::line(message))) {
        // It stopped reading: most likely it is gone, and the read says how.
        auto gone = process_->read_line(std::chrono::steady_clock::now() + std::chrono::milliseconds { 100 });
        return std::unexpected(fail(gone ? std::string { "it does not read its input" } : gone.error()));
    }
    const auto deadline = std::chrono::steady_clock::now() + timeout_;
    for (;;) {
        auto text = process_->read_line(deadline);
        if (!text) return std::unexpected(fail(text.error() == "it did not answer in time"
                                                   ? std::format("it did not answer in {} ms", timeout_.count())
                                                   : text.error()));
        auto answer = wire::message_from(*text);
        if (!answer) return std::unexpected(fail(std::format("it wrote what is not an MC4 message: {}", answer.error())));
        if (!answer->contains("id") || (*answer)["id"] != id) {
            if ((*answer)["type"] == "error") return std::unexpected(fail(std::format("it said: {}", answer->value("message", std::string {}))));
            return std::unexpected(fail("it answered a request it was not asked"));
        }
        return std::move(*answer);
    }
}

std::expected<std::vector<wire::ProviderInfo>, std::string> Session::start() {
    std::lock_guard lock { mutex_ };
    base::trace::Span span { "plugins", "start", name_ };
    auto process = Process::start(command_, directory_, base::trace::enabled("plugins", base::trace::Level::debug));
    if (!process) return std::unexpected(fail(process.error()));
    process_.emplace(std::move(*process));
    Json hello = Json::object();
    hello["type"] = "hello";
    hello["id"] = 0;
    hello["host"] = "mcxx 0.1.0";
    hello["protocols"] = Json::array({ wire::PROTOCOL });
    auto answer = exchange(hello, 0);
    if (!answer) return std::unexpected(answer.error());
    if ((*answer)["type"] == "error")
        return std::unexpected(fail(std::format("it speaks none of the protocols offered ({})", answer->value("message", std::string {}))));
    if ((*answer)["type"] != "welcome" || !answer->contains("protocol") || (*answer)["protocol"] != wire::PROTOCOL || !answer->contains("providers") ||
        !(*answer)["providers"].is_array())
        return std::unexpected(fail("its answer to hello is not a welcome with protocol 1 and its providers (MC4-6.2-1)"));
    std::vector<wire::ProviderInfo> providers;
    for (const auto& p : (*answer)["providers"]) {
        auto info = wire::provider_from(p);
        if (!info) return std::unexpected(fail(std::format("it describes a provider wrongly: {}", info.error())));
        providers.push_back(std::move(*info));
    }
    return providers;
}

std::expected<Json, std::string> Session::request(Json message) {
    std::lock_guard lock { mutex_ };
    const std::int64_t id { next_id_++ };
    message["id"] = id;
    auto answer = exchange(message, id);
    if (!answer) return answer;
    if ((*answer)["type"] == "error") return std::unexpected(std::format("it could not: {}", answer->value("message", std::string {})));
    return answer;
}

void Session::shutdown() {
    std::lock_guard lock { mutex_ };
    if (!process_) return;
    Json bye = Json::object();
    bye["type"] = "shutdown";
    bye["id"] = next_id_++;
    process_->write(wire::line(bye));
    // A moment to exit on its own, then it is killed (Process::kill).
    const auto until = std::chrono::steady_clock::now() + std::chrono::milliseconds { 200 };
    (void)process_->read_line(until);
    process_.reset();
}

// ---- proxies -----------------------------------------------------------------------------------

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
