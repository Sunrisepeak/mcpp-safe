// The plugin side of MC4 protocol 1 (specs/mc4-plugins.md §6). A plugin program is its plugin
// packages linked with this one and a main that calls serve():
//
//   import mcxx.plugin.remote;
//   int main() { return mcxx::plugin::remote::serve(); }
//
// It serves the providers registered in the program -- the SDK's Registration, as in a static
// composition, so one plugin package works both ways -- and answers requests until `shutdown` or
// the end of its input. Nothing but messages goes to the output (MC4-6.1-2).
export module mcxx.plugin.remote;

import std;
import nlohmann.json;
import mcxx.msa;
import mcxx.plugin;
import mcxx.plugin.wire;

export namespace mcxx::plugin::remote {

// Serves until shutdown or the end of `in`; 0 on a clean end, 1 when the host speaks no protocol
// this side does.
int serve(std::istream& in, std::ostream& out);
int serve();   // standard input and output

} // namespace mcxx::plugin::remote
