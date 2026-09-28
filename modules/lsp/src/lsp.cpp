module mcxx.lsp;

import std;
import nlohmann.json;
import mcxx.msa;
import mcxx.base;

namespace mcxx::lsp {

namespace {

namespace base = mcxx::base;

// ---- text and positions -------------------------------------------------------------------

std::string_view line_of(std::string_view text, std::uint32_t line) {
    std::size_t begin { 0 };
    for (std::uint32_t i { 0 }; i < line; ++i) {
        const std::size_t at { text.find('\n', begin) };
        if (at == std::string_view::npos) return {};
        begin = at + 1;
    }
    std::size_t end { text.find('\n', begin) };
    if (end == std::string_view::npos) end = text.size();
    if (end > begin && text[end - 1] == '\r') --end;
    return text.substr(begin, end - begin);
}

std::uint32_t utf16_column(std::string_view text, std::uint32_t line, std::uint32_t byteColumn) {
    const std::string_view l { line_of(text, line) };
    return static_cast<std::uint32_t>(base::utf16_length(l.substr(0, std::min<std::size_t>(byteColumn, l.size()))));
}

Json position_json(std::uint32_t line, std::uint32_t character) { return Json { { "line", line }, { "character", character } }; }

std::optional<std::string> read_text(const std::string& path) {
    std::ifstream in { path, std::ios::binary };
    if (!in) return std::nullopt;
    std::ostringstream text;
    text << in.rdbuf();
    return std::move(text).str();
}

// ---- kinds ---------------------------------------------------------------------------------

int symbol_kind(msa::Kind kind) {
    using K = msa::Kind;
    switch (kind) {
    case K::module: return 2;
    case K::namespace_:
    case K::namespace_alias: return 3;
    case K::class_: return 5;
    case K::method:
    case K::conversion: return 6;
    case K::field: return 8;
    case K::constructor:
    case K::destructor: return 9;
    case K::enum_: return 10;
    case K::concept_: return 11;
    case K::function: return 12;
    case K::variable:
    case K::parameter: return 13;
    case K::enumerator: return 22;
    case K::struct_:
    case K::union_: return 23;
    case K::type_alias: return 5;
    case K::template_parameter: return 26;
    default: return 13;
    }
}

int completion_kind(msa::Kind kind) {
    using K = msa::Kind;
    switch (kind) {
    case K::method:
    case K::conversion: return 2;
    case K::function: return 3;
    case K::constructor:
    case K::destructor: return 4;
    case K::field: return 5;
    case K::variable:
    case K::parameter: return 6;
    case K::class_:
    case K::type_alias: return 7;
    case K::concept_: return 8;
    case K::module:
    case K::namespace_:
    case K::namespace_alias: return 9;
    case K::enum_: return 13;
    case K::enumerator: return 20;
    case K::struct_:
    case K::union_: return 22;
    case K::template_parameter: return 25;
    case K::macro: return 15;
    default: return 14;   // keyword
    }
}

const std::vector<std::string>& token_types() {
    static const std::vector<std::string> types { "namespace", "type",      "class",    "enum",   "interface", "struct",  "typeParameter", "parameter",
                                                  "variable",  "property",  "enumMember", "event", "function",  "method",  "macro",         "keyword",
                                                  "modifier",  "comment",   "string",   "number", "regexp",    "operator", "concept",      "label" };
    return types;
}

const std::vector<std::string>& token_modifiers() {
    static const std::vector<std::string> modifiers { "declaration", "definition", "readonly", "static", "deprecated", "abstract", "async",
                                                      "modification", "documentation", "defaultLibrary" };
    return modifiers;
}

int token_type(msa::Kind kind) {
    using K = msa::Kind;
    auto index = [](std::string_view name) {
        const auto& t = token_types();
        return static_cast<int>(std::ranges::find(t, name) - t.begin());
    };
    switch (kind) {
    case K::module:
    case K::namespace_:
    case K::namespace_alias: return index("namespace");
    case K::class_:
    case K::union_: return index("class");
    case K::struct_: return index("struct");
    case K::enum_: return index("enum");
    case K::enumerator: return index("enumMember");
    case K::type_alias: return index("type");
    case K::concept_: return index("concept");
    case K::function: return index("function");
    case K::method:
    case K::constructor:
    case K::destructor:
    case K::conversion: return index("method");
    case K::field: return index("property");
    case K::variable: return index("variable");
    case K::parameter: return index("parameter");
    case K::template_parameter: return index("typeParameter");
    case K::macro: return index("macro");
    case K::label: return index("label");
    default: return -1;
    }
}

std::string_view kind_word(msa::Kind kind) {
    using K = msa::Kind;
    switch (kind) {
    case K::class_: return "class";
    case K::struct_: return "struct";
    case K::union_: return "union";
    case K::enum_: return "enum";
    case K::enumerator: return "enumerator";
    case K::type_alias: return "type-alias";
    case K::concept_: return "concept";
    case K::function: return "function";
    case K::method: return "instance-method";
    case K::constructor: return "constructor";
    case K::destructor: return "destructor";
    case K::conversion: return "conversion";
    case K::field: return "field";
    case K::variable: return "variable";
    case K::parameter: return "param";
    case K::template_parameter: return "template-type-param";
    case K::namespace_: return "namespace";
    case K::namespace_alias: return "namespace-alias";
    case K::module: return "module";
    case K::macro: return "macro";
    default: return "symbol";
    }
}

std::string hover_markdown(const msa::Entity& e) {
    std::string md { std::format("### {} `{}`", kind_word(e.kind), e.name) };
    std::string provider { e.module };
    if (provider.empty() && e.declaration) provider = std::filesystem::path { e.declaration->path }.filename().string();
    if (!provider.empty()) md += std::format("\n\nprovided by `\"{}\"`", provider);
    md += "\n\n---";
    bool sections { false };
    if (msa::is_callable(e.kind) && e.kind != msa::Kind::constructor && e.kind != msa::Kind::destructor && !e.return_type.empty()) {
        md += std::format("\n→ `{}`", e.return_type);
        sections = true;
    } else if (!e.type.empty() && !msa::is_callable(e.kind)) {
        md += std::format("\nType: `{}`", e.type);
        sections = true;
    }
    if (!e.value.empty()) {
        md += std::format("\nValue = `{}`", e.value);
        sections = true;
    }
    if (!e.parameters.empty()) {
        md += "\n\nParameters:\n";
        for (const auto& p : e.parameters)
            md += std::format("\n- `{}{}{}`", p.type, p.name.empty() ? "" : " " + p.name, p.default_value.empty() ? "" : " = " + p.default_value);
        sections = true;
    }
    if (!e.documentation.empty()) md += std::format("{}{}", sections ? "\n\n---\n" : "\n", e.documentation);
    md += "\n\n---\n```cpp\n";
    if (!e.container.empty() && e.kind != msa::Kind::namespace_) md += std::format("// In {}\n", e.container);
    if (!e.access.empty()) md += e.access + ": ";
    md += e.signature + "\n```";
    return md;
}

} // namespace

std::string uri_to_path(std::string_view uri) {
    if (auto path = base::uri_to_path(uri)) return *path;
    return std::string { uri };
}

std::string path_to_uri(std::string_view path) { return base::path_to_uri(path); }

Json to_lsp(const msa::Range& range, std::string_view text) {
    return Json { { "start", position_json(range.begin.line, utf16_column(text, range.begin.line, range.begin.column)) },
                  { "end", position_json(range.end.line, utf16_column(text, range.end.line, range.end.column)) } };
}

msa::Position from_lsp(const Json& position, std::string_view text) {
    const std::uint32_t line { position.value("line", 0u) };
    const std::uint32_t character { position.value("character", 0u) };
    const std::string_view l { line_of(text, line) };
    std::size_t bytes { 0 };
    std::uint32_t units { 0 };
    while (bytes < l.size() && units < character) {
        const unsigned char c { static_cast<unsigned char>(l[bytes]) };
        const std::size_t n { c < 0x80 ? 1u : (c >> 5) == 0x6 ? 2u : (c >> 4) == 0xE ? 3u : (c >> 3) == 0x1E ? 4u : 1u };
        units += n == 4 ? 2 : 1;
        bytes += n;
    }
    return { line, static_cast<std::uint32_t>(std::min(bytes, l.size())) };
}

struct Service::Document {
    std::string uri;
    std::string path;
    std::string text;
    std::int64_t version { 0 };
    std::shared_ptr<const msa::Unit> unit;
    std::chrono::steady_clock::time_point due;
    bool queued { false };
    bool open { true };
};

struct Service::State {
    msa::Workspace& workspace;
    Options options;
    Notify notify;
    std::mutex mutex;
    std::condition_variable_any cv;
    std::map<std::string, std::shared_ptr<Document>> documents;   // by uri
    std::deque<std::string> queue;
    bool stopping { false };
    std::vector<std::jthread> workers;
    // Texts of files that are not open, for position conversion; dropped when a file changes.
    std::mutex textsMutex;
    std::map<std::string, std::shared_ptr<const std::string>> texts;

