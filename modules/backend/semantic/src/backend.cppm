// The backend facade: what a consumer of MSA asks for, without knowing which front end answers.
//
//   auto workspace = mcxx::backend::make_workspace(options);
//   const auto about = mcxx::backend::info();   // name, version, the kit's standard library
//
// Today the answer is mcxx.backend.clang. Nothing here, and nothing a consumer sees, names Clang.
export module mcxx.backend;

import std;
import mcxx.msa;
import mcxx.backend.clang;

export namespace mcxx::backend {

msa::BackendInfo info() { return clang::info(); }

std::unique_ptr<msa::Workspace> make_workspace(msa::Workspace::Options options) { return clang::make_workspace(std::move(options)); }

// MC5 §6: what a unit's command says about its program, whichever form it came in.
std::vector<std::string> derived_arguments(const msa::Command& command) { return clang::derived_arguments(command); }
// MC4 0.4.0: the arguments the file's enabled language features need, added to its compile command.
std::vector<std::string> language_arguments(const std::string& path, const std::vector<std::string>& args) {
    return clang::language_arguments(path, args);
}

// A unit's declarations counted straight off the backend's AST, by its own kind names: a check's
// reference, not an API to build on (A0.4.3).
std::map<std::string, std::int64_t> census(const msa::Unit& unit) { return clang::census(unit); }

// A text's tokens as the backend's own lexer finds them, comments included: the reference MC++'s
// frontend lexer is checked against (A1.6.1). Also not an API to build on.
using RawToken = clang::RawToken;
std::vector<RawToken> raw_tokens(std::string_view text) { return clang::raw_tokens(text); }

} // namespace mcxx::backend
