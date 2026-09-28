int plain();
[[nodiscard]] int checked();
[[maybe_unused]] static int spare;
// [[mcpp::cfg(unix)]] in a comment
const char* text = "[[mcpp::cfg(unix)]]";
[[deprecated("use plain")]] int old();
