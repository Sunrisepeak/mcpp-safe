namespace std { template <class T> struct vector { T* data; }; template <class T> struct list { T* head; }; }
namespace mine { template <class T> struct vector { T* data; }; }
mine::vector<int> own;
std::list<int> linked;
int plain[1];
struct vector_like { int size; };
vector_like v;
