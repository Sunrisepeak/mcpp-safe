// mcxx.backend.clang partition :gate: MC++ feature gates inside every Clang compilation of this program, and
// [[mcpp::allow]]. The Clang plugin and the attributes register themselves from gate.cpp; what the
// workspace asks is declared here (MC5 §8).
module;

#include <string>
#include <string_view>
#include <vector>

module mcxx.backend.clang:gate;

import mcxx.msa;

namespace mcxx::clang_backend {

// msa::Workspace::quick: MC++'s own front end reads the buffer's text (with the command's -D, -U and
// target) and gives the findings of the features it decides from the facts it fills -- and which
// features those are -- before, and without, a parse (A1.8.3).
msa::Workspace::Quick quick_gates(const std::string& path, std::string_view text, const msa::Command* command);

// The arguments the file's enabled language features need (MC4 0.4.0: the language providers), for
// a command's arguments: its target from --target, else Clang's default. Added before the arguments
// make a module's key, so an interface built with them is another interface.
std::vector<std::string> language_arguments_of(const std::string& path, const std::vector<std::string>& args);

} // namespace mcxx::clang_backend
