struct Meters { explicit Meters(double v) : v(v) {} double v; };
double half(int v) { return static_cast<double>(v) / 2; }
Meters length(double v) { return Meters(v); }
const int* view(int* p) { return p; }
long widen(int v) { return v; }
int round_down(double d) { return static_cast<int>(d); }