    State(msa::Workspace& w, Options o, Notify n) : workspace { w }, options { std::move(o) }, notify { std::move(n) } {}

    std::shared_ptr<const std::string> text_of(const std::string& path) {
        {
            std::lock_guard lock { mutex };
            for (const auto& [uri, doc] : documents)
                if (doc->path == path) return std::make_shared<const std::string>(doc->text);
        }
        std::lock_guard lock { textsMutex };
        if (const auto it = texts.find(path); it != texts.end()) return it->second;
        auto text = read_text(path);
        auto shared = std::make_shared<const std::string>(text ? std::move(*text) : std::string {});
        if (texts.size() > 256) texts.clear();
        texts.emplace(path, shared);
        return shared;
    }

    void forget_text(const std::string& path) {
        std::lock_guard lock { textsMutex };
        texts.erase(path);
    }

    Json location(const msa::Location& l) {
        const auto text = text_of(l.path);
        return Json { { "uri", path_to_uri(l.path) }, { "range", to_lsp(l.range, *text) } };
    }

    void publish(const Document& doc, const msa::Unit& unit) {
        Json diagnostics = Json::array();
        const std::string_view text { unit.text() };
        for (const auto& d : unit.diagnostics()) {
            Json item { { "range", to_lsp(d.range, text) }, { "severity", static_cast<int>(d.severity) }, { "source", "mcxx" }, { "message", d.message } };
            if (!d.code.empty()) item["code"] = d.code;
            if (!d.notes.empty()) {
                Json related = Json::array();
                for (const auto& n : d.notes) related.push_back(Json { { "location", location(n.location) }, { "message", n.message } });
                item["relatedInformation"] = std::move(related);
            }
            diagnostics.push_back(std::move(item));
        }
        notify("textDocument/publishDiagnostics", Json { { "uri", doc.uri }, { "version", unit.version() }, { "diagnostics", std::move(diagnostics) } });
    }

