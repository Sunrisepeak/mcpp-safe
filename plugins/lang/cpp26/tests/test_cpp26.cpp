// mcxx.plugins.lang.cpp26: the papers of C++26 as MC1 features (category standard), and the arguments
// a file where one is enabled is compiled with.
import std;
import mcxx.testing;
import mcxx.plugin;
import mcxx.plugins.lang.cpp26;

namespace plugin = mcxx::plugin;
namespace cpp26 = mcxx::plugins::lang::cpp26;

namespace {

std::vector<std::string> run(std::vector<std::string> command, std::string_view enabled) {
    const plugin::Target linux { .triple = "x86_64-unknown-linux-gnu", .os = "linux" };
    cpp26::Language language;
    return language.arguments({ "/p/src/a.cpp", linux, [&](std::string_view id) { return id == enabled; }, command });
}

} // namespace

int main() {
    using namespace mcxx::testing;

    "every core-language paper of C++26 in Clang 23.1's list is a feature, each once"_test = [] {
        expect(cpp26::PAPERS.size() == 54u) << std::format("{} papers", cpp26::PAPERS.size());
        std::set<std::string_view> ids;
        for (const auto& p : cpp26::PAPERS) {
            expect(p.id.starts_with("c++26:") && p.paper.starts_with('P') && !p.title.empty()) << p.id;
            expect(ids.insert(p.id).second) << std::format("{} twice", p.id);
        }
        const auto count = [](cpp26::Status s) { return std::ranges::count(cpp26::PAPERS, s, &cpp26::Paper::status); };
        expect(count(cpp26::Status::baseline) == 28 && count(cpp26::Status::partial) == 1 && count(cpp26::Status::todo) == 25)
            << std::format("{} baseline, {} partial, {} todo", count(cpp26::Status::baseline), count(cpp26::Status::partial),
                           count(cpp26::Status::todo));
        const auto reflection = std::ranges::find(cpp26::PAPERS, std::string_view { "c++26:reflection" }, &cpp26::Paper::id);
        expect(reflection != cpp26::PAPERS.end() && reflection->paper == "P2996R13" && reflection->status == cpp26::Status::todo);
    };

    "they are registered: category standard, off by default, and the catalog has no problem with them"_test = [] {
        const auto catalog = plugin::catalog();
        expect(catalog->languages.size() == 1 && catalog->languages[0]->name() == "mcxx.plugins.lang.cpp26");
        for (const auto& p : cpp26::PAPERS) {
            const plugin::Feature* f { plugin::find_feature(p.id) };
            expect(f != nullptr && f->category == plugin::Category::standard && f->default_level == plugin::Level::deny && f->standard == p.paper) << p.id;
        }
        expect(catalog->problems.empty()) << (catalog->problems.empty() ? std::string {} : catalog->problems.front());
    };

    "a file with a C++26 feature enabled is compiled as C++26; one without, as its command says"_test = [] {
        expect(run({ "clang++", "-std=c++23", "-c", "a.cpp" }, "c++26:pack-indexing") == std::vector<std::string> { "-std=c++2c" });
        expect(run({ "clang++", "-std=gnu++23", "-c", "a.cpp" }, "c++26:pack-indexing") == std::vector<std::string> { "-std=gnu++2c" });
        expect(run({ "clang++", "-c", "a.cpp" }, "c++26:pack-indexing") == std::vector<std::string> { "-std=c++2c" });
        expect(run({ "clang++", "-std=c++23", "-c", "a.cpp" }, "c++29:nothing").empty()) << "none of its features enabled";
    };

    "a command that already asks for C++26 or later is left as it is"_test = [] {
        expect(run({ "clang++", "-std=c++2c", "-c", "a.cpp" }, "c++26:pack-indexing").empty());
        expect(run({ "clang++", "-std=c++26", "-c", "a.cpp" }, "c++26:pack-indexing").empty());
        expect(run({ "clang++", "-std=c++2d", "-c", "a.cpp" }, "c++26:pack-indexing").empty()) << "never a downgrade";
        expect(cpp26::asked({ { "-std=c++17", "-std=gnu++2b" } }).year == 23 && cpp26::asked({ { "-std=gnu++2b" } }).gnu);
    };

    return report();
}
