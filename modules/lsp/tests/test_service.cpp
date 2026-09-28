// mcxx.lsp over a fake backend: the LSP layer's own logic -- UTF-16 positions, diagnostics pushed on
// parse, and each request's shape -- without Clang, in well under a second.
import std;
import nlohmann.json;
import mcxx.testing;
import mcxx.msa;
import mcxx.lsp;

namespace msa = mcxx::msa;
namespace lsp = mcxx::lsp;
using Json = nlohmann::json;

namespace {

msa::Range range(std::uint32_t line, std::uint32_t b, std::uint32_t e) { return { { line, b }, { line, e } }; }

// What a parse of one file yields, set up by each test case.
struct Scenario {
    std::vector<msa::Diagnostic> diagnostics;
    std::vector<msa::Occurrence> occurrences;
    std::vector<msa::Symbol> symbols;
    std::map<std::string, msa::Entity> entities;
    std::map<std::string, std::vector<msa::Location>> definitions, declarations, references;
};

class FakeUnit final : public msa::Unit {
public:
    FakeUnit(std::string path, std::string text, std::int64_t version, const Scenario& s)
        : path_ { std::move(path) }, text_ { std::move(text) }, version_ { version }, s_ { s } {}
    const std::string& path() const override { return path_; }
    std::int64_t version() const override { return version_; }
    std::string_view text() const override { return text_; }
    std::string_view module_name() const override { return {}; }
    std::span<const msa::Diagnostic> diagnostics() const override { return s_.diagnostics; }
    std::span<const msa::Occurrence> occurrences() const override { return s_.occurrences; }
    std::vector<msa::Symbol> symbols() const override { return s_.symbols; }
    std::optional<msa::Entity> entity_at(msa::Position at) const override {
        for (const auto& o : s_.occurrences)
            if (o.range.contains(at)) return entity(o.entity);
        return std::nullopt;
    }
    std::optional<msa::Entity> entity(std::string_view id) const override {
        const auto it = s_.entities.find(std::string { id });
        return it == s_.entities.end() ? std::nullopt : std::optional { it->second };
    }
    std::vector<msa::Location> overriders(std::string_view) const override { return {}; }

private:
    std::string path_;
    std::string text_;
    std::int64_t version_;
    const Scenario& s_;
};

class FakeWorkspace final : public msa::Workspace {
public:
    Scenario scenario;
    std::vector<std::string> closed;
    std::atomic<int> parses { 0 };

    void set_commands(std::vector<msa::Command>) override {}
    msa::Status status() const override { return {}; }
    std::shared_ptr<const msa::Unit> parse(const std::string& path, std::string text, std::int64_t version, msa::Cancel) override {
        ++parses;
        return std::make_shared<FakeUnit>(path, std::move(text), version, scenario);
    }
    std::vector<msa::CompletionItem> complete(const std::string&, const std::string&, msa::Position, msa::Cancel) override {
        return { { .label = "greet", .detail = "std::string", .insert_text = "greet", .kind = msa::Kind::function } };
    }
    msa::SignatureHelp signature_help(const std::string&, const std::string&, msa::Position, msa::Cancel) override { return {}; }
    std::vector<msa::Location> definitions(std::string_view id) const override { return lookup(scenario.definitions, id); }
    std::vector<msa::Location> declarations(std::string_view id) const override { return lookup(scenario.declarations, id); }
    std::vector<msa::Location> references(std::string_view id) const override { return lookup(scenario.references, id); }
    std::vector<msa::Found> find(std::string_view, std::size_t) const override { return {}; }
    void file_changed(const std::string&) override {}
    void close(const std::string& path) override { closed.push_back(path); }

private:
    static std::vector<msa::Location> lookup(const std::map<std::string, std::vector<msa::Location>>& m, std::string_view id) {
        const auto it = m.find(std::string { id });
        return it == m.end() ? std::vector<msa::Location> {} : it->second;
    }
};

Json at(std::string_view uri, std::uint32_t line, std::uint32_t character) {
    return Json { { "textDocument", Json { { "uri", uri } } }, { "position", Json { { "line", line }, { "character", character } } } };
}

} // namespace

