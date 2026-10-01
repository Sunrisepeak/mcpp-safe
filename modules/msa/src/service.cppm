// mcxx.msa:service -- the interfaces a backend implements: a parsed file (Unit) and a program
// (Workspace).
export module mcxx.msa:service;

import std;
import :basics;
import :entities;
import :facts;

export namespace mcxx::msa {

// A parsed file: the snapshot one version of its text produced. Immutable once returned, so any
// number of threads may read it.
class Unit {
public:
    virtual ~Unit() = default;
    virtual const std::string& path() const = 0;
    virtual std::int64_t version() const = 0;
    virtual std::string_view text() const = 0;
    virtual std::string_view module_name() const = 0;   // "m", "m:p" or "" when the file is no module unit
    virtual std::span<const Diagnostic> diagnostics() const = 0;
    // Every occurrence of a named entity in this file, ordered by position.
    virtual std::span<const Occurrence> occurrences() const = 0;
    virtual std::vector<Symbol> symbols() const = 0;
    // The entity whose name covers `at`, if any.
    virtual std::optional<Entity> entity_at(Position at) const = 0;
    // An entity this unit's AST can resolve, by id.
    virtual std::optional<Entity> entity(std::string_view id) const = 0;
    // Methods overriding the given one, as far as this unit sees them.
    virtual std::vector<Location> overriders(std::string_view id) const = 0;
    // What the file's own code declares and does (MC3 v0): computed once, on first use.
    virtual const fact::Facts& facts() const = 0;
};

// Cooperative cancellation: a long operation polls it and returns early when set.
using Cancel = std::stop_token;

// The program: its compile commands, its modules, and what is known across units.
class Workspace {
public:
    struct Options {
        std::string cache_directory;      // module interfaces and the index live here
        std::string resource_directory;   // the backend's builtin headers (Clang: lib/clang/<v>)
        unsigned workers { 0 };           // 0: a quarter of the hardware threads, at least 1
        bool background_index { true };
        // Where the backend's own log lines go (never standard output): each with its level and
        // category ("modules", "parse", "index", "complete", "workspace"). Which ones are produced is
        // MCXX_LOG's to say (mcxx.base.trace); failures always are.
        std::function<void(LogLevel level, std::string_view category, std::string_view message)> log;
        // Called, from any thread, whenever status() changed.
        std::function<void()> changed;
    };

    virtual ~Workspace() = default;

    // The program is described again: new or changed commands, units gone.
    virtual void set_commands(std::vector<Command> commands) = 0;
    virtual Status status() const = 0;

    // Parse one file at one version of its text, building the module interfaces it imports first.
    // The text is the file's from then on, for every unit that reads it (an interface edited in the
    // editor is what its importers see), until close(). Blocks; returns null only when cancelled or
    // when no command at all can be found for the file.
    virtual std::shared_ptr<const Unit> parse(const std::string& path, std::string text, std::int64_t version,
                                              Cancel cancel = {}) = 0;
    virtual std::vector<CompletionItem> complete(const std::string& path, const std::string& text, Position at,
                                                 Cancel cancel = {}) = 0;
    virtual SignatureHelp signature_help(const std::string& path, const std::string& text, Position at,
                                         Cancel cancel = {}) = 0;

    // The program index (every unit, built in the background).
    virtual std::vector<Location> definitions(std::string_view entity) const = 0;
    virtual std::vector<Location> declarations(std::string_view entity) const = 0;
    virtual std::vector<Location> references(std::string_view entity) const = 0;
    virtual std::vector<Found> find(std::string_view query, std::size_t limit) const = 0;
    // The definitions of what `declaredIn` declares are wanted and the index has none yet: the units
    // that can hold them (a module interface's implementation units) are indexed before the rest.
    virtual void index_first(const std::string& declaredIn) { (void)declaredIn; }

    // What a syntax-level reading of the text says at once, without a parse (the Clang backend: MC++'s
    // own front end): the gate findings of the features it decides, and those features -- what an
    // editor shows until the parse's diagnostics come (A1.8.3). Nothing when the backend has no such
    // reading, or nothing is gated.
    struct Quick {
        std::vector<Diagnostic> diagnostics;
        std::vector<std::string> features;   // decided here: a parse's findings of these, on an older text, are superseded
    };
    virtual Quick quick(const std::string& path, std::string_view text) {
        (void)path;
        (void)text;
        return {};
    }

    // A file the editor changed on disk (not an open buffer): its unit and dependents are stale.
    virtual void file_changed(const std::string& path) = 0;
    // The editor closed the file: its text is the disk's again.
    virtual void close(const std::string& path) = 0;
};

} // namespace mcxx::msa
