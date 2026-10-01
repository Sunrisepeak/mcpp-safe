// MSA over Clang 23.1 (llvm.clang-dev): the one package of libmc++ that knows Clang (plan P1).
//
//   auto workspace = mcxx::backend::clang::make_workspace(options);
//   workspace->set_commands(commands);
//   auto unit = workspace->parse(path, text, version);
//
// Nothing Clang-typed crosses this interface; the implementation units include Clang's headers in
// their global module fragments only.
export module mcxx.backend.clang;

import std;
import mcxx.msa;

export namespace mcxx::backend::clang {

// The Clang release the backend is built on, e.g. "23.1.0".
std::string_view version();

// The backend as msa describes backends: "mcxx.backend.clang", its version, and the libc++ release a
// semantic kit must be (the one this Clang release reads).
msa::BackendInfo info();

std::unique_ptr<msa::Workspace> make_workspace(msa::Workspace::Options options);

// The arguments that describe a unit's program (MC5 §6): its command without what the backend
// decides itself -- outputs, the interfaces read, the input language, the resource directory, the
// phase, dependency and color flags, the source. What a parse starts from and what the interface
// cache is keyed on, so the same whichever form the command arrived in (MC5-6-2).
std::vector<std::string> derived_arguments(const msa::Command& command);
// The arguments a file's enabled language features need (MC4 0.4.0, the language providers), for a
// command's arguments (its target from --target, else Clang's default).
std::vector<std::string> language_arguments(const std::string& path, const std::vector<std::string>& args);

// The command a file the build does not list is read with: the nearest listed file's (longest
// common directory, then the same extension), with the file swapped in.
std::optional<msa::Command> inferred_command(std::span<const msa::Command> commands, std::string_view file);

// A unit's own declarations counted straight off Clang's AST, by Clang's declaration kind names
// ("Var", "ParmVar", "CXXMethod", ...): the reference MSA's T1 facts are checked against (A0.4.3,
// tools/checks/facts.py). Empty for a unit this backend did not parse.
std::map<std::string, std::int64_t> census(const msa::Unit& unit);

// One token of Clang's raw lexer: its kind as Clang names it ("raw_identifier", "l_paren", ...),
// its bytes [begin, end) in the text, and its 1-based line and column.
struct RawToken {
    std::string kind;
    std::uint32_t begin { 0 };
    std::uint32_t end { 0 };
    std::uint32_t line { 0 };
    std::uint32_t column { 0 };
};

// A text's tokens as Clang's raw lexer finds them under C++23 (what `-dump-raw-tokens` prints),
// comments included, whitespace (the `unknown` tokens there that are only blanks) not: the reference MC++'s own lexer is checked against (A1.6.1,
// tools/checks/lexdiff.py).
std::vector<RawToken> raw_tokens(std::string_view text);

} // namespace mcxx::backend::clang
