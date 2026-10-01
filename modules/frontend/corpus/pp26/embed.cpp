// langdiff: c++2c
// #embed (P1967R14) as Clang 23.1 has it, and P3540R3's offset as Clang's `clang::offset`: the directive's
// replacement, token for token (tools/checks/langdiff.py).
int plain[] = {
#embed "data/hello.txt"
};
int again[] = {
#embed "data/hello.txt" limit(5)
};
unsigned char binary[] = {
#embed "data/bytes.bin"
};
int limited[] = {
#embed "data/hello.txt" limit(2)
};
int limit_expression[] = {
#embed "data/hello.txt" limit(1 + 3)
};
int limit_beyond[] = {
#embed "data/hello.txt" limit(100)
};
int limit_zero[] = {
#embed "data/hello.txt" limit(0)
};
int around[] = {
#embed "data/hello.txt" prefix(0xEF, 0xBB, 0xBF, ) suffix(,)
  0
};
int around_empty[] = {
#embed "data/empty.txt" prefix(1, 2, ) suffix(,)
  0
};
int fallback[] = {
#embed "data/empty.txt" if_empty(42203)
};
int fallback_limit[] = {
#embed "data/hello.txt" if_empty(42203) limit(0)
};
int fallback_unused[] = {
#embed "data/hello.txt" if_empty(42203) limit(1)
};
int underscored[] = {
#embed "data/hello.txt" __limit__(2) __prefix__(7,)
};
#define LIM 3
#define PFX(x) suffix(x)
#define THE_RESOURCE "data/hello.txt"
#define THE_ADDITION "teehee"
int expanded[] = {
#embed "data/hello.txt" limit(LIM) PFX(0)
};
int third_form[] = {
#embed THE_RESOURCE limit(LIM)
};
int sum[] = {
#embed "data/hello.txt" limit(1) prefix(1 +) suffix()
};
int offset_vendor[] = {
#embed "data/hello.txt" clang::offset(3)
};
int offset_limit[] = {
#embed "data/hello.txt" clang::offset(1) limit(2)
};
int offset_empty[] = {
#embed "data/hello.txt" clang::offset(5) if_empty(9)
};
int offset_past[] = {
#embed "data/hello.txt" clang::offset(50) if_empty(9)
};
#if __has_embed("data/hello.txt") == __STDC_EMBED_FOUND__
found
#endif
#if __has_embed("data/empty.txt") == __STDC_EMBED_EMPTY__
empty
#endif
#if __has_embed("data/hello.txt" limit(0)) == __STDC_EMBED_EMPTY__
empty_by_limit
#endif
#if __has_embed("data/nonexistent.txt") == __STDC_EMBED_NOT_FOUND__
missing
#endif
#if __has_embed("data/hello.txt" acme::open_mode("x"))
unsupported_found
#else
unsupported
#endif
#if __has_embed("data/hello.txt" clang::offset(2)) && !__has_embed("data/hello.txt" clang::offset(9))
offset_in_has_embed
#endif
#if defined(__has_embed) && defined __has_embed
has_embed_defined
#endif
