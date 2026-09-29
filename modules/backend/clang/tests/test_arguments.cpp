// A0.5.2 (MC5-6-2): one unit, the forms its command arrives in -- a build database's object compile,
// a build plan's interface precompilation, a command line, an attached spelling, a command read back
// from a cache, a neighbor's command inferred for a file the build does not list -- derive
// byte-for-byte the same arguments; what changes the program does not.
import std;
import mcxx.testing;
import mcxx.msa;
import mcxx.backend.clang;

namespace msa = mcxx::msa;
using mcxx::backend::clang::derived_arguments;
using mcxx::backend::clang::inferred_command;

namespace {

const std::vector<std::string> PROGRAM { "clang++", "-std=c++23", "-I", "include", "-DMODE=1", "--target=x86_64-unknown-linux-gnu", "-fno-exceptions" };

msa::Command command(std::vector<std::string> tail, std::string file = "/w/src/m.cppm") {
    std::vector<std::string> args { PROGRAM };
    args.insert(args.end(), tail.begin(), tail.end());
    return { "/w", std::move(file), std::move(args) };
}

std::string bytes(const std::vector<std::string>& args) {
    std::string out;
    for (const auto& a : args) out += a + '\0';
    return out;
}

} // namespace

int main() {
    using namespace mcxx::testing;
    const auto expected = derived_arguments(command({ "-c", "src/m.cppm", "-o", "obj/m.o" }));

    "the program's own arguments, in their order, and nothing the backend decides"_test = [&] {
        expect(expected == PROGRAM) << std::format("{}", expected);
    };

    "a database's compile, a plan's precompilation, a command line: the same bytes"_test = [&] {
        const std::vector<msa::Command> forms {
            command({ "-c", "src/m.cppm", "-o", "obj/m.o", "-MD", "-MF", "obj/m.o.d", "-fdiagnostics-color=always" }),
            command({ "-x", "c++-module", "--precompile", "/w/src/m.cppm", "-o", "pcm.cache/m.pcm", "-fmodule-file=std=/c/std.pcm" }),
            command({ "-fmodule-output=gcm/m.pcm", "-fmodules-reduced-bmi", "-c", "/w/src/m.cppm", "-o", "/w/target/obj/m.o" }),
            command({ "-xc++-module", "-fsyntax-only", "src/m.cppm", "-oobj/m.o", "-fprebuilt-module-path=/w/pcm" }),
            command({ "-c", "./src/../src/m.cppm", "-o", "obj/m.o", "-resource-dir", "/opt/clang/lib/clang/23", "-fmodule-file", "a=/c/a.pcm" }),
            command({ "-c", "src/m.cppm", "-o", "obj/m.o" }, "src/m.cppm"),   // the database's file field, relative
        };
        for (const auto& c : forms) expect(bytes(derived_arguments(c)) == bytes(expected)) << std::format("{}", derived_arguments(c));
    };

    "derived arguments read back from a cache derive to themselves"_test = [&] {
        msa::Command cached { "/w", "/w/src/m.cppm", expected };
        cached.arguments.push_back("/w/src/m.cppm");
        expect(bytes(derived_arguments(cached)) == bytes(expected));
    };

    "a file the build does not list takes its nearest neighbor's program"_test = [&] {
        const std::vector<msa::Command> commands { command({ "-c", "src/m.cppm", "-o", "obj/m.o" }),
                                                   { "/w", "/w/tools/t.cpp", { "clang++", "-std=c++20", "-c", "tools/t.cpp" } } };
        const auto header = inferred_command(commands, "/w/src/detail/m.h");
        expect(fatal(header.has_value()));
        expect(header->file == "/w/src/detail/m.h" && bytes(derived_arguments(*header)) == bytes(expected)) << std::format("{}", header->arguments);
    };

    "what changes the program changes the bytes: a value, an order, a flag"_test = [&] {
        auto value = command({ "-c", "src/m.cppm" });
        value.arguments[4] = "-DMODE=2";
        auto order = command({ "-c", "src/m.cppm" });
        std::swap(order.arguments[4], order.arguments[6]);
        auto flag = command({ "-c", "src/m.cppm", "-fno-rtti" });
        for (const auto& c : { value, order, flag }) expect(bytes(derived_arguments(c)) != bytes(expected)) << std::format("{}", c.arguments);
    };

    "a GCC command's module switches are GCC's: not Clang's header modules (MC5 0.1.1)"_test = [&] {
        const msa::Command gcc { "/w", "/w/src/m.cppm", { "/opt/gcc/bin/g++", "-std=c++23", "-fmodules", "-fdeps-format=p1689r5", "-DMODE=1", "-c", "src/m.cppm" } };
        expect(derived_arguments(gcc) == std::vector<std::string> { "/opt/gcc/bin/g++", "-std=c++23", "-DMODE=1" }) << std::format("{}", derived_arguments(gcc));
        const msa::Command clang { "/w", "/w/src/m.cppm", { "clang++", "-std=c++23", "-fmodules", "-c", "src/m.cppm" } };
        expect(std::ranges::contains(derived_arguments(clang), std::string { "-fmodules" }));
    };

    return report();
}
