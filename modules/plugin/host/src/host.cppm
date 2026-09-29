// The host side of MC++'s out-of-process plugins (specs/mc4-plugins.md §3, §5, §6).
//
// load(config) starts every out-of-process plugin a package declares ([package.metadata.mcxx.plugins],
// entries with `command`), once per process, shakes hands, and registers each provider it describes
// as a proxy in the catalog: a rule proxy sends the file's facts and gets findings back, a filter
// proxy sends the text. A plugin that crashes, does not answer within its time limit, or answers
// what MC4 does not allow is killed and reported (plugin::Context::fail), never a crash or a hang
// of the compiler. A proxy answers only for files of the package that declared it.
export module mcxx.plugin.host;

import std;
import nlohmann.json;
import mcxx.base;
import mcxx.os;
import mcxx.msa;
import mcxx.plugin;
import mcxx.plugin.wire;
import mcxx.features;

export namespace mcxx::plugin::host {

// Starts and registers the out-of-process plugins `config` declares, and loads its plugin libraries,
// that are not running yet. Returns what could not be started or loaded, one line each, for the
// gates to report at the file (MC4-5-5): such a plugin gates what nobody knows, so its package's
// files fail.
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

// A plugin library (MC4 §3, `library`) loaded into this process, its SDK checked before what it
// registered is taken (MC4-3-7): nothing when it is, else why not (library.cpp).
std::string load_library(std::string_view name, const std::filesystem::path& file);

} // namespace mcxx::plugin::host
