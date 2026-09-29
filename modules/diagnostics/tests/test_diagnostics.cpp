// The two views of a diagnostic (MC5 §9), from Items as a backend makes them.
import std;
import nlohmann.json;
import mcxx.testing;
import mcxx.msa;
import mcxx.diagnostics;

namespace d = mcxx::diagnostics;
namespace msa = mcxx::msa;

namespace {

d::Item gate_finding() {
    d::Item item;
    item.path = "src/a.cpp";
    item.lines = { "    int* p = nullptr;" };
    auto& x = item.diagnostic;
    x.range = { { 2, 8 }, { 2, 9 } };
    x.severity = msa::Severity::error;
    x.code = "raw-pointers";
    x.message = "`p` holds a raw pointer [raw-pointers]; use std::unique_ptr; to allow it here, [[mcpp::allow(\"raw-pointers\")]] on the declaration";
    x.headline = "`p` holds a raw pointer";
    x.fix = "use std::unique_ptr";
    x.waiver = "[[mcpp::allow(\"raw-pointers\", \"<why>\")]]";
    x.level = "deny";
    x.level_from = "profile `safe` (/p/mcpp.toml)";
    return item;
}

} // namespace

int main() {
    using namespace mcxx::testing;

    "a view is chosen by name, the option over the environment, human on a terminal"_test = [] {
        expect(d::view_named("human") == d::View::human && d::view_named("agent") == d::View::agent && d::view_named("clang") == d::View::clang);
        expect(!d::view_named("json").has_value());
        expect(d::view_for(d::View::agent, true) == d::View::agent);
        if (!d::view_from_environment()) {
            expect(d::view_for(std::nullopt, true) == d::View::human && d::view_for(std::nullopt, false) == d::View::clang);
        }
    };

    "the human view lays a finding out as Rust does: code, place, excerpt, help, where the level is set"_test = [] {
        const std::string text { d::human(gate_finding(), false) };
        expect(text.starts_with("error[raw-pointers]: `p` holds a raw pointer\n")) << text;
        expect(text.contains(" --> src/a.cpp:3:9\n")) << text;
        expect(text.contains("3 |     int* p = nullptr;\n")) << text;
        expect(text.contains("  |         ^\n")) << "the span underlined, under the name";
        expect(text.contains("= help: use std::unique_ptr\n") && text.contains("= help: to allow it here: [[mcpp::allow(\"raw-pointers\", \"<why>\")]]\n"));
        expect(text.contains("= note: deny, from profile `safe` (/p/mcpp.toml)\n"));
        expect(!text.contains("\x1b[")) << "no color unless asked";
        expect(d::human(gate_finding(), true).contains("\x1b[1;31merror")) << "red on a terminal";
    };

    "the agent view is one JSON object with the finding apart: code, range, level and its source, fix, waiver"_test = [] {
        const std::string line { d::agent(gate_finding()) };
        expect(line.ends_with("\n") && std::ranges::count(line, '\n') == 1);
        const auto j = nlohmann::json::parse(line);
        expect(j["mcxx-diagnostic"] == "0.1.0" && j["severity"] == "error" && j["code"] == "raw-pointers");
        expect(j["message"] == "`p` holds a raw pointer" && j["file"] == "src/a.cpp" && j["location"] == "src/a.cpp:3:9");
        expect(j["range"]["begin"]["line"] == 2 && j["range"]["begin"]["column"] == 8 && j["range"]["end"]["column"] == 9) << "MC3's positions, from 0";
        expect(j["level"] == "deny" && j["level-from"] == "profile `safe` (/p/mcpp.toml)");
        expect(j["fix"] == "use std::unique_ptr" && j["waiver"] == "[[mcpp::allow(\"raw-pointers\", \"<why>\")]]");
    };

    "a compiler's own diagnostic with a note and a fix-it, in both views"_test = [] {
        d::Item item;
        item.path = "src/b.cpp";
        item.lines = { "int x = y;" };
        item.diagnostic = { { { 0, 8 }, { 0, 9 } }, msa::Severity::error, "use of undeclared identifier 'y'", "undeclared_var_use", "Semantic Issue", {} };
        item.fixits = { { { { 0, 8 }, { 0, 9 } }, "z" } };
        d::Item note;
        note.path = "src/b.cpp";
        note.lines = { "int z;" };
        note.diagnostic = { { { 0, 4 }, { 0, 5 } }, msa::Severity::information, "'z' declared here", "", "", {} };
        item.notes = { note };
        const std::string text { d::human(item, false) };
        expect(text.contains("error[undeclared_var_use]: use of undeclared identifier 'y'\n") && text.contains("= help: replace 1:9-10 with `z`\n"));
        expect(text.contains("note: 'z' declared here\n") && text.contains(" --> src/b.cpp:1:5\n")) << text;
        const auto j = nlohmann::json::parse(d::agent(item));
        expect(j["fixits"][0]["text"] == "z" && j["notes"][0]["message"] == "'z' declared here" && !j.contains("level"));
    };

    return report();
}
