// mcxx-lexdump FILE...: every token MC++'s lexer finds, comments included, one JSON object per line:
// {"file":..., "kind":..., "line":..., "column":..., "text":...} -- the text as written, splices kept.
// mcxx-lexdump --bench N FILE...: lexes the files N times, prints bytes, tokens and the best time.
// mcxx-lexdump --pp [OPTIONS] FILE: the file preprocessed by mcxx.frontend:preprocess, one JSON
// object: certain, the notes, the module, the includes, the macros and the tokens' spellings.
// mcxx-lexdump --ppdiff [OPTIONS] FILE CLANG_E: that against Clang's `-E` output for the same file
// (tools/checks/ppdiff.py): the main file's lines of it (by its line markers), lexed, token for token.
// mcxx-lexdump --syntax [OPTIONS] FILE: the file's outline from mcxx.frontend:syntax, one JSON object,
// as `mcxx-probe --symbols` prints Clang's (tools/checks/syntaxdiff.py), with the parser's diagnostics.
// mcxx-lexdump --facts [OPTIONS] FILE: the file's declarations as MC++'s own front end gives them (MC3
// T1, facts(syntax)), one JSON object: {"declarations": [...]} with MC3's members, as `mcxx-probe
// --facts` prints the Clang backend's (tools/checks/declsdiff.py, M2.1).
// mcxx-lexdump --references [OPTIONS] FILE: each name the file writes that the front end resolves and
// what it names (mcxx.frontend:lookup), as `mcxx-probe --references` prints Clang's (refsdiff.py).
// mcxx-lexdump --fuzz N FILE...: each file cut short or given random tokens and bytes, N times each,
// parsed every time, its outline and its facts taken; the process ending is the pass (A1.7.3). Prints the counts.
// mcxx-lexdump --parse-bench N FILE...: lexing, preprocessing and parsing the files, N times; the best.
// mcxx-lexdump --directives FILE: the file's directive lines, as the lexer finds them (not in a raw
// string or a comment), less #error, #warning and the #defines and #undefs after its last #include,
// and the names it #defines: what ppdiff.py preprocesses to learn the headers' macros.
// OPTIONS: --target T, -DNAME[=VALUE], -UNAME, --header-macros FILE (the headers' macros, as `-dM -E`
// prints them: PreprocessOptions::header_macros, complete).
import std;
import mcxx.msa;
import mcxx.frontend;
import mcxx.base;
import mcxx.ifc;

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

int syntax(int argc, char** argv) {
    mcxx::frontend::PreprocessOptions options;
    std::string file;
    for (int i { 2 }; i < argc; ++i) {
        const std::string_view a { argv[i] };
        if (a == "--target" && i + 1 < argc) options.target = argv[++i];
        else if (a == "--header-macros" && i + 1 < argc) {
            std::istringstream lines { read(argv[++i]) };
            for (std::string l; std::getline(lines, l);) options.header_macros.push_back(l);
            options.header_macros_complete = true;
        } else if (a.starts_with("-D")) options.defines.emplace_back(a.substr(2));
        else if (a.starts_with("-U")) options.undefines.emplace_back(a.substr(2));
        else file = a;
    }
    options.file = file;
    const std::string text { read(file) };
    const auto started = std::chrono::steady_clock::now();
    const auto parsed = mcxx::frontend::parse(text, options);
    const auto outline = mcxx::frontend::symbols(parsed);
    const double seconds { std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count() };
    std::string out { "[" };
    int count { 0 };
    const std::function<void(const std::vector<mcxx::msa::Symbol>&, int)> walk = [&](const std::vector<mcxx::msa::Symbol>& ss, int parent) {
        for (const auto& s : ss) {
            const int self { count++ };
            out += std::format("{}{{\"kind\":\"{}\",\"name\":{},\"range\":[{},{},{},{}],\"selection\":[{},{},{},{}],\"parent\":{}}}", self ? "," : "",
                               mcxx::msa::to_string(s.kind), json(s.name), s.range.begin.line, s.range.begin.column, s.range.end.line, s.range.end.column,
                               s.selection.begin.line, s.selection.begin.column, s.selection.end.line, s.selection.end.column, parent);
            walk(s.children, self);
        }
    };
    walk(outline, -1);
    out += "]";
    std::vector<std::string> notes;
    for (const auto& d : parsed.diagnostics) notes.push_back(std::format("{}:{}: {}", d.at.line, d.at.column, d.message));
    std::println("{{\"symbols\":{},\"diagnostics\":{},\"certain\":{},\"seconds\":{:.6f}}}", out, strings(notes), parsed.pp.certain, seconds);
    return 0;
}

std::string range_json(const mcxx::msa::Range& r) {
    return std::format("{{\"begin\":{{\"line\":{},\"column\":{}}},\"end\":{{\"line\":{},\"column\":{}}}}}", r.begin.line, r.begin.column, r.end.line,
                       r.end.column);
}

