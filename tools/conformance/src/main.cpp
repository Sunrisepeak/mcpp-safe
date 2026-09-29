// mcxx-conformance: MC++'s gate fixtures, checked.
//
//   mcxx-conformance [--json FILE] [--driver MCXX] [DIR]   default DIR: conformance/gates
//
// Every directory with sources is one program (its files compile together; module interfaces are
// built as its importers need them), under the nearest mcpp.toml's [package.metadata.mcxx]. A line
// that must be reported carries `// expect: <feature> [<feature> ...] [-- a note]`; every gate diagnostic must
// be expected and every expectation reported, on that line. Anything else the compiler says is an
// error is a broken fixture. The report counts, per feature, what was found, missed and wrong.
//
// A finding a declaration waives is marked `// expect-waived: <feature>`: it must be in the audit
// (MCXX_AUDIT, which the runner sets), and every audited waiver must be expected (A0.3.4). The JSON
// report carries each program's time and the total (A0.8.2).
//
// With --driver, every program is also compiled by that mcxx as a build compiles it -- its module
// interfaces precompiled, its units checked -- and the gate findings of the two paths, the editor's
// and the build's, must be the same set (A1.4.2).
import std;
import mcxx.msa;
import mcxx.plugin;
import mcxx.backend;
import mcxx.plugins.std;
import mcxx.plugins.libs;

extern "C" int setenv(const char* name, const char* value, int overwrite);   // POSIX: MCXX_AUDIT for the gates
extern "C" int unsetenv(const char* name);

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

std::vector<std::string> expectations(std::string_view line, std::string_view marker = "// expect:") {
    std::vector<std::string> out;
    const auto at = line.find(marker);
    if (at == std::string_view::npos) return out;
    std::string rest { line.substr(at + marker.size()) };
    if (const auto note = rest.find("--"); note != std::string::npos) rest.erase(note);   // `-- why`, for readers
    std::ranges::replace(rest, ',', ' ');
    std::istringstream in { rest };
    for (std::string id; in >> id;) out.push_back(id);
    return out;
}

// Runs a command line, its standard error captured (for the driver comparison).
std::pair<int, std::string> run(const std::vector<std::string>& argv, const fs::path& errors) {
    std::string line;
    for (const auto& a : argv) line += "'" + a + "' ";
    const int status { std::system((line + "2> '" + errors.generic_string() + "' > /dev/null").c_str()) };
    std::ifstream in { errors };
    return { status, std::string { std::istreambuf_iterator<char> { in }, {} } };
}

// The gate findings in a compiler's output: `file:line:col: error|warning: ... [feature] ...`.
std::set<std::tuple<std::string, std::uint32_t, std::string>> gate_findings(std::string_view output) {
    std::set<std::tuple<std::string, std::uint32_t, std::string>> out;
    std::istringstream lines { std::string { output } };
    for (std::string line; std::getline(lines, line);) {
        const auto w = line.find(": warning: "), e = line.find(": error: ");
        const auto at = std::min(w, e);
        if (at == std::string::npos) continue;
        const std::string place { line.substr(0, at) };
        const auto c2 = place.rfind(':');
        const auto c1 = c2 == std::string::npos ? std::string::npos : place.rfind(':', c2 - 1);
        if (c1 == std::string::npos) continue;
        for (std::size_t open { line.find('[', at) }; open != std::string::npos; open = line.find('[', open + 1)) {
            const auto close = line.find(']', open);
            if (close == std::string::npos) break;
            const std::string id { line.substr(open + 1, close - open - 1) };
            if (mcxx::plugin::find_feature(id) != nullptr) {
                out.emplace(fs::weakly_canonical(place.substr(0, c1)).generic_string(), static_cast<std::uint32_t>(std::stoul(place.substr(c1 + 1, c2 - c1 - 1)) - 1), id);
                break;
            }
        }
    }
    return out;
}

std::string read(const fs::path& p) {
    std::ifstream in { p };
    return { std::istreambuf_iterator<char> { in }, {} };
}

} // namespace

