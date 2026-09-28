// example.naming out of process: linking the plugin package registers its rule; serve() speaks MC4
// protocol 1 on standard input and output.
import mcxx.plugin.remote;

int main() { return mcxx::plugin::remote::serve(); }
