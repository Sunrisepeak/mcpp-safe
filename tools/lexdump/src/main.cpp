// mcxx-lexdump FILE...: every token MC++'s lexer finds, comments included, one JSON object per line:
// {"file":..., "kind":..., "line":..., "column":..., "text":...} -- the text as written, splices kept.
// mcxx-lexdump --bench N FILE...: lexes the files N times, prints bytes, tokens and the best time.
// mcxx-lexdump --pp [OPTIONS] FILE: the file preprocessed by mcxx.frontend:preprocess, one JSON
// object: certain, the notes, the module, the includes, the macros and the tokens' spellings.
// mcxx-lexdump --ppdiff [OPTIONS] FILE CLANG_E: that against Clang's `-E` output for the same file
// (tools/checks/ppdiff.py): the main file's lines of it (by its line markers), lexed, token for token.
// mcxx-lexdump --directives FILE: the file's directive lines, as the lexer finds them (not in a raw
// string or a comment), less #error, #warning and the #defines and #undefs after its last #include,
// and the names it #defines: what ppdiff.py preprocesses to learn the headers' macros.
// OPTIONS: --target T, -DNAME[=VALUE], -UNAME, --header-macros FILE (the headers' macros, as `-dM -E`
// prints them: PreprocessOptions::header_macros, complete).
import std;
import mcxx.frontend;

namespace {

std::string json(std::string_view s) {
    std::string out { "\"" };
    for (const char c : s) {
        switch (c) {
        case '"': out += "\\\""; break;
        case '\\': out += "\\\\"; break;
        case '\n': out += "\\n"; break;
        case '\r': out += "\\r"; break;
        case '\t': out += "\\t"; break;
        default:
            if (static_cast<unsigned char>(c) < 0x20) out += std::format("\\u{:04x}", static_cast<unsigned char>(c));
            else out += c;
        }
    }
    return out + "\"";
}

} // namespace

int bench(int rounds, int argc, char** argv) {
    std::vector<std::string> texts;
    std::size_t bytes { 0 };
    for (int i { 3 }; i < argc; ++i) {
        std::ifstream in { argv[i], std::ios::binary };
        texts.emplace_back(std::istreambuf_iterator<char> { in }, std::istreambuf_iterator<char> {});
        bytes += texts.back().size();
    }
    double best { 1e9 };
    std::size_t tokens { 0 };
    for (int r { 0 }; r < rounds; ++r) {
        const auto started = std::chrono::steady_clock::now();
        tokens = 0;
        for (const auto& t : texts) tokens += mcxx::frontend::lex(t, { .whitespace = false, .comments = true }).size();
        best = std::min(best, std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count());
    }
    std::println("{{\"lexer\":\"mcxx.frontend\",\"bytes\":{},\"tokens\":{},\"seconds\":{:.4f},\"mb-per-second\":{:.1f}}}", bytes, tokens, best,
                 static_cast<double>(bytes) / best / 1e6);
    return 0;
}

std::string read(const std::string& path) {
    std::ifstream in { path, std::ios::binary };
    return { std::istreambuf_iterator<char> { in }, std::istreambuf_iterator<char> {} };
}

std::string strings(const std::vector<std::string>& v) {
    std::string out { "[" };
    for (std::size_t i { 0 }; i < v.size(); ++i) out += (i > 0 ? "," : "") + json(v[i]);
    return out + "]";
}

// The text of `main`'s own lines in a -E output, by its line markers (# N "file" flags); #pragma lines out.
std::string main_lines(std::string_view output, std::string_view main) {
    std::string out;
    bool in_main { false };
    std::size_t at { 0 };
    while (at < output.size()) {
        std::size_t end { output.find('\n', at) };
        if (end == std::string_view::npos) end = output.size();
        std::string_view line { output.substr(at, end - at) };
        // Clang 23.1 can print a line marker right after a module directive's `;`
        // (`__preprocessed_module;# 61 "file"`): that is two lines.
        if (const auto split = line.find(";# "); split != std::string_view::npos && split + 3 < line.size() &&
            std::isdigit(static_cast<unsigned char>(line[split + 3]))) {
            end = at + split + 1;
            line = output.substr(at, split + 1);
            at = end;
        } else {
            at = end + 1;
        }
        if (line.starts_with("# ") && line.size() > 2 && std::isdigit(static_cast<unsigned char>(line[2]))) {
            const auto q = line.find('"');
            const auto r = line.rfind('"');
            if (q != std::string_view::npos && r > q) in_main = line.substr(q + 1, r - q - 1) == main;
            out += '\n';
            continue;
        }
        if (in_main && !line.starts_with("#pragma")) out += line;
        out += '\n';
    }
    return out;
}

