// mcxx serve (MC6 v1, specs/mc6-serve.md): the semantic service as a process.
//
// The transport is LSP's base protocol (Content-Length framing, JSON-RPC 2.0) on standard input and
// output. LSP's requests go to mcxx.lsp's Service; MC++'s own are:
//
//   mcxx/setCommands   {commands: [{directory, file, arguments}]}   the program's compile commands
//   mcxx/facts         {textDocument: {uri}}                         the document's MC3 facts
//   mcxx/gates         {textDocument: {uri}}                         its gates: configuration, levels
//   mcxx/catalog       {}                                            MC1's catalog
//
// Client is the host's side: it starts `mcxx serve`, and when the process dies it starts it again and
// replays the commands and the open documents, so a crash costs the host a request, never itself.
export module mcxx.serve;

import std;
import nlohmann.json;
import mcxx.msa;

export namespace mcxx::serve {

using Json = nlohmann::json;

// LSP's base protocol: a message is `Content-Length: N\r\n\r\n` and N bytes of JSON.
std::string frame(const Json& message);
std::optional<Json> read_message(std::istream& in);   // nothing at the end of input or on a broken header

struct Options {
    std::string database;     // a directory with compile_commands.json, or "" (mcxx/setCommands gives them)
    std::string resource;     // Clang's resource directory ("" : beside the program, lib/clang/<major>)
    std::string cache;        // "" : ~/.cache/mcxx/serve
    unsigned workers { 0 };   // 0 : half the cores
};

// Serves until `exit`: 0 when `shutdown` came first, 1 otherwise (LSP's rule).
int run(std::istream& in, std::ostream& out, Options options);

// A build database's commands: `path` a compile_commands.json or the directory that holds one.
std::vector<msa::Command> read_database(const std::string& path);

class Client {
public:
    using Notify = std::function<void(std::string_view method, const Json& params)>;

    Client(std::vector<std::string> command, Notify notify, std::chrono::milliseconds timeout = std::chrono::seconds { 60 });
    ~Client();
    Client(const Client&) = delete;
    Client& operator=(const Client&) = delete;

    // A request and its result, or why there is none (an error answer, a timeout, the process gone).
    // A process that is gone is started again first.
    std::expected<Json, std::string> request(std::string method, Json params);
    // A notification. didOpen, didChange (full text) and didClose are remembered, as are the last
    // mcxx/setCommands, and replayed to a process started again.
    void notify(std::string method, Json params);

    int restarts() const;
    int pid() const;

private:
    struct State;
    std::unique_ptr<State> state_;
};

} // namespace mcxx::serve
