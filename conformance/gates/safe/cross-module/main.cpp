import shapes;
int use() {
    auto v = make();          // a pointer, though no `*` is written here
    Buf b = {};               // expect: c-array
    Node n { nullptr };
    return *(v + 1) + b[0];   // expect: raw-pointer-arithmetic
}
