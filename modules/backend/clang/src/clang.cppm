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

// The command a file the build does not list is read with: the nearest listed file's (longest
// common directory, then the same extension), with the file swapped in.
std::optional<msa::Command> inferred_command(std::span<const msa::Command> commands, std::string_view file);

} // namespace mcxx::backend::clang
