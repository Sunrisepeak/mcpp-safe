struct Point { int x, y; };
class Shape { int kind; };
enum class Color { red, green };
union Declared;                                            // a declaration, not a definition
struct Variant { int which; Point p; };
