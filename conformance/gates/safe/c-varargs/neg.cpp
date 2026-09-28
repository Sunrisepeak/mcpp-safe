template <class... Ts> int count(Ts... xs) { return sizeof...(xs); }
int first(int a, int b = 0);
struct Out { template <class... A> void print(A&&... a); };
void none();
void guard() { try { none(); } catch (...) {} }
int two(int, int);
