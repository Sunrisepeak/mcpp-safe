// mcxx.base.trace: configuration, filtering by category, spans, counters and the trace file.
import std;
import mcxx.testing;
import mcxx.base.trace;

namespace trace = mcxx::base::trace;

int main() {
    using namespace mcxx::testing;

    "a spec names a default level and per-category ones"_test = [] {
        const auto config = trace::parse("debug, parse=info,modules=off");
        expect(fatal(config.has_value()));
        expect(config->level == trace::Level::debug);
        expect(config->categories.at("parse") == trace::Level::info);
        expect(config->categories.at("modules") == trace::Level::off);
        expect(!trace::parse("loud").has_value()) << "an unknown level is refused, not guessed";
        expect(!trace::parse("=info").has_value());
    };

    "lines are filtered per category and reach the sink"_test = [] {
        std::vector<std::string> lines;
        trace::set_sink([&](trace::Level level, std::string_view category, std::string_view message) {
            lines.push_back(std::format("{} {} {}", trace::to_string(level), category, message));
        });
        trace::configure(*trace::parse("info,parse=debug,index=off"));
        trace::debug("modules", "hidden {}", 1);
        trace::info("modules", "shown {}", 2);
        trace::debug("parse", "shown {}", 3);
        trace::error("index", "hidden {}", 4);
        expect(lines == std::vector<std::string> { "info modules shown 2", "debug parse shown 3" });
        trace::set_sink({});
    };

    "a span counts itself, logs its duration and writes a trace event"_test = [] {
        const std::string file { (std::filesystem::temp_directory_path() / std::format("mcxx-trace-{}.json", std::random_device {}())).string() };
        std::vector<std::string> lines;
        trace::set_sink([&](trace::Level, std::string_view, std::string_view message) { lines.emplace_back(message); });
        trace::Config config { *trace::parse("debug") };
        config.trace_file = file;
        trace::configure(config);
        trace::reset_counters();
        {
            trace::Span span { "modules", "build", "m.a" };
            span.note("cached");
        }
        { trace::Span span { "modules", "build", "m.b" }; }
        trace::flush();
        const auto counters = trace::counters();
        expect(std::ranges::find(counters, std::pair<std::string, std::int64_t> { "modules.build", 2 }) != counters.end());
        expect(lines.size() == 2 && lines[0].starts_with("build m.a took ") && lines[0].ends_with("; cached"));
        std::ifstream in { file };
        const std::string text { std::istreambuf_iterator<char> { in }, {} };
        expect(text.starts_with("[\n")) << text;
        expect(text.contains("\"name\":\"build\",\"cat\":\"modules\",\"ph\":\"X\""));
        expect(text.contains("\"detail\":\"m.b\""));
        expect(text.contains("]"));
        std::filesystem::remove(file);
        trace::set_sink({});
        trace::configure({});
    };

    return report();
}
