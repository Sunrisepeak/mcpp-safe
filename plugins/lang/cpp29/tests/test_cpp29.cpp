// mcxx.plugins.lang.cpp29: the papers of C++29 as MC1 features (category standard), and the arguments
// a file where one is enabled is compiled with.
import std;
import mcxx.testing;
import mcxx.plugin;
import mcxx.plugins.lang.cpp29;

namespace plugin = mcxx::plugin;
namespace cpp29 = mcxx::plugins::lang::cpp29;

namespace {

std::vector<std::string> run(std::vector<std::string> command, std::string_view enabled) {
    const plugin::Target linux { .triple = "x86_64-unknown-linux-gnu", .os = "linux" };
    cpp29::Language language;
    return language.arguments({ "/p/src/a.cpp", linux, [&](std::string_view id) { return id == enabled; }, command });
}

} // namespace

int main() {
    using namespace mcxx::testing;

    "every core-language paper of C++29 in Clang 23.1's list is a feature, each once"_test = [] {
        expect(cpp29::PAPERS.size() == 17u) << std::format("{} papers", cpp29::PAPERS.size());
        std::set<std::string_view> ids;
        for (const auto& p : cpp29::PAPERS) {
            expect(p.id.starts_with("c++29:") && p.paper.starts_with('P') && !p.title.empty()) << p.id;
            expect(ids.insert(p.id).second) << std::format("{} twice", p.id);
        }
        const auto count = [](cpp29::Status s) { return std::ranges::count(cpp29::PAPERS, s, &cpp29::Paper::status); };
        expect(count(cpp29::Status::baseline) == 2 && count(cpp29::Status::partial) == 0 && count(cpp29::Status::todo) == 15)
            << std::format("{} baseline, {} partial, {} todo", count(cpp29::Status::baseline), count(cpp29::Status::partial),
                           count(cpp29::Status::todo));
        const auto pack = std::ranges::find(cpp29::PAPERS, std::string_view { "c++29:pack-indexing-template-names" }, &cpp29::Paper::id);
        expect(pack != cpp29::PAPERS.end() && pack->paper == "P3670R4" && pack->status == cpp29::Status::todo);
    };

    "they are registered: category standard, off by default, and the catalog has no problem with them"_test = [] {
        const auto catalog = plugin::catalog();
        expect(catalog->languages.size() == 1 && catalog->languages[0]->name() == "mcxx.plugins.lang.cpp29");
        for (const auto& p : cpp29::PAPERS) {
            const plugin::Feature* f { plugin::find_feature(p.id) };
            expect(f != nullptr && f->category == plugin::Category::standard && f->default_level == plugin::Level::deny && f->standard == p.paper) << p.id;
        }
        expect(catalog->problems.empty()) << (catalog->problems.empty() ? std::string {} : catalog->problems.front());
    };

    "a file with a C++29 feature enabled is compiled as C++29; one without, as its command says"_test = [] {
        expect(run({ "clang++", "-std=c++23", "-c", "a.cpp" }, "c++29:pack-indexing-template-names") == std::vector<std::string> { "-std=c++2d" });
        expect(run({ "clang++", "-std=gnu++23", "-c", "a.cpp" }, "c++29:pack-indexing-template-names") == std::vector<std::string> { "-std=gnu++2d" });
        expect(run({ "clang++", "-c", "a.cpp" }, "c++29:pack-indexing-template-names") == std::vector<std::string> { "-std=c++2d" });
        expect(run({ "clang++", "-std=c++23", "-c", "a.cpp" }, "c++26:pack-indexing").empty()) << "none of its features enabled: C++26's is another plugin's";
    };

    "a command that already asks for C++29 or later is left as it is"_test = [] {
        expect(run({ "clang++", "-std=c++2d", "-c", "a.cpp" }, "c++29:pack-indexing-template-names").empty());
        expect(run({ "clang++", "-std=c++29", "-c", "a.cpp" }, "c++29:pack-indexing-template-names").empty());
        expect(run({ "clang++", "-std=c++2c", "-c", "a.cpp" }, "c++29:pack-indexing-template-names") == std::vector<std::string> { "-std=c++2d" })
            << "C++26 is not enough";
        expect(cpp29::asked({ { "-std=c++17", "-std=gnu++2b" } }).year == 23 && cpp29::asked({ { "-std=gnu++2b" } }).gnu);
    };

    return report();
}