int preprocessed(bool diff, int argc, char** argv) {
    mcxx::frontend::PreprocessOptions options;
    std::vector<std::string> files;
    for (int i { 2 }; i < argc; ++i) {
        const std::string_view a { argv[i] };
        if (a == "--target" && i + 1 < argc) options.target = argv[++i];
        else if (a == "--header-macros" && i + 1 < argc) {
            std::istringstream lines { read(argv[++i]) };
            for (std::string l; std::getline(lines, l);) options.header_macros.push_back(l);
            options.header_macros_complete = true;
        }
        else if (a.starts_with("-D")) options.defines.emplace_back(a.substr(2));
        else if (a.starts_with("-U")) options.undefines.emplace_back(a.substr(2));
        else files.emplace_back(a);
    }
    if (files.empty() || (diff && files.size() < 2)) {
        std::println(std::cerr, "usage: mcxx-lexdump --pp|--ppdiff [--target T] [-DX[=V]] [-UX] FILE [CLANG_E]");
        return 2;
    }
    options.file = files[0];
    const std::string text { read(files[0]) };
    const auto pp = mcxx::frontend::preprocess(text, options);
    std::vector<std::string> ours, notes;
    for (const auto& t : pp.tokens) ours.emplace_back(t.spelling);
    for (const auto& d : pp.diagnostics) notes.push_back(std::format("{}:{}: {}", d.at.line, d.at.column, d.message));
    if (!diff) {
        std::vector<std::string> includes, macros;
        for (const auto& i : pp.includes) includes.push_back(i.header);
        for (const auto& m : pp.macros) macros.push_back(m.name);
        std::println("{{\"file\":{},\"certain\":{},\"module\":{},\"includes\":{},\"macros\":{},\"notes\":{},\"tokens\":{}}}", json(files[0]),
                     pp.certain, json(pp.module.name + (pp.module.partition.empty() ? "" : ":" + pp.module.partition)), strings(includes), strings(macros),
                     strings(notes), strings(ours));
        return 0;
    }
    const std::string reference { main_lines(read(files[1]), files[0]) };
    std::vector<std::string> theirs;
    // Clang's -E spells the keywords of module and import directives so that its output is not read
    // as directives again.
    for (const auto& t : mcxx::frontend::lex(reference)) {
        auto s = mcxx::frontend::spelling(reference, t);
        if (s == "__preprocessed_module") s = "module";
        else if (s == "__preprocessed_import") s = "import";
        theirs.push_back(std::move(s));
    }
    std::size_t i { 0 };
    while (i < ours.size() && i < theirs.size() && ours[i] == theirs[i]) ++i;
    const bool equal { i == ours.size() && i == theirs.size() };
    // A difference at a name the file does not define, after an #include: a header's macro, which
    // this preprocessor does not read.
    std::string kind { equal ? "equal" : pp.certain ? "certain" : "uncertain" };
    if (!equal && i < ours.size() && !pp.includes.empty()) {
        const auto& t = pp.tokens[i];
        const bool own { std::ranges::any_of(pp.macros, [&](const auto& m) { return m.name == t.spelling; }) };
        if (t.kind == mcxx::frontend::Kind::raw_identifier && !own) kind = "header-macro";
    }
    const auto around = [](const std::vector<std::string>& v, std::size_t at) {
        std::vector<std::string> out;
        for (std::size_t k { at }; k < v.size() && k < at + 6; ++k) out.push_back(v[k]);
        return strings(out);
    };
    std::println("{{\"file\":{},\"verdict\":\"{}\",\"tokens\":{},\"at\":{},\"line\":{},\"ours\":{},\"clang\":{},\"notes\":{}}}", json(files[0]), kind,
                 theirs.size(), i, i < pp.tokens.size() ? pp.tokens[i].at.line : 0, around(ours, i), around(theirs, i), strings(notes));
    return 0;
}

int directives(const char* path) {
    const std::string text { read(path) };
    const auto tokens = mcxx::frontend::lex(text);
    struct Line {
        std::string_view name;
        std::string_view text;
        std::string_view defined;
    };
    std::vector<Line> lines;
    for (std::size_t i { 0 }; i < tokens.size(); ++i) {
        if (!tokens[i].start_of_line || tokens[i].kind != mcxx::frontend::Kind::hash) continue;
        std::size_t end { i + 1 };
        while (end < tokens.size() && !tokens[end].start_of_line) ++end;
        const auto word = [&](std::size_t k) {
            return k < end && tokens[k].kind == mcxx::frontend::Kind::raw_identifier
                       ? std::string_view { text }.substr(tokens[k].begin, tokens[k].end - tokens[k].begin)
                       : std::string_view {};
        };
        lines.push_back({ word(i + 1), std::string_view { text }.substr(tokens[i].begin, tokens[end - 1].end - tokens[i].begin),
                          word(i + 1) == "define" ? word(i + 2) : std::string_view {} });
        i = end - 1;
    }
    std::size_t last_include { 0 };
    for (std::size_t k { 0 }; k < lines.size(); ++k)
        if (lines[k].name == "include") last_include = k + 1;
    std::string out;
    std::vector<std::string> own;
    for (std::size_t k { 0 }; k < lines.size(); ++k) {
        const auto& l = lines[k];
        if (!l.defined.empty()) own.emplace_back(l.defined);
        if (l.name == "error" || l.name == "warning" || ((l.name == "define" || l.name == "undef") && k >= last_include)) continue;
        out += l.text;
        out += '\n';
    }
    std::println("{{\"includes\":{},\"text\":{},\"own\":{}}}", last_include > 0, json(out), strings(own));
    return 0;
}

int main(int argc, char** argv) {
    if (argc > 2 && std::string_view { argv[1] } == "--directives") return directives(argv[2]);
    if (argc > 2 && std::string_view { argv[1] } == "--bench") return bench(std::stoi(argv[2]), argc, argv);
    if (argc > 1 && (std::string_view { argv[1] } == "--pp" || std::string_view { argv[1] } == "--ppdiff")) return preprocessed(std::string_view { argv[1] } == "--ppdiff", argc, argv);
    for (int i { 1 }; i < argc; ++i) {
        std::ifstream in { argv[i], std::ios::binary };
        const std::string text { std::istreambuf_iterator<char> { in }, {} };
        for (const auto& t : mcxx::frontend::lex(text, { .whitespace = false, .comments = true }))
            std::println("{{\"file\":{},\"kind\":\"{}\",\"line\":{},\"column\":{},\"text\":{}}}", json(argv[i]), mcxx::frontend::name(t.kind), t.line, t.column,
                         json(std::string_view { text }.substr(t.begin, t.end - t.begin)));
    }
    return 0;
}
