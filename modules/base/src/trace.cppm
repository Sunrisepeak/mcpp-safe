// Observability for libmc++: categorized log lines, timed spans, counters, and a trace file.
//
//   MCXX_LOG=info                      every category at info and above (the default: warning)
//   MCXX_LOG=debug,parse=info          debug everywhere but parse
//   MCXX_TRACE=/tmp/mcxx.json          every span as a Chrome trace event (chrome://tracing, Perfetto)
//
//   trace::info("modules", "built {} in {:.2f} s", name, seconds);
//   { trace::Span span { "parse", path }; ... }       // logs at debug with its duration, traces it
//   trace::count("modules.built");                    // counters(): what status and reports show
//
// A library does not own standard error: lines go to the sink its host installs (set_sink), and to
// standard error only when none is installed and MCXX_LOG asked for them.
export module mcxx.base.trace;

import std;

export namespace mcxx::base::trace {

enum class Level { debug, info, warning, error, off };

std::string_view to_string(Level level);
std::optional<Level> parse_level(std::string_view name);

// "info" or "debug,parse=info,modules=warning": a default level, then per-category overrides.
struct Config {
    Level level { Level::warning };
    std::map<std::string, Level, std::less<>> categories;
    std::string trace_file;   // empty: no trace file
};
std::optional<Config> parse(std::string_view spec);

void configure(Config config);
// MCXX_LOG and MCXX_TRACE, when set; otherwise the configuration stays as it is.
void configure_from_environment();
Config configuration();

using Sink = std::function<void(Level level, std::string_view category, std::string_view message)>;
void set_sink(Sink sink);

bool enabled(std::string_view category, Level level);
void write(std::string_view category, Level level, std::string_view message);

template <class... Args>
void debug(std::string_view category, std::format_string<Args...> fmt, Args&&... args) {
    if (enabled(category, Level::debug)) write(category, Level::debug, std::format(fmt, std::forward<Args>(args)...));
}
template <class... Args>
void info(std::string_view category, std::format_string<Args...> fmt, Args&&... args) {
    if (enabled(category, Level::info)) write(category, Level::info, std::format(fmt, std::forward<Args>(args)...));
}
template <class... Args>
void warning(std::string_view category, std::format_string<Args...> fmt, Args&&... args) {
    if (enabled(category, Level::warning)) write(category, Level::warning, std::format(fmt, std::forward<Args>(args)...));
}
template <class... Args>
void error(std::string_view category, std::format_string<Args...> fmt, Args&&... args) {
    if (enabled(category, Level::error)) write(category, Level::error, std::format(fmt, std::forward<Args>(args)...));
}

// A timed region. Its end logs "<name> <detail> took <s>" at debug in its category (at info when it
// took at least `slow`), counts "<category>.<name>", and writes one trace event when tracing.
class Span {
public:
    Span(std::string_view category, std::string_view name, std::string detail = {},
         std::chrono::milliseconds slow = std::chrono::milliseconds { 2000 });
    ~Span();
    Span(const Span&) = delete;
    Span& operator=(const Span&) = delete;
    double seconds() const;
    // Something learned inside the span, appended to its end line and trace event (e.g. "cached").
    void note(std::string text);

private:
    std::string category_;
    std::string name_;
    std::string detail_;
    std::string notes_;
    std::chrono::milliseconds slow_;
    std::chrono::steady_clock::time_point start_;
};

void count(std::string_view name, std::int64_t delta = 1);
// Every counter, sorted by name.
std::vector<std::pair<std::string, std::int64_t>> counters();
void reset_counters();

// Closes the trace file's JSON array (a missing "]" is accepted by the viewers too, so a crash
// still leaves a readable trace).
void flush();

} // namespace mcxx::base::trace
