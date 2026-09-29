// A plugin's process: started with its standard streams as pipes, read with a deadline, killed.
module;

#include <cerrno>
#include <csignal>
#include <fcntl.h>
#include <poll.h>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>

module mcxx.plugin.host;

import std;
import nlohmann.json;
import mcxx.base;
import mcxx.os;
import mcxx.msa;
import mcxx.plugin;
import mcxx.plugin.wire;
import mcxx.features;

namespace mcxx::plugin::host {

namespace {

void ignore_sigpipe() {
    static std::once_flag once;
    std::call_once(once, [] { std::signal(SIGPIPE, SIG_IGN); });   // a dead plugin's pipe is an error, not a signal
}

} // namespace

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
    // Its standard error, when not shown: the null device. openkal's Windows target has none to open by
    // name and refuses to close a started program's standard streams (ENOSYS), so there it is ours.
    if constexpr (mcxx::os::FAMILY != mcxx::os::Family::windows)
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

} // namespace mcxx::plugin::host
