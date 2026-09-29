// mcxx.backend.clang partition :store: module interfaces (BMIs) built in dependency order, cached by content
// The class; its members are defined in store.cpp (MC5 §8).
module;

#include <llvm/Support/thread.h>

#include <condition_variable>
#include <deque>
#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <stop_token>
#include <string>
#include <vector>

module mcxx.backend.clang:store;

import mcxx.msa;
import mcxx.graph;
import mcxx.base;
import :support;
import :ifc;

namespace mcxx::clang_backend {

namespace fs = std::filesystem;
using msa::Position;
using msa::Range;
using msa::Location;

class ModuleStore {
public:
    struct Result {
        std::map<std::string, std::string> pcms;   // module -> interface file, dependencies first
        std::vector<std::string> failed;           // modules that could not be built (or depend on one)
        std::vector<std::string> missing;          // imported, provided by no unit
    };

    ModuleStore(std::string cacheDirectory, std::string resourceDirectory, unsigned workers, std::function<void()> changed);

    ~ModuleStore();

    // The program as the graph and the commands describe it now. Interfaces whose inputs changed are
    // rebuilt the next time they are required.
    void set_program(const mcxx::graph::Graph& graph, const std::map<std::string, msa::Command, std::less<>>& commands);

    // The editor's text of an open file, read in place of the disk's; nullopt: the disk's again. An
    // interface whose text changed is stale, and so is every interface that imports it.
    void set_buffer(const std::string& path, std::optional<std::string> text);

    void file_changed(const std::string& path);

    // Builds (or finds) the interfaces of `roots` and everything they import, and waits for them.
    Result require(const std::vector<std::string>& roots, std::stop_token cancel = {});

    // Every module the program provides, built in the background.
    void prepare_all();

    msa::Status status() const;

    std::string pcm_of(const std::string& module) const;

private:
    enum class State { unknown, stale, queued, building, ready, failed };
    struct Entry {
        State state { State::unknown };
        std::string pcm;
        std::string key;
        std::string error;
        std::string cause;                 // failed: the module whose own build failed
        bool command { false };            // failed: the compile command was rejected
        bool again { false };              // building: its input changed meanwhile, build it again
        std::string source;
        std::vector<std::string> inputs;   // files the last build read
    };

    std::string cacheDirectory_;
    std::string resourceDirectory_;
    std::function<void()> changed_;
    mutable std::mutex mutex_;
    std::condition_variable_any cv_;
    mcxx::graph::Graph graph_;
    std::map<std::string, msa::Command, std::less<>> commands_;
    std::map<std::string, Entry, std::less<>> entries_;
    std::map<std::string, std::string, std::less<>> buffers_;   // path -> the editor's text
    std::deque<std::string> ready_;   // scheduled modules whose dependencies are all done
    std::vector<std::string> waiting_;
    bool stopping_ { false };
    std::stop_source stop_;
    std::vector<std::unique_ptr<llvm::thread>> workers_;

    void schedule_(const std::string& module);

    // Moves queued modules whose dependencies are done to the ready queue. Holds mutex_.
    void dispatch_();

    void work_(std::stop_token stop);

    void build_(const std::string& module);

    static std::optional<std::vector<std::string>> valid_inputs_(const std::string& manifest);
};

// Defined here rather than in store.cpp, for the one reason: with Clang 22, libc++'s lock inside
// std::stop_source (`__atomic_unique_lock::__set_locked_bit`, a static constexpr member of a class
// template) reached through this partition's global module fragment is emitted by no unit that
// imports it -- store.cpp's and workspace.cpp's objects name it and none defined it once these
// bodies moved. The partition's own object, where they use stop_, emits it.
ModuleStore::ModuleStore(std::string cacheDirectory, std::string resourceDirectory, unsigned workers, std::function<void()> changed)
    : cacheDirectory_ { std::move(cacheDirectory) }, resourceDirectory_ { std::move(resourceDirectory) },
      changed_ { std::move(changed) } {
    std::error_code ec;
    fs::create_directories(cacheDirectory_ + "/modules", ec);
    for (unsigned i { 0 }; i < std::max(1u, workers); ++i)
        workers_.push_back(clang_thread([this] { work_(stop_.get_token()); }));
}

ModuleStore::~ModuleStore() {
    {
        std::lock_guard lock { mutex_ };
        stopping_ = true;
    }
    stop_.request_stop();
    cv_.notify_all();
    for (auto& w : workers_) w->join();
    workers_.clear();
}

} // namespace mcxx::clang_backend

