// mcxx.backend.clang partition :support: Clang on its own stack (ClangPool), a build's arguments normalized, shared constants
// Declarations; the definitions are in support.cpp (MC5 §8).
module;

#include <clang/Basic/Diagnostic.h>
#include <llvm/Support/thread.h>

#include <condition_variable>
#include <cstddef>
#include <deque>
#include <filesystem>
#include <functional>
#include <future>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

module mcxx.backend.clang:support;

import mcxx.msa;
import mcxx.graph;
import mcxx.base;

namespace mcxx::clang_backend {

namespace cl = ::clang;
namespace fs = std::filesystem;
using msa::Position;
using msa::Range;
using msa::Location;

inline constexpr std::string_view CLANG_VERSION { "23.1.0" };
// Bumped whenever what a cached interface depends on changes in this backend.
inline constexpr std::string_view CACHE_EPOCH { "mcxx.clang/2" };   // 2: reduced interfaces

// Every thread that runs Clang does so on a stack of its own of 16 MiB.
//
// openkal gives every thread a fixed 256 KiB stack (openkal-linux task.cpp kStack; the size a
// pthread attribute asks for is not used) and maps no guard page, so deep template or module code
// overflows silently into a neighbouring thread's descriptor. Until openkal takes a stack size, a
// thread switches to a stack this backend allocates before any Clang code runs on it.
// mcxx_call_on_stack and whether it switches stacks come from the target's architecture package
// (mcxx.arch, through mcxx.base): the one assembly routine, where platform code belongs (plan P9).
inline constexpr bool STACK_SWITCH { mcxx::arch::STACK_SWITCH };

inline constexpr std::size_t CLANG_STACK { std::size_t { 16 } << 20 };

// Clang measures its stack from the bottom a thread noted, and past DesiredStackSize (8 MiB) minus
// 256 KiB moves the rest of the work to a new thread "of 8 MiB" -- which openkal makes 256 KiB,
// where the work overflows. A distance larger than DesiredStackSize reads to Clang as a stack it
// does not understand, and it then never moves. So the bottom is noted once, from a small stack
// placed 9 MiB above the real one, before any Clang code runs on the thread; Clang's own later
// notes (FrontendAction, CompilerInstance) do not overwrite a bottom already noted.
inline constexpr std::size_t NOTE_DISTANCE { std::size_t { 9 } << 20 };
inline constexpr std::size_t NOTE_STACK { std::size_t { 64 } << 10 };

inline thread_local bool on_clang_stack { false };

// Set while this thread builds a module interface or indexes in the background: MC++'s feature
// gates (:gate) do not run there. A gate's error is the file's own diagnostic; in an interface
// built for the editor it would fail the BMI and every importer with it.
inline thread_local bool gates_suppressed { false };

// Runs `body` on a Clang stack of this thread's own (above).
void run_on_clang_stack(const std::function<void()>& body);
// A thread that runs `body` so.
std::unique_ptr<llvm::thread> clang_thread(std::function<void()> body);

// A pool of such threads for work a caller waits on.
class ClangPool {
public:
    explicit ClangPool(unsigned size);
    ~ClangPool();
    // Runs `f` on one of the pool's threads and waits; inline when the caller already runs on a
    // Clang stack (so a pool thread never waits on its own pool).
    template <class F>
    auto run(F&& f) -> decltype(f()) {
        if (on_clang_stack) return f();
        using R = decltype(f());
        std::packaged_task<R()> task { std::forward<F>(f) };
        auto result = task.get_future();
        {
            std::lock_guard lock { mutex_ };
            queue_.push_back([&task] { task(); });
        }
        cv_.notify_one();
        return result.get();
    }

private:
    std::mutex mutex_;
    std::condition_variable cv_;
    std::deque<std::function<void()>> queue_;
    bool stopping_ { false };
    std::vector<std::unique_ptr<llvm::thread>> threads_;
    void loop_();
};

// ============================================================================================
// 1. commands
// ============================================================================================

// Paths by mcxx.base's rules, not std::filesystem's: beneath this program is a POSIX C library on every
// system, to which `C:\\Users\\x\\a.cpp` is a relative name; mcxx.base knows it for an absolute one
// on Windows, and keeps every path with '/' separators and its drive.
std::string normalize_path(std::string_view path, std::string_view base = {});
// `path` made absolute against the working directory, when it is not.
std::string absolute_path(std::string_view path);
bool is_source_argument(std::string_view arg, const msa::Command& command);
// Whether a command's compiler is GCC, by its program's name (g++, gcc, x86_64-linux-gnu-g++-16, ...).
bool gcc_command(const msa::Command& command);
// The build's arguments with everything this backend decides for itself taken out: where outputs
// go, which interfaces are read, the input language, the resource directory, and the source.
// What remains describes the program and is what a cached interface is keyed on.
std::vector<std::string> normalize(const msa::Command& command);
// A command for a file the build does not list: the listed command of the nearest file (longest
// common directory, then the same extension), with the file swapped in.
std::optional<msa::Command> infer_command(const std::map<std::string, msa::Command, std::less<>>& commands, const std::string& file);
std::string hex_digest(std::string_view data);
std::optional<std::string> read_file(const std::string& path);
bool write_file_atomic(const std::string& path, std::string_view data);
// Arguments the backend adds to every compile: its own builtin headers, and every comment kept
// (hover shows plain `//` comments too).
std::vector<std::string> backend_arguments(const std::string& resourceDirectory);

// Diagnostics of a compile, kept as text: a module interface that fails is reported by module.
class CollectingConsumer : public cl::DiagnosticConsumer {
public:
    std::vector<std::string> errors;
    void HandleDiagnostic(cl::DiagnosticsEngine::Level level, const cl::Diagnostic& info) override;
};

} // namespace mcxx::clang_backend
