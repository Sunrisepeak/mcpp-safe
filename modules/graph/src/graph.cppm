// Module facts without a compiler: what a C++ source provides and imports, read lexically, and the
// module graph of a program built from those scans.
//
// The scan does not preprocess (a module declaration and its imports must not come from a macro,
// [cpp.pre]); it skips comments, string and character literals (raw ones included) and directive
// lines, and reads only what is at namespace scope outside every brace.
export module mcxx.graph;

import std;

export namespace mcxx::graph {

struct Scan {
    std::string module;                 // "m", "m:p", or "" when the file is no module unit
    bool exported { false };            // `export module` (an interface or interface partition)
    bool global_fragment { false };     // begins with `module;`
    std::vector<std::string> imports;   // fully qualified: `import :p;` in module m is "m:p"
    std::vector<std::string> exported_imports;
    // Where each import names its module: byte offsets [begin, end) into the text, parallel to `imports`.
    std::vector<std::pair<std::size_t, std::size_t>> import_spans;
    std::pair<std::size_t, std::size_t> module_span { 0, 0 };   // the declaration's module name

    bool is_module_unit() const { return !module.empty(); }
    bool is_partition() const { return module.find(':') != std::string::npos; }
    // A unit whose compilation produces an interface others import: every interface, and every
    // partition (an implementation partition is imported by its own module's units).
    bool provides_interface() const { return exported || is_partition(); }
    std::string primary() const { return module.substr(0, module.find(':')); }
};

Scan scan(std::string_view text);

class Graph {
public:
    // Adds (or replaces) what `file` is, from its scan.
    void set(const std::string& file, Scan scan);
    void remove(const std::string& file);
    void clear();

    const Scan* scan_of(std::string_view file) const;
    // The file that provides the interface of `module` ("m" or "m:p"), or empty.
    std::string provider(std::string_view module) const;
    // Modules the named module's interface imports, directly.
    std::vector<std::string> requires_of(std::string_view module) const;
    // Every module `roots` need, dependencies first, each once; modules nobody provides are left
    // out and reported in `missing`.
    std::vector<std::string> closure(std::span<const std::string> roots, std::vector<std::string>* missing = nullptr) const;
    std::vector<std::string> modules() const;
    // Modules whose interfaces import one another in a cycle.
    std::vector<std::vector<std::string>> cycles() const;

private:
    std::map<std::string, Scan, std::less<>> files_;
    std::map<std::string, std::string, std::less<>> providers_;   // module -> file
    void rebuild_providers_();
};

} // namespace mcxx::graph