int facts(int argc, char** argv) {
    mcxx::frontend::PreprocessOptions options;
    std::string file;
    for (int i { 2 }; i < argc; ++i) {
        const std::string_view a { argv[i] };
        if (a == "--target" && i + 1 < argc) options.target = argv[++i];
        else if (a == "--header-macros" && i + 1 < argc) {
            std::istringstream lines { read(argv[++i]) };
            for (std::string l; std::getline(lines, l);) options.header_macros.push_back(l);
            options.header_macros_complete = true;
        } else if (a.starts_with("-D")) options.defines.emplace_back(a.substr(2));
        else if (a.starts_with("-U")) options.undefines.emplace_back(a.substr(2));
        else file = a;
    }
    options.file = file;
    const std::string text { read(file) };
    const auto parsed = mcxx::frontend::parse(text, options);
    const auto f = mcxx::frontend::facts(parsed);
    std::string out { "{\"declarations\":[" };
    bool first { true };
    for (const auto& d : f.declarations) {
        std::string kind { mcxx::msa::to_string(d.kind) };
        std::ranges::replace(kind, ' ', '-');
        std::string list;
        for (const auto& t : d.templates) list += (list.empty() ? "" : ",") + json(t);
        out += std::format("{}{{\"range\":{},\"name\":{},\"container\":{},\"qualified-name\":{},\"kind\":\"{}\",\"type\":{},\"templates\":[{}],"
                           "\"exported\":{},\"c-array\":{},\"pointer\":{},\"union\":{},\"c-variadic\":{},\"local\":{}}}",
                           first ? "" : ",", range_json(d.range), range_json(d.name), json(d.container), json(d.qualified_name), kind, json(d.type), list,
                           d.exported, d.c_array, d.pointer, d.is_union, d.c_variadic, d.local);
        first = false;
    }
    out += std::format("],\"certain\":{}}}", f.certainty == mcxx::msa::Certainty::certain);
    std::println("{}", out);
    return 0;
}

// Each name the file writes that MC++'s front end resolves (mcxx.frontend:lookup), as `mcxx-probe
// --references` prints the Clang backend's (tools/checks/refsdiff.py, M2.1).
// What the file's imports bring in, from the modules' MC2 interfaces (M2.2), found as Clang finds
// the modules: `-fmodule-file=m=X.pcm`, or `m.pcm` (`m-p.pcm` for m:p) on a `-fprebuilt-module-path`;
// the interface beside the BMI or kept for it (mcxx::ifc::interface_for). An implementation unit sees
// all of its own module's interface and what that imports; an importer, what another module exports
// and re-exports.
mcxx::frontend::Imported imported_by(const mcxx::frontend::Syntax& syntax, const std::map<std::string, std::string, std::less<>>& module_files,
                                     const std::vector<std::string>& prebuilt) {
    mcxx::frontend::Imported out;
    const auto& pp = syntax.pp;
    const std::string own { pp.module.present ? pp.module.name : std::string {} };
    std::set<std::string> seen;
    const auto bmi_of = [&](const std::string& name) -> std::string {
        if (const auto it = module_files.find(name); it != module_files.end()) return it->second;
        std::string file { name };
        std::ranges::replace(file, ':', '-');
        for (const auto& dir : prebuilt)
            if (std::filesystem::exists(dir + "/" + file + ".pcm")) return dir + "/" + file + ".pcm";
        return {};
    };
    const std::function<void(const std::string&)> add = [&](const std::string& name) {
        if (name.empty() || name.starts_with('<') || name.starts_with('"') || !seen.insert(name).second) return;
        const std::string bmi { bmi_of(name) };
        if (bmi.empty()) return;
        const auto iface { mcxx::ifc::interface_for(bmi) };
        if (!iface) return;
        const bool same_module { !own.empty() && (name == own || name.starts_with(own + ":")) };
        for (const auto& d : iface->declarations)
            if (same_module || d.exported) out.declarations.push_back(d);
        for (const auto& reexported : iface->reexports) add(reexported);
    };
    // An implementation unit (`module m;`) imports its module's interface.
    if (pp.module.present && !pp.module.exported && pp.module.partition.empty()) add(own);
    for (const auto& import : pp.imports) add(import.name.starts_with(':') ? own + import.name : import.name);
    return out;
}