int main() {
    using namespace mcxx::testing;
    const std::string path { "/w/src/main.cpp" };
    const std::string uri { lsp::path_to_uri(path) };

    "positions: UTF-16 on the wire, UTF-8 bytes inside"_test = [] {
        const std::string text { "int é = 1; // 中\n𝒳 y;\n" };
        // "é" is 2 bytes and 1 UTF-16 unit; "𝒳" is 4 bytes and 2 units.
        const msa::Position p { lsp::from_lsp(Json { { "line", 0 }, { "character", 6 } }, text) };
        expect(p.line == 0 && p.column == 7) << p.column;
        const msa::Position q { lsp::from_lsp(Json { { "line", 1 }, { "character", 3 } }, text) };
        expect(q.line == 1 && q.column == 5) << q.column;
        const Json r = lsp::to_lsp(range(1, 5, 6), text);
        expect(r["start"]["character"] == 3 && r["end"]["character"] == 4) << r.dump();
        expect(lsp::uri_to_path(lsp::path_to_uri("/a b/c.cpp")) == "/a b/c.cpp");
    };

    "a parse publishes its diagnostics, converted"_test = [&] {
        FakeWorkspace w;
        w.scenario.diagnostics.push_back({ range(0, 4, 5), msa::Severity::error, "bad", "undeclared_var_use", "Semantic Issue", {} });
        std::mutex m;
        std::condition_variable cv;
        std::vector<Json> published;
        lsp::Service service { w, { .parse_workers = 1, .debounce = std::chrono::milliseconds { 0 } }, [&](std::string_view method, Json params) {
            if (method != "textDocument/publishDiagnostics") return;
            std::lock_guard lock { m };
            published.push_back(std::move(params));
            cv.notify_all();
        } };
        service.open(uri, "int é;\n", 1);
        std::unique_lock lock { m };
        expect(fatal(cv.wait_for(lock, std::chrono::seconds { 5 }, [&] { return !published.empty(); })));
        const Json& d = published.front()["diagnostics"][0];
        expect(published.front()["uri"] == uri && published.front()["version"] == 1);
        expect(d["code"] == "undeclared_var_use" && d["severity"] == 1 && d["message"] == "bad");
        expect(d["range"]["start"]["character"] == 4 && d["range"]["end"]["character"] == 5) << "é is one UTF-16 unit";
    };

    "definition at a definition goes back to the declaration; hover names kind and value"_test = [&] {
        FakeWorkspace w;
        const msa::Location decl { "/w/src/greet.cppm", range(3, 11, 16) };
        const msa::Location def { path, range(0, 4, 9) };
        w.scenario.entities["c:@F@greet"] = msa::Entity { .id = "c:@F@greet", .name = "greet", .qualified_name = "greet", .kind = msa::Kind::variable,
                                                          .signature = "constexpr int greet = 42", .type = "const int", .value = "42",
                                                          .declaration = decl, .definition = def };
        w.scenario.occurrences.push_back({ range(0, 4, 9), "c:@F@greet", "greet", msa::Kind::variable, msa::role::definition });
        w.scenario.occurrences.push_back({ range(1, 7, 12), "c:@F@greet", "greet", msa::Kind::variable, msa::role::reference });
        lsp::Service service { w, { .parse_workers = 1, .debounce = std::chrono::milliseconds { 0 } }, {} };
        service.open(uri, "int greet = 42;\nreturn greet;\n", 1);
        const auto back = service.request("textDocument/definition", at(uri, 0, 5));
        expect(fatal(back.has_value()));
        expect(back->size() == 1 && (*back)[0]["uri"] == lsp::path_to_uri(decl.path)) << back->dump();
        const auto hover = service.request("textDocument/hover", at(uri, 1, 8));
        expect(fatal(hover.has_value()));
        const std::string md { (*hover)["contents"]["value"].get<std::string>() };
        expect(md.starts_with("### variable `greet`")) << md;
        expect(md.contains("Value = `42`")) << md;
        expect((*hover)["range"]["start"]["character"] == 7);
    };

    "references without declarations leave definitions out too"_test = [&] {
        FakeWorkspace w;
        const msa::Location decl { "/w/src/a.cppm", range(2, 0, 3) };
        const msa::Location def { "/w/src/a.cpp", range(4, 0, 3) };
        const msa::Location use { "/w/src/b.cpp", range(9, 2, 5) };
        w.scenario.entities["c:@F@f"] = msa::Entity { .id = "c:@F@f", .name = "f", .kind = msa::Kind::function };
        w.scenario.occurrences.push_back({ range(0, 0, 1), "c:@F@f", "f", msa::Kind::function, msa::role::reference | msa::role::call });
        w.scenario.references["c:@F@f"] = { decl, def, use };
        w.scenario.declarations["c:@F@f"] = { decl };
        w.scenario.definitions["c:@F@f"] = { def };
        lsp::Service service { w, { .parse_workers = 1, .debounce = std::chrono::milliseconds { 0 } }, {} };
        service.open(uri, "f();\n", 1);
        Json params = at(uri, 0, 0);
        params["context"] = Json { { "includeDeclaration", false } };
        const auto refs = service.request("textDocument/references", params);
        expect(fatal(refs.has_value()));
        std::vector<std::string> files;
        for (const auto& r : *refs) files.push_back(lsp::uri_to_path(r["uri"].get<std::string>()));
        expect(files == std::vector<std::string> { "/w/src/b.cpp", path }) << refs->dump();
    };

    "symbolInfo gives the id; outgoing calls group the calls of a body by callee"_test = [&] {
        FakeWorkspace w;
        const msa::Location fdef { path, range(0, 4, 5) };
        w.scenario.entities["c:@F@f"] = msa::Entity { .id = "c:@F@f", .name = "f", .container = "ns", .kind = msa::Kind::function, .definition = fdef };
        w.scenario.entities["c:@F@g"] = msa::Entity { .id = "c:@F@g", .name = "g", .kind = msa::Kind::function,
                                                      .definition = msa::Location { "/w/src/g.cpp", range(0, 5, 6) } };
        w.scenario.occurrences = {
            { range(0, 4, 5), "c:@F@f", "f", msa::Kind::function, msa::role::definition },
            { range(0, 10, 11), "c:@F@g", "g", msa::Kind::function, msa::role::reference | msa::role::call },
            { range(0, 15, 16), "c:@F@g", "g", msa::Kind::function, msa::role::reference | msa::role::call },
        };
        w.scenario.symbols = { msa::Symbol { .name = "f", .kind = msa::Kind::function, .range = range(0, 0, 19), .selection = range(0, 4, 5) } };
        lsp::Service service { w, { .parse_workers = 1, .debounce = std::chrono::milliseconds { 0 } }, {} };
        service.open(uri, "int f() { g(); g(); }\n", 1);
        const auto info = service.request("textDocument/symbolInfo", at(uri, 0, 4));
        expect(fatal(info.has_value()));
        expect((*info)[0]["usr"] == "c:@F@f" && (*info)[0]["containerName"] == "ns::") << info->dump();
        const auto items = service.request("textDocument/prepareCallHierarchy", at(uri, 0, 4));
        expect(fatal(items.has_value() && items->size() == 1));
        const auto calls = service.request("callHierarchy/outgoingCalls", Json { { "item", (*items)[0] } });
        expect(fatal(calls.has_value()));
        expect(calls->size() == 1 && (*calls)[0]["to"]["name"] == "g" && (*calls)[0]["fromRanges"].size() == 2) << calls->dump();
    };

    "closing a document hands the file back to the disk"_test = [&] {
        FakeWorkspace w;
        {
            lsp::Service service { w, { .parse_workers = 1, .debounce = std::chrono::milliseconds { 0 } }, {} };
            service.open(uri, "int x;\n", 1);
            service.close(uri);
        }
        expect(w.closed == std::vector<std::string> { path });
    };

    return report();
}
