export module legacy;
export import :detail;

export namespace legacy {
int* address(int& value);
using Table = int[16];
int log(const char* format, ...);
}

int scratch[4];   // not exported: it does not cross
