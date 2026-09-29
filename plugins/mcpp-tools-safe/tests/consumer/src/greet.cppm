export module greet;
import std;

export std::string greet(std::string_view who) { return std::format("hello, {}", who); }
