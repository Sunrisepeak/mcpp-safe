export module app.table;

export namespace app {
[[mcpp::allow("c-array", "a lookup table the hardware defines")]] extern const int crc_table[256];   // expect-waived: c-array
int lookup(int i);
}
