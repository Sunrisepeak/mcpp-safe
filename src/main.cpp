// mcxx: MC++'s compiler driver (modules/driver), with MC++'s standard plugins linked in
// (plugins/std, plugins/libs). `mcxx compose` builds the same program with a package's own static
// plugins added (MC4 §3).
import mcxx.driver;

int main(int argc, char** argv) { return mcxx::driver::run(argc, argv); }
