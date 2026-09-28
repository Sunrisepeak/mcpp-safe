namespace mine { template <class T> struct ptr { T v; }; }
int value = 0;
int& alias = value;
void take(const int& r);
struct Box { int v; };
mine::ptr<int> boxed;
auto size = sizeof(int*);
decltype(nullptr) nothing = nullptr;
int numbers[3] = {};