int references(int argc, char** argv) {
    mcxx::frontend::PreprocessOptions options;
    std::string file;
    std::map<std::string, std::string, std::less<>> module_files;
    std::vector<std::string> prebuilt;
    for (int i { 2 }; i < argc; ++i) {
        const std::string_view a { argv[i] };
        if (a == "--target" && i + 1 < argc) options.target = argv[++i];
        else if (a.starts_with("-D")) options.defines.emplace_back(a.substr(2));
        else if (a.starts_with("-U")) options.undefines.emplace_back(a.substr(2));
        else if (a.starts_with("-fmodule-file=") && a.find('=', 14) != std::string_view::npos) {
            const auto rest { a.substr(14) };
            const auto eq { rest.find('=') };
            module_files.insert_or_assign(std::string { rest.substr(0, eq) }, std::string { rest.substr(eq + 1) });
        } else if (a.starts_with("-fprebuilt-module-path=")) prebuilt.emplace_back(a.substr(23));
        else file = a;
    }
    options.file = file;
    const std::string text { read(file) };
    const auto parsed = mcxx::frontend::parse(text, options);
    const auto imported { imported_by(parsed, module_files, prebuilt) };
    std::string out { "{\"references\":[" };
    bool first { true };
    for (const auto& r : mcxx::frontend::references(parsed, imported)) {
        std::string declaration { "null" };
        if (r.declaration >= 0) {
            const auto at = mcxx::frontend::selection_range(parsed, parsed.declarations[static_cast<std::size_t>(r.declaration)]);
            declaration = std::format("[{},{}]", at.begin.line, at.begin.column);
        }
        out += std::format("{}{{\"range\":[{},{},{},{}],\"name\":{},\"target\":{},\"kind\":\"{}\",\"declaration\":{}}}", first ? "" : ",",
                           r.range.begin.line, r.range.begin.column, r.range.end.line, r.range.end.column, json(r.name), json(r.target),
                           mcxx::msa::to_string(r.kind), declaration);
        first = false;
    }
    out += "]}";
    std::println("{}", out);
    return 0;
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

int fuzz(int rounds, int argc, char** argv) {
    // Fragments a mutation inserts: the tokens that open and close what a parser tracks, and noise.
    static constexpr std::string_view FRAGMENTS[] {
        "{", "}", "(", ")", "[", "]", "<", ">", ">>", ";", ",", "::", "template", "class", "struct", "enum", "namespace", "export",
        "module", "import", "using", "operator", "~", "=", "#define X(", "#if 1\n", "#endif\n", "\"", "'", "R\"(", "/*", "\\\n", "\xff",
        "requires", "decltype(", "->", "...", "friend", "typedef", "extern \"C\"", "[[", "]]", "&&", "*", "\n",
    };
    std::mt19937_64 random { 20260929 };
    std::size_t parses { 0 }, declarations { 0 }, facts { 0 }, references { 0 };
    for (int i { 3 }; i < argc; ++i) {
        const std::string text { read(argv[i]) };
        for (int r { 0 }; r < rounds; ++r) {
            std::string mutated { text };
            const auto at = [&] { return mutated.empty() ? std::size_t { 0 } : static_cast<std::size_t>(random() % (mutated.size() + 1)); };
            switch (random() % 3) {
            case 0: mutated.resize(at()); break;   // cut short
            case 1:
                for (int k { 0 }, n { static_cast<int>(1 + random() % 4) }; k < n; ++k) {
                    const auto& f = FRAGMENTS[random() % std::size(FRAGMENTS)];
                    mutated.insert(at(), f);
                }
                break;
            default:
                for (int k { 0 }, n { static_cast<int>(1 + random() % 8) }; k < n; ++k) mutated.insert(at(), 1, static_cast<char>(random() % 256));
            }
            const auto parsed = mcxx::frontend::parse(mutated, { .file = argv[i] });
            declarations += mcxx::frontend::symbols(parsed).size();
            // And the facts the editor's quick gates read on every edit: types as text among them.
            facts += mcxx::frontend::facts(parsed).declarations.size();
            references += mcxx::frontend::references(parsed).size();   // and the names it resolves
            ++parses;
        }
    }
    std::println("{{\"parses\":{},\"symbols\":{},\"facts\":{},\"references\":{}}}", parses, declarations, facts, references);
    return 0;
}

int parse_bench(int rounds, int argc, char** argv) {
    std::vector<std::string> texts;
    std::size_t bytes { 0 };
    for (int i { 3 }; i < argc; ++i) bytes += texts.emplace_back(read(argv[i])).size();
    double best { 1e9 };
    std::size_t symbols { 0 };
    for (int r { 0 }; r < rounds; ++r) {
        const auto started = std::chrono::steady_clock::now();
        symbols = 0;
        for (const auto& t : texts) symbols += mcxx::frontend::parse(t).declarations.size();
        best = std::min(best, std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count());
    }
    std::println("{{\"files\":{},\"bytes\":{},\"declarations\":{},\"seconds\":{:.4f},\"mb-per-second\":{:.1f}}}", texts.size(), bytes, symbols, best,
                 static_cast<double>(bytes) / best / 1e6);
    return 0;
}

int main(int argc, char** argv) {
    mcxx::base::trace::configure_from_environment();   // MCXX_LOG=frontend.syntax=debug: what the parser decides
    if (argc > 3 && std::string_view { argv[1] } == "--fuzz") return fuzz(std::stoi(argv[2]), argc, argv);
    if (argc > 3 && std::string_view { argv[1] } == "--parse-bench") return parse_bench(std::stoi(argv[2]), argc, argv);
    if (argc > 2 && std::string_view { argv[1] } == "--directives") return directives(argv[2]);
    if (argc > 2 && std::string_view { argv[1] } == "--syntax") return syntax(argc, argv);
    if (argc > 2 && std::string_view { argv[1] } == "--facts") return facts(argc, argv);
    if (argc > 2 && std::string_view { argv[1] } == "--references") return references(argc, argv);
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
