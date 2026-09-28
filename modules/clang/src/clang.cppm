// MSA over Clang 23.1 (llvm.clang-dev): the one package of libmc++ that knows Clang (plan P1).
//
//   auto workspace = mcxx::clang::make_workspace(options);
//   workspace->set_commands(commands);
//   auto unit = workspace->parse(path, text, version);
//
// Nothing Clang-typed crosses this interface; the implementation units include Clang's headers in
// their global module fragments only.
export module mcxx.clang;

import std;
import mcxx.msa;

export namespace mcxx::clang {

// The Clang release the backend is built on, e.g. "23.1.0".
std::string_view version();

// The backend as msa describes backends: "mcxx.clang", its version, and the libc++ release a
// semantic kit must be (the one this Clang release reads).
msa::BackendInfo info();

std::unique_ptr<msa::Workspace> make_workspace(msa::Workspace::Options options);

} // namespace mcxx::clang