int main(int argc, char** argv) {
    std::string json, driver;
    fs::path root { "conformance/gates" };
    for (int i { 1 }; i < argc; ++i) {
        const std::string_view a { argv[i] };
        if (a == "--json" && i + 1 < argc) json = argv[++i];
        else if (a == "--driver" && i + 1 < argc) driver = argv[++i];
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
    // Waivers: expected and audited (the gates append one JSON line per waived finding).
    fs::create_directories(cache);
    const fs::path audit { cache / "audit.jsonl" };
    setenv("MCXX_AUDIT", audit.generic_string().c_str(), 1);
    std::set<Mark> expectedWaivers;
    Tally waiverTally;
    const auto started = std::chrono::steady_clock::now();
    std::string timings;
    std::size_t driverCompared { 0 };
    std::vector<std::string> driverUnreached;   // files the build does not compile: an interface they import has a gate error
    for (auto& [dir, sources] : programs) {
        const auto programStarted = std::chrono::steady_clock::now();
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
            for (std::string line; std::getline(lines, line); ++n) {
                for (auto& id : expectations(line)) expected.insert({ rel, n, std::move(id) });
                for (auto& id : expectations(line, "// expect-waived:")) expectedWaivers.insert({ rel, n, std::move(id) });
            }
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
        if (!driver.empty()) {
            // The build's path: interfaces precompiled (the gates run where a source is parsed), in
            // an order that satisfies their imports, then the other units checked.
            unsetenv("MCXX_AUDIT");
            const fs::path out { cache / "driver" / fs::relative(dir, root) };
            fs::create_directories(out);
            // An interface a gate fails is not written (a compilation with errors leaves no output), so
            // a file importing it is not compiled by the build: such files are named, not compared.
            std::vector<std::string> base { driver, "c++", "-std=c++23", "--target=x86_64-unknown-linux-gnu", "-fprebuilt-module-path=" + out.generic_string() };
            std::set<std::string> unreached;
            std::set<std::tuple<std::string, std::uint32_t, std::string>> built;
            std::vector<fs::path> interfaces, others;
            std::map<fs::path, std::string> lastOutput;   // an interface not written yet: what its last try said
            for (const auto& src : sources) (src.extension() == ".cppm" ? interfaces : others).push_back(src);
            for (bool progress { true }; progress && !interfaces.empty();) {
                progress = false;
                for (auto it = interfaces.begin(); it != interfaces.end();) {
                    std::string name { read(*it) };
                    const auto m = name.find("export module ");
                    name = m == std::string::npos ? it->stem().string() : name.substr(m + 14, name.find(';', m) - m - 14);
                    std::ranges::replace(name, ':', '-');
                    auto args = base;
                    args.insert(args.end(), { "--precompile", it->generic_string(), "-o", (out / (name + ".pcm")).generic_string() });
                    const auto [status, text] = run(args, out / "errors.txt");
                    lastOutput[*it] = text;
                    const bool missingImport { text.contains("fatal error: module '") && text.contains("' not found") };
                    if (fs::exists(out / (name + ".pcm")) || !missingImport) {
                        for (auto& f : gate_findings(text)) built.insert(f);
                        if (fs::exists(out / (name + ".pcm")) || !missingImport) {   // written, or failed on its own findings: done
                            it = interfaces.erase(it);
                            progress = true;
                            continue;
                        }
                    }
                    ++it;
                }
            }
            for (const auto& src : interfaces) unreached.insert(fs::weakly_canonical(src).generic_string());   // only a missing import left them
            for (const auto& src : others) {
                auto args = base;
                args.insert(args.end(), { "-fsyntax-only", src.generic_string() });
                const auto [status, text] = run(args, out / "errors.txt");
                if (text.contains("fatal error: module '") && text.contains("' not found")) {
                    unreached.insert(fs::weakly_canonical(src).generic_string());
                    continue;
                }
                for (auto& f : gate_findings(text)) built.insert(f);
            }
            std::set<std::tuple<std::string, std::uint32_t, std::string>> editor;
            for (const auto& m : reported)
                if (!unreached.contains(fs::weakly_canonical(root / m.file).generic_string()))
                    editor.emplace(fs::weakly_canonical(root / m.file).generic_string(), m.line, m.feature);
            for (const auto& u : unreached) driverUnreached.push_back(fs::relative(u, root).generic_string());
            for (const auto& f : built)
                if (!editor.contains(f))
                    problems.push_back(std::format("{}:{}: the build reports {}, the editor does not", fs::relative(std::get<0>(f), root).generic_string(),
                                                   std::get<1>(f) + 1, std::get<2>(f)));
            for (const auto& f : editor)
                if (!built.contains(f))
                    problems.push_back(std::format("{}:{}: the editor reports {}, the build does not", fs::relative(std::get<0>(f), root).generic_string(),
                                                   std::get<1>(f) + 1, std::get<2>(f)));
            driverCompared += built.size();
            setenv("MCXX_AUDIT", audit.generic_string().c_str(), 1);
        }
        const double seconds { std::chrono::duration<double>(std::chrono::steady_clock::now() - programStarted).count() };
        timings += std::format("{}{{\"program\":\"{}\",\"files\":{},\"seconds\":{:.3f}}}", timings.empty() ? "" : ",",
                               fs::relative(dir, root).generic_string(), sources.size(), seconds);
    }
    // Every waiver the gates recorded, against the waivers the fixtures expect.
    std::set<Mark> audited;
    {
        std::ifstream in { audit };
        for (std::string line; std::getline(in, line);) {
            auto field = [&](std::string_view key) -> std::string {
                const auto at = line.find(std::format("\"{}\":", key));
                if (at == std::string::npos) return {};
                std::size_t b { at + key.size() + 3 };
                if (line[b] == '"') {
                    const auto e = line.find('"', b + 1);
                    return line.substr(b + 1, e - b - 1);
                }
                const auto e = line.find_first_of(",}", b);
                return line.substr(b, e - b);
            };
            const std::string file { field("path") };
            if (file.empty()) continue;
            audited.insert({ fs::relative(file, root).generic_string(), static_cast<std::uint32_t>(std::stoul(field("line")) - 1), field("feature") });
        }
    }
    for (const auto& m : expectedWaivers) {
        if (audited.contains(m)) ++waiverTally.found;
        else {
            ++waiverTally.missed;
            problems.push_back(std::format("{}:{}: the waiver of {} is not in the audit", m.file, m.line + 1, m.feature));
        }
    }
    for (const auto& m : audited)
        if (!expectedWaivers.contains(m)) {
            ++waiverTally.wrong;
            problems.push_back(std::format("{}:{}: {} was waived where nothing expects a waiver", m.file, m.line + 1, m.feature));
        }
    const double total { std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count() };
    std::error_code ec;
    fs::remove_all(cache, ec);

    std::string report { std::format("{{\"programs\":{},\"files\":{},\"seconds\":{:.3f},\"timings\":[{}],\"waivers\":{{\"found\":{},\"missed\":{},\"wrong\":{}}},\"features\":{{",
                                     programs.size(), files, total, timings, waiverTally.found, waiverTally.missed, waiverTally.wrong) };
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
    std::println("waivers: {} audited as expected, {} missing, {} unexpected", waiverTally.found, waiverTally.missed, waiverTally.wrong);
    if (!driver.empty()) {
        std::println("the build (mcxx c++) and the editor: {} gate findings compared, the same", driverCompared);
        for (const auto& u : driverUnreached) std::println("  not compiled by the build (an interface it imports has a gate error): {}", u);
    }
    std::println("{} programs, {} files, {} problems, {:.1f} s", programs.size(), files, problems.size(), total);
    if (!json.empty()) std::ofstream { json } << report;
    return problems.empty() ? 0 : 1;
}