    void schedule(const std::shared_ptr<Document>& doc) {
        doc->due = std::chrono::steady_clock::now() + options.debounce;
        if (!doc->queued) {
            doc->queued = true;
            queue.push_back(doc->uri);
        }
        cv.notify_all();
    }

    void work(std::stop_token stop) {
        while (true) {
            std::shared_ptr<Document> doc;
            std::string text;
            std::int64_t version { 0 };
            {
                std::unique_lock lock { mutex };
                while (true) {
                    if (stopping || stop.stop_requested()) return;
                    if (queue.empty()) {
                        cv.wait(lock, stop, [&] { return stopping || !queue.empty(); });
                        continue;
                    }
                    const auto it = documents.find(queue.front());
                    if (it == documents.end() || !it->second->open) {
                        queue.pop_front();
                        continue;
                    }
                    const auto now = std::chrono::steady_clock::now();
                    if (it->second->due > now) {
                        cv.wait_until(lock, stop, it->second->due, [&] { return stopping; });
                        continue;
                    }
                    doc = it->second;
                    queue.pop_front();
                    doc->queued = false;
                    text = doc->text;
                    version = doc->version;
                    break;
                }
            }
            auto unit = workspace.parse(doc->path, std::move(text), version, stop);
            if (!unit) continue;
            {
                std::lock_guard lock { mutex };
                if (!doc->unit || doc->unit->version() <= unit->version()) doc->unit = unit;
            }
            cv.notify_all();
            publish(*doc, *unit);
        }
    }
};

Service::Service(msa::Workspace& workspace, Options options, Notify notify)
    : state_ { std::make_unique<State>(workspace, std::move(options), std::move(notify)) } {
    for (unsigned i { 0 }; i < std::max(1u, state_->options.parse_workers); ++i)
        state_->workers.emplace_back([s = state_.get()](std::stop_token stop) { s->work(stop); });
}

Service::~Service() {
    {
        std::lock_guard lock { state_->mutex };
        state_->stopping = true;
    }
    state_->cv.notify_all();
    for (auto& w : state_->workers) w.request_stop();
    state_->cv.notify_all();
    state_->workers.clear();
}

Json Service::legend() { return Json { { "tokenTypes", token_types() }, { "tokenModifiers", token_modifiers() } }; }

Json Service::capabilities() {
    return Json {
        { "textDocumentSync", Json { { "openClose", true }, { "change", 2 }, { "save", Json { { "includeText", false } } } } },
        { "definitionProvider", true },
        { "declarationProvider", true },
        { "typeDefinitionProvider", true },
        { "implementationProvider", true },
        { "referencesProvider", true },
        { "hoverProvider", true },
        { "documentHighlightProvider", true },
        { "documentSymbolProvider", true },
        { "workspaceSymbolProvider", true },
        { "completionProvider", Json { { "triggerCharacters", Json::array({ ".", "<", ">", ":", "\"", "/", "*" }) }, { "resolveProvider", false } } },
        { "signatureHelpProvider", Json { { "triggerCharacters", Json::array({ "(", ",", ")", "{", "}" }) } } },
        { "semanticTokensProvider", Json { { "legend", legend() }, { "full", true }, { "range", false } } },
    };
}

void Service::open(const std::string& uri, std::string text, std::int64_t version) {
    std::lock_guard lock { state_->mutex };
    auto& doc = state_->documents[uri];
    if (!doc) doc = std::make_shared<Document>();
    doc->uri = uri;
    doc->path = uri_to_path(uri);
    doc->text = std::move(text);
    doc->version = version;
    doc->open = true;
    state_->schedule(doc);
}

void Service::change(const std::string& uri, const Json& changes, std::int64_t version) {
    std::lock_guard lock { state_->mutex };
    const auto it = state_->documents.find(uri);
    if (it == state_->documents.end()) return;
    auto& doc = *it->second;
    for (const auto& change : changes) {
        if (!change.contains("range")) {
            doc.text = change.value("text", std::string {});
            continue;
        }
        const auto begin = from_lsp(change["range"]["start"], doc.text);
        const auto end = from_lsp(change["range"]["end"], doc.text);
        auto offset = [&](msa::Position p) {
            std::size_t at { 0 };
            for (std::uint32_t line { 0 }; line < p.line && at < doc.text.size(); ++line) {
                const std::size_t nl { doc.text.find('\n', at) };
                at = nl == std::string::npos ? doc.text.size() : nl + 1;
            }
            return std::min(doc.text.size(), at + p.column);
        };
        const std::size_t b { offset(begin) };
        const std::size_t e { std::max(b, offset(end)) };
        doc.text.replace(b, e - b, change.value("text", std::string {}));
    }
    doc.version = version;
    state_->schedule(it->second);
}

void Service::close(const std::string& uri) {
    std::lock_guard lock { state_->mutex };
    state_->documents.erase(uri);
}

void Service::saved(const std::string& uri) {
    const std::string path { uri_to_path(uri) };
    state_->forget_text(path);
    state_->workspace.file_changed(path);
}

void Service::changed_on_disk(const std::string& uri) {
    const std::string path { uri_to_path(uri) };
    state_->forget_text(path);
    state_->workspace.file_changed(path);
}

std::shared_ptr<const msa::Unit> Service::unit(const std::string& uri, bool current, std::stop_token cancel) {
    std::unique_lock lock { state_->mutex };
    const auto it = state_->documents.find(uri);
    if (it == state_->documents.end()) return nullptr;
    auto doc = it->second;
    if (!current) return doc->unit;
    state_->cv.wait_for(lock, cancel, state_->options.wait,
                        [&] { return state_->stopping || !doc->open || (doc->unit && doc->unit->version() >= doc->version); });
    return doc->unit;
}

Result Service::request(std::string_view method, const Json& params, std::stop_token cancel) {
    auto& s = *state_;
    const std::string uri { params.contains("textDocument") ? params["textDocument"].value("uri", std::string {}) : std::string {} };

    if (method == "workspace/symbol") {
        Json out = Json::array();
        for (const auto& f : s.workspace.find(params.value("query", std::string {}), 200)) {
            out.push_back(Json { { "name", f.name }, { "kind", symbol_kind(f.kind) }, { "location", s.location(f.location) },
                                 { "containerName", f.container } });
        }
        return out;
    }

    auto unit = this->unit(uri, true, cancel);
    if (!unit) {
        if (cancel.stop_requested()) return std::unexpected(Error { -32800, "cancelled" });
        return Json(nullptr);
    }
    const std::string_view text { unit->text() };
    const msa::Position at { params.contains("position") ? from_lsp(params["position"], text) : msa::Position {} };

    auto locations = [&](const std::vector<msa::Location>& list) {
        Json out = Json::array();
        for (const auto& l : list) out.push_back(s.location(l));
        return out;
    };

    if (method == "textDocument/definition" || method == "textDocument/declaration" || method == "textDocument/typeDefinition" ||
        method == "textDocument/implementation") {
        const auto entity = unit->entity_at(at);
        if (!entity) return Json::array();
        std::vector<msa::Location> found;
        if (method == "textDocument/declaration") {
            if (entity->declaration) found.push_back(*entity->declaration);
        } else if (method == "textDocument/typeDefinition") {
            if (entity->type_location) found.push_back(*entity->type_location);
        } else if (method == "textDocument/implementation") {
            found = unit->overriders(entity->id);
        } else {
            if (entity->definition) found.push_back(*entity->definition);
            if (found.empty()) found = s.workspace.definitions(entity->id);
            if (found.empty() && entity->declaration) found.push_back(*entity->declaration);
        }
        return locations(found);
    }

    if (method == "textDocument/hover") {
        const auto entity = unit->entity_at(at);
        if (!entity) return Json(nullptr);
        Json hover { { "contents", Json { { "kind", "markdown" }, { "value", hover_markdown(*entity) } } } };
        for (const auto& o : unit->occurrences())
            if (o.entity == entity->id && o.range.contains(at)) {
                hover["range"] = to_lsp(o.range, text);
                break;
            }
        return hover;
    }

    if (method == "textDocument/references") {
        const auto entity = unit->entity_at(at);
        if (!entity) return Json::array();
        const bool withDeclaration { params.contains("context") && params["context"].value("includeDeclaration", false) };
        std::vector<msa::Location> found;
        for (const auto& l : s.workspace.references(entity->id))
            if (l.path != unit->path()) found.push_back(l);
        for (const auto& o : unit->occurrences())
            if (o.entity == entity->id) found.push_back({ unit->path(), o.range });
        if (!withDeclaration) {
            std::vector<msa::Location> declared { s.workspace.declarations(entity->id) };
            for (const auto& o : unit->occurrences())
                if (o.entity == entity->id && (o.roles & (msa::role::declaration | msa::role::definition)))
                    declared.push_back({ unit->path(), o.range });
            std::erase_if(found, [&](const msa::Location& l) { return std::ranges::find(declared, l) != declared.end(); });
        }
        std::ranges::sort(found, [](const auto& a, const auto& b) { return std::tie(a.path, a.range.begin) < std::tie(b.path, b.range.begin); });
        found.erase(std::unique(found.begin(), found.end()), found.end());
        return locations(found);
    }

    if (method == "textDocument/documentHighlight") {
        const auto entity = unit->entity_at(at);
        Json out = Json::array();
        if (!entity) return out;
        for (const auto& o : unit->occurrences())
            if (o.entity == entity->id) out.push_back(Json { { "range", to_lsp(o.range, text) }, { "kind", (o.roles & msa::role::write) ? 3 : 2 } });
        return out;
    }

    if (method == "textDocument/documentSymbol") {
        std::function<Json(const msa::Symbol&)> convert = [&](const msa::Symbol& symbol) {
            Json out { { "name", symbol.name }, { "kind", symbol_kind(symbol.kind) }, { "range", to_lsp(symbol.range, text) },
                       { "selectionRange", to_lsp(symbol.selection, text) } };
            if (!symbol.detail.empty()) out["detail"] = symbol.detail;
            if (!symbol.children.empty()) {
                Json children = Json::array();
                for (const auto& c : symbol.children) children.push_back(convert(c));
                out["children"] = std::move(children);
            }
            return out;
        };
        Json out = Json::array();
        for (const auto& symbol : unit->symbols()) out.push_back(convert(symbol));
        return out;
    }

    if (method == "textDocument/semanticTokens/full") {
        Json data = Json::array();
        std::uint32_t lastLine { 0 };
        std::uint32_t lastColumn { 0 };
        for (const auto& o : unit->occurrences()) {
            if (o.range.begin.line != o.range.end.line || (o.roles & msa::role::implicit)) continue;
            const int type { token_type(o.kind) };
            if (type < 0) continue;
            std::uint32_t modifiers { 0 };
            if (o.roles & msa::role::declaration) modifiers |= 1u << 0;
            if (o.roles & msa::role::definition) modifiers |= 1u << 1;
            const std::uint32_t line { o.range.begin.line };
            const std::uint32_t column { utf16_column(text, line, o.range.begin.column) };
            const std::uint32_t length { utf16_column(text, line, o.range.end.column) - column };
            if (length == 0) continue;
            if (line == lastLine && column < lastColumn) continue;   // overlapping: keep the first
            data.push_back(line - lastLine);
            data.push_back(line == lastLine ? column - lastColumn : column);
            data.push_back(length);
            data.push_back(type);
            data.push_back(modifiers);
            lastLine = line;
            lastColumn = column;
        }
        return Json { { "data", std::move(data) } };
    }

    std::string current;
    {
        std::lock_guard lock { s.mutex };
        if (const auto it = s.documents.find(uri); it != s.documents.end()) current = it->second->text;
    }
    if (method == "textDocument/completion") {
        const msa::Position p { from_lsp(params["position"], current) };
        Json items = Json::array();
        std::size_t rank { 0 };
        for (const auto& item : s.workspace.complete(unit->path(), current, p, cancel)) {
            Json out { { "label", item.label }, { "kind", completion_kind(item.kind) }, { "insertText", item.insert_text },
                       { "filterText", item.filter_text }, { "sortText", std::format("{:05}", rank++) }, { "insertTextFormat", 1 } };
            if (!item.detail.empty()) out["detail"] = item.detail;
            if (!item.documentation.empty()) out["documentation"] = item.documentation;
            items.push_back(std::move(out));
        }
        return Json { { "isIncomplete", false }, { "items", std::move(items) } };
    }

    if (method == "textDocument/signatureHelp") {
        const msa::Position p { from_lsp(params["position"], current) };
        const auto help = s.workspace.signature_help(unit->path(), current, p, cancel);
        if (help.signatures.empty()) return Json(nullptr);
        Json signatures = Json::array();
        for (const auto& sig : help.signatures) {
            Json parameters = Json::array();
            for (const auto& [b, e] : sig.parameters)
                parameters.push_back(Json { { "label", Json::array({ base::utf16_length(std::string_view { sig.label }.substr(0, b)),
                                                                    base::utf16_length(std::string_view { sig.label }.substr(0, e)) }) } });
            Json out { { "label", sig.label }, { "parameters", std::move(parameters) } };
            if (!sig.documentation.empty()) out["documentation"] = sig.documentation;
            signatures.push_back(std::move(out));
        }
        return Json { { "signatures", std::move(signatures) }, { "activeSignature", help.active_signature },
                      { "activeParameter", help.active_parameter } };
    }

    return std::unexpected(Error { -32601, std::format("{} is not supported by mcxx", method) });
}

} // namespace mcxx::lsp
