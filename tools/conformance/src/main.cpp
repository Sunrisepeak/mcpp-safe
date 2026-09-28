// mcxx-conformance: MC++'s gate fixtures, checked.
//
//   mcxx-conformance [--json FILE] [DIR]          default DIR: conformance/gates
//
// Every directory with sources is one program (its files compile together; module interfaces are
// built as its importers need them), under the nearest mcpp.toml's [package.metadata.mcxx]. A line
// that must be reported carries `// expect: <feature> [<feature> ...] [-- a note]`; every gate diagnostic must
// be expected and every expectation reported, on that line. Anything else the compiler says is an
// error is a broken fixture. The report counts, per feature, what was found, missed and wrong.
import std;
import mcxx.msa;
import mcxx.plugin;
import mcxx.backend;
import mcxx.plugins.std;
import mcxx.plugins.libs;

namespace msa = mcxx::msa;
namespace fs = std::filesystem;

namespace {

struct Mark {
    std::string file;
    std::uint32_t line { 0 };   // 0-based
    std::string feature;
    auto operator<=>(const Mark&) const = default;
};

struct Tally {
    int found { 0 }, missed { 0 }, wrong { 0 };
};

std::vector<std::string> expectations(std::string_view line) {
    std::vector<std::string> out;
    const auto at = line.find("// expect:");
    if (at == std::string_view::npos) return out;
    std::string rest { line.substr(at + 10) };
    if (const auto note = rest.find("--"); note != std::string::npos) rest.erase(note);   // `-- why`, for readers
    std::ranges::replace(rest, ',', ' ');
    std::istringstream in { rest };
    for (std::string id; in >> id;) out.push_back(id);
    return out;
}

std::string read(const fs::path& p) {
    std::ifstream in { p };
    return { std::istreambuf_iterator<char> { in }, {} };
}

} // namespace

int main(int argc, char** argv) {
    std::string json;
    fs::path root { "conformance/gates" };
    for (int i { 1 }; i < argc; ++i) {
        const std::string_view a { argv[i] };
        if (a == "--json" && i + 1 < argc) json = argv[++i];
        else root = a;
    }
    root = fs::absolute(root);
    const fs::path cache { fs::temp_directory_path() / std::format("mcxx-conformance-{}", std::random_device {}()) };

    // Programs: directories that hold sources.
    std::map<fs::path, std::vector<fs::path>> programs;
    for (const auto& e : fs::recursive_directory_iterator { root }) {
        const auto ext = e.path().extension();
        if (e.is_regular_file() && (ext == ".cpp" || ext == ".cppm")) programs[e.path().parent_path()].push_back(e.path());
    }

    std::map<std::string, Tally> tally;
    std::vector<std::string> problems;
    int files { 0 };
    for (auto& [dir, sources] : programs) {
        std::ranges::sort(sources);
        std::vector<msa::Command> commands;
        for (const auto& s : sources) {
            const std::string p { s.generic_string() };
            commands.push_back({ dir.generic_string(), p, { "clang++", "-std=c++23", "--target=x86_64-unknown-linux-gnu", "-c", p } });
        }
        msa::Workspace::Options options;
        options.cache_directory = (cache / fs::relative(dir, root)).generic_string();
        options.workers = 4;
        options.background_index = false;
        auto workspace = mcxx::backend::make_workspace(std::move(options));
        workspace->set_commands(commands);
        std::set<Mark> expected, reported;
        for (const auto& s : sources) {
            ++files;
            const std::string path { s.generic_string() };
            const std::string text { read(s) };
            const std::string rel { fs::relative(s, root).generic_string() };
            std::uint32_t n { 0 };
            std::istringstream lines { text };
            for (std::string line; std::getline(lines, line); ++n)
                for (auto& id : expectations(line)) expected.insert({ rel, n, std::move(id) });
            const auto unit = workspace->parse(path, text, 1);
            if (!unit) {
                problems.push_back(std::format("{}: no parse", rel));
                continue;
            }
            for (const auto& d : unit->diagnostics()) {
                if (mcxx::plugin::find_feature(d.code) != nullptr) reported.insert({ rel, d.range.begin.line, d.code });
                else if (d.severity == msa::Severity::error)
                    problems.push_back(std::format("{}:{}: the fixture does not compile: {}", rel, d.range.begin.line + 1, d.message));
            }
        }
        for (const auto& m : expected) {
            if (reported.contains(m)) ++tally[m.feature].found;
            else {
                ++tally[m.feature].missed;
                problems.push_back(std::format("{}:{}: expected {} was not reported", m.file, m.line + 1, m.feature));
            }
        }
        for (const auto& m : reported) {
            if (expected.contains(m)) continue;
            ++tally[m.feature].wrong;
            problems.push_back(std::format("{}:{}: {} was reported where nothing expects it", m.file, m.line + 1, m.feature));
        }
    }
    std::error_code ec;
    fs::remove_all(cache, ec);

    std::string report { "{\"programs\":" + std::to_string(programs.size()) + ",\"files\":" + std::to_string(files) + ",\"features\":{" };
    bool first { true };
    std::println("{:<26} {:>5} {:>6} {:>5} {:>9} {:>7}", "feature", "found", "missed", "wrong", "precision", "recall");
    for (const auto& [feature, t] : tally) {
        const double precision { t.found + t.wrong == 0 ? 1.0 : double(t.found) / (t.found + t.wrong) };
        const double recall { t.found + t.missed == 0 ? 1.0 : double(t.found) / (t.found + t.missed) };
        std::println("{:<26} {:>5} {:>6} {:>5} {:>8.1f}% {:>6.1f}%", feature, t.found, t.missed, t.wrong, precision * 100, recall * 100);
        report += std::format("{}\"{}\":{{\"found\":{},\"missed\":{},\"wrong\":{},\"precision\":{},\"recall\":{}}}", first ? "" : ",", feature, t.found,
                              t.missed, t.wrong, precision, recall);
        first = false;
    }
    report += "},\"problems\":[";
    for (std::size_t i { 0 }; i < problems.size(); ++i) {
        std::string escaped;
        for (const char c : problems[i]) {
            if (c == '"' || c == '\\') escaped += '\\';
            escaped += c;
        }
        report += std::format("{}\"{}\"", i ? "," : "", escaped);
    }
    report += "]}\n";
    for (const auto& p : problems) std::println(std::cerr, "  {}", p);
    std::println("{} programs, {} files, {} problems", programs.size(), files, problems.size());
    if (!json.empty()) std::ofstream { json } << report;
    return problems.empty() ? 0 : 1;
}
