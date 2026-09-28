// The backend facade: what a consumer of MSA asks for, without knowing which front end answers.
//
//   auto workspace = mcxx::backend::make_workspace(options);
//   const auto about = mcxx::backend::info();   // name, version, the kit's standard library
//
// Today the answer is mcxx.clang. Nothing here, and nothing a consumer sees, names Clang.
export module mcxx.backend;

import std;
import mcxx.msa;
import mcxx.clang;

export namespace mcxx::backend {

msa::BackendInfo info() { return mcxx::clang::info(); }

std::unique_ptr<msa::Workspace> make_workspace(msa::Workspace::Options options) { return mcxx::clang::make_workspace(std::move(options)); }

} // namespace mcxx::backend
