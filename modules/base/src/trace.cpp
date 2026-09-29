module mcxx.base.trace;

import std;

namespace mcxx::base::trace {
namespace {

using Clock = std::chrono::steady_clock;

struct State {
    std::mutex mutex;
    Config config;
    Sink sink;
    std::map<std::string, std::int64_t, std::less<>> counters;
    std::FILE* trace { nullptr };
    bool first_event { true };
    Clock::time_point epoch { Clock::now() };
};

State& state() {
    static State s;
    return s;
}

// The lowest level any category is on at: below it nothing is enabled, and enabled() says so with one
// load -- so a trace point in a hot loop (a parser's) costs nothing while its category is off.
constinit std::atomic<int> floor_ { static_cast<int>(Level::warning) };

int lowest(const Config& config) {
    int low { static_cast<int>(config.level) };
    for (const auto& [category, level] : config.categories) low = std::min(low, static_cast<int>(level));
    return low;
}

std::uint64_t thread_number() {
    static std::atomic<std::uint64_t> next { 1 };
    thread_local const std::uint64_t mine { next++ };
    return mine;
}

std::string json_escape(std::string_view text) {
    std::string out;
    out.reserve(text.size());
    for (const char c : text) {
        switch (c) {
        case '"': out += "\\\""; break;
        case '\\': out += "\\\\"; break;
        case '\n': out += "\\n"; break;
        case '\t': out += "\\t"; break;
        default:
            if (static_cast<unsigned char>(c) < 0x20) out += std::format("\\u{:04x}", static_cast<unsigned>(c));
            else out += c;
        }
    }
    return out;
}

void open_trace(State& s) {
    if (s.trace != nullptr) {
        std::fputs("\n]\n", s.trace);
        std::fclose(s.trace);
        s.trace = nullptr;
    }
    if (s.config.trace_file.empty()) return;
    s.trace = std::fopen(s.config.trace_file.c_str(), "w");
    if (s.trace != nullptr) {
        std::fputs("[\n", s.trace);
        s.first_event = true;
    }
}

} // namespace

std::string_view to_string(Level level) {
    switch (level) {
    case Level::debug: return "debug";
    case Level::info: return "info";
    case Level::warning: return "warning";
    case Level::error: return "error";
    case Level::off: return "off";
    }
    return "off";
}

std::optional<Level> parse_level(std::string_view name) {
    for (const Level level : { Level::debug, Level::info, Level::warning, Level::error, Level::off })
        if (to_string(level) == name) return level;
    if (name == "warn") return Level::warning;
    return std::nullopt;
}

std::optional<Config> parse(std::string_view spec) {
    Config config;
    for (auto part : spec | std::views::split(',')) {
        std::string_view item { part.begin(), part.end() };
        while (!item.empty() && item.front() == ' ') item.remove_prefix(1);
        while (!item.empty() && item.back() == ' ') item.remove_suffix(1);
        if (item.empty()) continue;
        if (const auto eq = item.find('='); eq != std::string_view::npos) {
            const auto level = parse_level(item.substr(eq + 1));
            if (!level || eq == 0) return std::nullopt;
            config.categories.insert_or_assign(std::string { item.substr(0, eq) }, *level);
        } else {
            const auto level = parse_level(item);
            if (!level) return std::nullopt;
            config.level = *level;
        }
    }
    return config;
}

void configure(Config config) {
    auto& s = state();
    std::lock_guard lock { s.mutex };
    const bool reopen { config.trace_file != s.config.trace_file };
    s.config = std::move(config);
    floor_.store(lowest(s.config), std::memory_order_relaxed);
    if (reopen) open_trace(s);
}

void configure_from_environment() {
    Config config { configuration() };
    if (const char* spec = std::getenv("MCXX_LOG"); spec != nullptr && *spec != '\0') {
        if (auto parsed = parse(spec)) {
            parsed->trace_file = config.trace_file;
            config = std::move(*parsed);
        }
    }
    if (const char* file = std::getenv("MCXX_TRACE"); file != nullptr && *file != '\0') config.trace_file = file;
    configure(std::move(config));
}

Config configuration() {
    auto& s = state();
    std::lock_guard lock { s.mutex };
    return s.config;
}

void set_sink(Sink sink) {
    auto& s = state();
    std::lock_guard lock { s.mutex };
    s.sink = std::move(sink);
}

bool enabled(std::string_view category, Level level) {
    if (static_cast<int>(level) < floor_.load(std::memory_order_relaxed)) return false;
    auto& s = state();
    std::lock_guard lock { s.mutex };
    const auto it = s.config.categories.find(category);
    const Level threshold { it != s.config.categories.end() ? it->second : s.config.level };
    return threshold != Level::off && level >= threshold;
}

void write(std::string_view category, Level level, std::string_view message) {
    Sink sink;
    {
        auto& s = state();
        std::lock_guard lock { s.mutex };
        sink = s.sink;
    }
    if (sink) {
        sink(level, category, message);
        return;
    }
    std::println(std::cerr, "mcxx [{}] {}: {}", to_string(level), category, message);
}

Span::Span(std::string_view category, std::string_view name, std::string detail, std::chrono::milliseconds slow)
    : category_ { category }, name_ { name }, detail_ { std::move(detail) }, slow_ { slow }, start_ { Clock::now() } {}

double Span::seconds() const { return std::chrono::duration<double>(Clock::now() - start_).count(); }

void Span::note(std::string text) {
    if (!notes_.empty()) notes_ += ", ";
    notes_ += std::move(text);
}

Span::~Span() {
    const auto end = Clock::now();
    const double took { std::chrono::duration<double>(end - start_).count() };
    count(std::format("{}.{}", category_, name_));
    const Level level { end - start_ >= slow_ ? Level::info : Level::debug };
    if (enabled(category_, level)) {
        write(category_, level, std::format("{}{}{} took {:.3f} s{}{}", name_, detail_.empty() ? "" : " ", detail_, took,
                                            notes_.empty() ? "" : "; ", notes_));
    }
    auto& s = state();
    std::lock_guard lock { s.mutex };
    if (s.trace == nullptr) return;
    const auto ts = std::chrono::duration_cast<std::chrono::microseconds>(start_ - s.epoch).count();
    const auto dur = std::chrono::duration_cast<std::chrono::microseconds>(end - start_).count();
    std::string args { std::format("\"detail\":\"{}\"", json_escape(detail_)) };
    if (!notes_.empty()) args += std::format(",\"notes\":\"{}\"", json_escape(notes_));
    std::println(s.trace, "{}{{\"name\":\"{}\",\"cat\":\"{}\",\"ph\":\"X\",\"ts\":{},\"dur\":{},\"pid\":1,\"tid\":{},\"args\":{{{}}}}}",
                 s.first_event ? "" : ",", json_escape(name_), json_escape(category_), ts, dur, thread_number(), args);
    s.first_event = false;
    std::fflush(s.trace);
}

void count(std::string_view name, std::int64_t delta) {
    auto& s = state();
    std::lock_guard lock { s.mutex };
    auto it = s.counters.find(name);
    if (it == s.counters.end()) it = s.counters.emplace(std::string { name }, 0).first;
    it->second += delta;
}

std::vector<std::pair<std::string, std::int64_t>> counters() {
    auto& s = state();
    std::lock_guard lock { s.mutex };
    return { s.counters.begin(), s.counters.end() };
}

void reset_counters() {
    auto& s = state();
    std::lock_guard lock { s.mutex };
    s.counters.clear();
}

void flush() {
    auto& s = state();
    std::lock_guard lock { s.mutex };
    if (s.trace == nullptr) return;
    std::fputs("\n]\n", s.trace);
    std::fclose(s.trace);
    s.trace = nullptr;
}

} // namespace mcxx::base::trace
