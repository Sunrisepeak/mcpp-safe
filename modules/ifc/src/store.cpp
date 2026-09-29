// Finding a BMI's interface: beside it, or in the store by the BMI's content (specs/mc2-ifc.md §7).
module;

#include <ifc/abstract-sgraph.hxx>
#include <ifc/file.hxx>

module mcxx.ifc;

import std;
import mcxx.msa;
import :format;

namespace mcxx::ifc {

namespace {

std::string hex(const sdk::SHA256Hash& hash) {
    std::string out;
    const auto* bytes = reinterpret_cast<const unsigned char*>(hash.value.data());
    for (std::size_t i { 0 }; i < sizeof hash.value; ++i) out += std::format("{:02x}", bytes[i]);
    return out;
}

std::optional<std::string> sha256_of_file(const std::string& path) {
    std::ifstream in { path, std::ios::binary };
    if (!in) return std::nullopt;
    std::vector<char> data { std::istreambuf_iterator<char> { in }, {} };
    const auto* first = reinterpret_cast<const std::byte*>(data.data());
    return hex(sdk::hash_bytes(first, first + data.size()));
}

std::string sha256_of_text(std::string_view text) {
    const auto* first = reinterpret_cast<const std::byte*>(text.data());
    return hex(sdk::hash_bytes(first, first + text.size()));
}

struct Stamp {
    std::uintmax_t size { 0 };
    std::int64_t time { 0 };
    bool operator==(const Stamp&) const = default;
};

std::optional<Stamp> stamp_of(const std::string& path) {
    std::error_code ec;
    const auto size = std::filesystem::file_size(path, ec);
    if (ec) return std::nullopt;
    const auto time = std::filesystem::last_write_time(path, ec);
    if (ec) return std::nullopt;
    return Stamp { size, static_cast<std::int64_t>(time.time_since_epoch().count()) };
}

bool copy_atomically(const std::string& from, const std::string& to) {
    std::error_code ec;
    std::filesystem::create_directories(std::filesystem::path { to }.parent_path(), ec);
    const std::string tmp { to + ".tmp" + std::to_string(std::hash<std::thread::id> {}(std::this_thread::get_id())) };
    std::filesystem::copy_file(from, tmp, std::filesystem::copy_options::overwrite_existing, ec);
    if (ec) return false;
    std::filesystem::rename(tmp, to, ec);
    if (ec) std::filesystem::remove(tmp, ec);
    return !ec;
}

// A BMI's SHA-256, remembered by its path, size and time in the store (by-path/), so a BMI is read
// whole once, not at every importer's compile.
std::optional<std::string> bmi_digest(const std::string& bmi, const Stamp& stamp) {
    const std::string memo { store_directory() + "/by-path/" + sha256_of_text(bmi) };
    if (std::ifstream in { memo }; in) {
        Stamp kept;
        std::string digest;
        if (in >> kept.size >> kept.time >> digest && kept == stamp && digest.size() == 64) return digest;
    }
    auto digest = sha256_of_file(bmi);
    if (!digest) return std::nullopt;
    std::error_code ec;
    std::filesystem::create_directories(std::filesystem::path { memo }.parent_path(), ec);
    const std::string tmp { memo + ".tmp" + std::to_string(std::hash<std::thread::id> {}(std::this_thread::get_id())) };
    if (std::ofstream out { tmp }; out) out << stamp.size << ' ' << stamp.time << ' ' << *digest << '\n';
    std::filesystem::rename(tmp, memo, ec);
    if (ec) std::filesystem::remove(tmp, ec);
    return digest;
}

std::mutex& written_lock() {
    static std::mutex m;
    return m;
}
std::vector<std::pair<std::string, std::string>>& written() {
    static std::vector<std::pair<std::string, std::string>> w;
    return w;
}

} // namespace

std::mutex& store_lock() {
    static std::mutex m;
    return m;
}
std::string& chosen_store() {
    static std::string s;
    return s;
}

void use_store(std::string directory) {
    std::lock_guard lock { store_lock() };
    chosen_store() = std::move(directory);
}

std::string store_directory() {
    {
        std::lock_guard lock { store_lock() };
        if (!chosen_store().empty()) return chosen_store();
    }
    if (const char* s = std::getenv("MCXX_IFC_STORE"); s != nullptr && *s != '\0') return s;
    if (const char* x = std::getenv("XDG_CACHE_HOME"); x != nullptr && *x != '\0') return std::string { x } + "/mcxx/ifc";
    if (const char* h = std::getenv("HOME"); h != nullptr && *h != '\0') return std::string { h } + "/.cache/mcxx/ifc";
    return (std::filesystem::temp_directory_path() / "mcxx-ifc").string();
}

void note_written(std::string bmi, std::string ifc) {
    std::lock_guard lock { written_lock() };
    written().emplace_back(std::move(bmi), std::move(ifc));
}

void publish() {
    std::vector<std::pair<std::string, std::string>> done;
    {
        std::lock_guard lock { written_lock() };
        done.swap(written());
    }
    for (const auto& [bmi, ifc] : done) {
        const auto stamp = stamp_of(bmi);
        if (!stamp) continue;   // the compile did not leave its BMI (an error after the interface was written)
        if (const auto digest = bmi_digest(bmi, *stamp)) {
            const std::string kept { store_directory() + "/" + *digest + ".ifc" };
            if (!std::filesystem::exists(kept)) (void)copy_atomically(ifc, kept);
        }
    }
}

std::shared_ptr<const Interface> interface_for(const std::string& bmi, std::string* error) {
    struct Entry {
        Stamp stamp;
        std::shared_ptr<const Interface> unit;
        std::string error;
    };
    static std::mutex lock;
    static std::unordered_map<std::string, Entry> cache;
    const auto stamp = stamp_of(bmi);
    if (!stamp) {
        if (error) *error = std::format("no BMI at {}", bmi);
        return nullptr;
    }
    {
        std::lock_guard guard { lock };
        if (const auto it = cache.find(bmi); it != cache.end() && it->second.stamp == *stamp) {
            if (error) *error = it->second.error;
            return it->second.unit;
        }
    }
    Entry entry { *stamp, nullptr, {} };
    std::string path { path_for(bmi) };
    if (!std::filesystem::exists(path)) {
        path.clear();
        if (const auto digest = bmi_digest(bmi, *stamp)) {
            const std::string kept { store_directory() + "/" + *digest + ".ifc" };
            if (std::filesystem::exists(kept)) path = kept;
        }
    }
    if (path.empty()) entry.error = std::format("no {} beside {}, and none kept for it in {}", std::filesystem::path { path_for(bmi) }.filename().string(), bmi, store_directory());
    else if (auto unit = load(path)) entry.unit = std::make_shared<const Interface>(std::move(*unit));
    else entry.error = unit.error();
    if (error) *error = entry.error;
    std::lock_guard guard { lock };
    return (cache[bmi] = std::move(entry)).unit;
}

} // namespace mcxx::ifc
