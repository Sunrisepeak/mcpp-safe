// An LSP-shaped semantic service over MSA (spec MC6): documents in, LSP results and notifications
// out, as JSON. It owns no transport -- an editor engine (mcppls), a test driver or a server loop
// feeds it messages -- and no backend: any msa::Workspace serves it.
//
//   mcxx::lsp::Service service { workspace, {}, [](std::string_view method, Json params) { ... } };
//   service.open(uri, text, 1);
//   auto result = service.request("textDocument/hover", params);
//
// Requests may come from any thread. Parsing happens on the service's own threads; every parse
// publishes `textDocument/publishDiagnostics` through the notify callback.
export module mcxx.lsp;

import std;
import nlohmann.json;
import mcxx.msa;

export namespace mcxx::lsp {

using Json = nlohmann::json;

struct Error {
    int code { -32603 };
    std::string message;
};

using Result = std::expected<Json, Error>;

struct Options {
    unsigned parse_workers { 2 };
    std::chrono::milliseconds debounce { 150 };
    std::chrono::milliseconds wait { 60000 };   // how long a request waits for the current version's parse
};

class Service {
public:
    using Notify = std::function<void(std::string_view method, Json params)>;

    Service(msa::Workspace& workspace, Options options, Notify notify);
    ~Service();
    Service(const Service&) = delete;
    Service& operator=(const Service&) = delete;

    // The `capabilities` object of an initialize result.
    static Json capabilities();
    // The semantic token legend capabilities() declares.
    static Json legend();

    void open(const std::string& uri, std::string text, std::int64_t version);
    // `changes` is didChange's contentChanges (full or incremental).
    void change(const std::string& uri, const Json& changes, std::int64_t version);
    void close(const std::string& uri);
    void saved(const std::string& uri);
    // A file changed on disk that is not open (workspace/didChangeWatchedFiles).
    void changed_on_disk(const std::string& uri);
    // Parses every open document again: the program was described again (its commands changed).
    void refresh();

    // Answers one request; blocks until it has an answer or `cancel` is requested.
    Result request(std::string_view method, const Json& params, std::stop_token cancel = {});

    // The latest parse of an open document, waiting for the current version when asked to.
    std::shared_ptr<const msa::Unit> unit(const std::string& uri, bool current, std::stop_token cancel = {});

private:
    struct Document;
    struct State;
    std::unique_ptr<State> state_;
};

// file: URIs <-> paths, and LSP (UTF-16) <-> MSA (UTF-8 byte) columns.
std::string uri_to_path(std::string_view uri);
std::string path_to_uri(std::string_view path);
Json to_lsp(const msa::Range& range, std::string_view text);
msa::Position from_lsp(const Json& position, std::string_view text);

} // namespace mcxx::lsp
