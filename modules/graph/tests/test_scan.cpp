// Lexical module facts: declarations, partitions, imports, and what must be ignored.
import std;
import mcxx.testing;
import mcxx.graph;

using mcxx::graph::scan;

int main() {
    using namespace mcxx::testing;

    "interface, partitions and imports"_test = [] {
        const auto s = scan("export module a.b;\nimport std;\nexport import :part;\nimport c.d;\n");
        expect(s.module == "a.b" && s.exported && s.provides_interface());
        expect(s.imports == std::vector<std::string> { "std", "a.b:part", "c.d" }) << "partition imports are qualified";
        expect(s.exported_imports == std::vector<std::string> { "a.b:part" });
        const auto p = scan("module;\n#include <cstdio>\nexport module m:detail.x;\n");
        expect(p.global_fragment && p.module == "m:detail.x" && p.is_partition() && p.primary() == "m");
        const auto impl = scan("module m;\nimport :helpers;\n");
        expect(impl.module == "m" && !impl.exported && !impl.provides_interface()) << "an implementation unit provides nothing";
        expect(impl.imports == std::vector<std::string> { "m:helpers" });
        const auto implPart = scan("module m:impl;\n");
        expect(implPart.provides_interface()) << "an implementation partition is importable by its module";
    };

    "what is not a declaration"_test = [] {
        const auto s = scan("// import x;\n/* export module y; */\nconst char* s = \"import z;\";\n"
                            "auto r = R\"(import w;)\";\nnamespace n { import v; }\n#define M import u;\nint import_count;\n");
        expect(s.module.empty() && s.imports.empty()) << "comments, strings, raw strings, braces and directives";
        const auto h = scan("export module m;\nimport <vector>;\nimport \"local.h\";\nmodule :private;\n");
        expect(h.module == "m" && h.imports.empty()) << "header units and the private fragment are not named imports";
        const auto bom = scan("\xEF\xBB\xBF" "export module bom;\n");
        expect(bom.module == "bom");
    };

    "where names are"_test = [] {
        const std::string text { "export module m.x;\nimport  a.b ;\nexport import :p;\n" };
        const auto s = scan(text);
        expect(text.substr(s.module_span.first, s.module_span.second - s.module_span.first) == "m.x");
        expect(s.import_spans.size() == 2);
        expect(text.substr(s.import_spans[0].first, s.import_spans[0].second - s.import_spans[0].first) == "a.b");
        expect(text.substr(s.import_spans[1].first, s.import_spans[1].second - s.import_spans[1].first) == ":p") << "a partition from its colon";
    };

    "graph"_test = [] {
        mcxx::graph::Graph g;
        g.set("/a.cppm", scan("export module a;\nimport b;\nimport c;\n"));
        g.set("/b.cppm", scan("export module b;\nimport c;\n"));
        g.set("/c.cppm", scan("export module c;\nimport std;\n"));
        g.set("/a.cpp", scan("module a;\nimport d;\n"));
        expect(g.provider("a") == "/a.cppm" && g.provider("d").empty());
        std::vector<std::string> missing;
        const std::vector<std::string> roots { "a" };
        const auto order = g.closure(roots, &missing);
        expect(order == std::vector<std::string> { "c", "b", "a" }) << "dependencies first";
        expect(missing == std::vector<std::string> { "std" });
        g.set("/c.cppm", scan("export module c;\nimport a;\n"));
        expect(g.cycles().size() == 1);
    };

    return report();
}
