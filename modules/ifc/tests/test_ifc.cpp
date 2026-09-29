// MC2 v1: an interface written as IFC and read back is the same interface, item by item; what is not
// an MC2 file is an error, never a crash.
import std;
import mcxx.testing;
import mcxx.msa;
import mcxx.ifc;

namespace {

using mcxx::msa::Kind;
using mcxx::msa::Range;
using mcxx::msa::fact::Declaration;

Declaration decl(Kind kind, std::string qualified, Range range, Range name, std::string container = {}) {
    Declaration d;
    d.kind = kind;
    d.qualified_name = std::move(qualified);
    d.entity = "c:@" + d.qualified_name;
    d.range = range;
    d.name = name;
    d.container = std::move(container);
    return d;
}

Range r(std::uint32_t l0, std::uint32_t c0, std::uint32_t l1, std::uint32_t c1) { return { { l0, c0 }, { l1, c1 } }; }

// export module app:part; ... what a unit's facts would say of it.
mcxx::ifc::Interface sample() {
    mcxx::ifc::Interface unit;
    unit.module = "app:part";
    unit.source = "/src/app/part.cppm";
    unit.target = "x86_64-unknown-linux-gnu";
    unit.dialect.profiles = { "safe" };
    unit.dialect.features = { { "goto", "deny" }, { "raw-pointers", "warn" }, { "macros", "allow" } };
    unit.dialect.namespaces = { { "app::detail", "reinterpret-cast", "allow" } };
    auto& d = unit.declarations;
    d.push_back(decl(Kind::namespace_, "app", r(2, 7, 40, 1), r(2, 17, 2, 20)));
    d.back().exported = true;
    d.push_back(decl(Kind::class_, "app::Box", r(3, 4, 20, 5), r(3, 10, 3, 13), "app"));
    d.back().exported = true;
    d.back().templates = { "std::vector" };
    d.push_back(decl(Kind::field, "app::Box::data", r(4, 8, 4, 22), r(4, 18, 4, 22), "app"));
    d.back().type = "int *";
    d.back().pointer = true;
    d.push_back(decl(Kind::constructor, "app::Box::Box", r(5, 8, 5, 30), r(5, 8, 5, 11), "app"));
    d.push_back(decl(Kind::parameter, "app::Box::Box::n", r(5, 12, 5, 17), r(5, 16, 5, 17), "app"));
    d.back().type = "int";
    d.push_back(decl(Kind::destructor, "app::Box::~Box", r(6, 8, 6, 17), r(6, 8, 6, 12), "app"));
    d.push_back(decl(Kind::method, "app::Box::size", r(7, 8, 7, 40), r(7, 13, 7, 17), "app"));
    d.push_back(decl(Kind::conversion, "app::Box::operator bool", r(8, 8, 8, 40), r(8, 17, 8, 21), "app"));
    d.push_back(decl(Kind::method, "app::Box::operator<<", r(9, 8, 9, 40), r(9, 13, 9, 23), "app"));
    d.push_back(decl(Kind::parameter, "app::Box::operator<<::x", r(9, 24, 9, 29), r(9, 28, 9, 29), "app"));
    d.push_back(decl(Kind::union_, "app::Box::(anonymous)", r(10, 8, 12, 9), r(10, 8, 10, 13), "app"));
    d.back().is_union = true;
    d.push_back(decl(Kind::field, "app::Box::(anonymous)::b", r(11, 12, 11, 20), r(11, 19, 11, 20), "app"));
    d.back().c_array = true;
    d.back().type = "char[4]";
    d.push_back(decl(Kind::enum_, "app::Color", r(21, 4, 21, 34), r(21, 15, 21, 20), "app"));
    d.push_back(decl(Kind::enumerator, "app::Color::red", r(21, 22, 21, 25), r(21, 22, 21, 25), "app"));
    d.push_back(decl(Kind::enumerator, "app::Color::green", r(21, 27, 21, 32), r(21, 27, 21, 32), "app"));
    d.push_back(decl(Kind::type_alias, "app::Bytes", r(22, 4, 22, 40), r(22, 10, 22, 15), "app"));
    d.back().type = "std::vector<unsigned char>";
    d.back().templates = { "std::vector" };
    d.push_back(decl(Kind::function, "app::printf_like", r(23, 4, 23, 50), r(23, 9, 23, 20), "app"));
    d.back().c_variadic = true;
    d.back().exported = true;
    d.push_back(decl(Kind::parameter, "app::printf_like::format", r(23, 21, 23, 40), r(23, 33, 23, 39), "app"));
    d.back().type = "const char *";
    d.back().pointer = true;
    d.push_back(decl(Kind::variable, "app::table", r(24, 4, 24, 30), r(24, 10, 24, 15), "app"));
    d.back().type = "int[3]";
    d.back().c_array = true;
    d.push_back(decl(Kind::concept_, "app::Small", r(25, 4, 25, 50), r(25, 26, 25, 31), "app"));
    d.push_back(decl(Kind::namespace_, "app::detail", r(26, 4, 30, 5), r(26, 14, 26, 20), "app"));
    d.push_back(decl(Kind::variable, "app::detail::quoted", r(27, 8, 27, 60), r(27, 20, 27, 26), "app::detail"));
    d.back().type = "const char[9] \"\\\n\t\x01 é";   // every kind of character an argument must carry
    d.push_back(decl(Kind::function, "operator\"\"_kb", r(31, 0, 31, 40), r(31, 5, 31, 18)));
    d.push_back(decl(Kind::parameter, "", r(31, 19, 31, 37), r(31, 37, 31, 37)));   // unnamed
    unit.reexports = { "app:detail", "base" };
    return unit;
}

} // namespace

int main() {
    using namespace mcxx::testing;

    "an interface round-trips, item by item"_test = [] {
        const auto unit = sample();
        const auto bytes = mcxx::ifc::write(unit);
        const auto back = mcxx::ifc::read(bytes);
        expect(back.has_value()) << (back ? "" : back.error());
        if (!back) return;
        const auto diff = mcxx::ifc::differences(unit.declarations, back->declarations);
        for (const auto& line : diff) std::println("  {}", line);
        expect(diff.empty());
        expect(back->declarations.size() == unit.declarations.size());
        expect(back->module == "app:part" && back->source == unit.source && back->target == unit.target && back->cplusplus == 202302);
        expect(back->dialect == unit.dialect) << "profiles, feature levels and namespace levels (A1.1.4)";
        expect(back->reexports == unit.reexports) << "what an importer also sees (MC2 1.1)";
        expect(!back->internal);
    };

    "the file is IFC 0.43 as the SDK lays it out"_test = [] {
        const auto bytes = mcxx::ifc::write(sample());
        expect(bytes.size() > 100);
        expect(static_cast<unsigned char>(bytes[0]) == 0x54 && static_cast<unsigned char>(bytes[1]) == 0x51
               && static_cast<unsigned char>(bytes[2]) == 0x45 && static_cast<unsigned char>(bytes[3]) == 0x1A)
            << "the IFC signature";
        expect(mcxx::ifc::write(sample()) == bytes) << "the same interface, the same bytes";
    };

    "an empty unit and an implementation partition"_test = [] {
        mcxx::ifc::Interface unit;
        unit.module = "m:impl";
        unit.internal = true;
        const auto back = mcxx::ifc::read(mcxx::ifc::write(unit));
        expect(back.has_value() && back->declarations.empty() && back->internal && back->module == "m:impl" && back->target.empty());
    };

    "what is not an MC2 file is an error"_test = [] {
        auto bytes = mcxx::ifc::write(sample());
        expect(!mcxx::ifc::read(std::span { bytes }.first(10)).has_value()) << "cut short";
        auto flipped = bytes;
        flipped[flipped.size() / 2] ^= std::byte { 0x5a };
        const auto hashed = mcxx::ifc::read(flipped);
        expect(!hashed.has_value() && hashed.error().contains("hash"));
        auto other = bytes;
        other[0] = std::byte { 0 };
        expect(!mcxx::ifc::read(other).has_value()) << "no signature";
        // Every truncation and every one-byte change: an answer, never a crash (the hash catches the
        // changes; a truncated file is short of its table of contents).
        for (std::size_t n { 0 }; n < bytes.size(); n += 7) (void)mcxx::ifc::read(std::span { bytes }.first(n));
    };

    "a changed declaration is found"_test = [] {
        const auto unit = sample();
        auto other = unit.declarations;
        other[2].type = "int*";
        other[5].pointer = true;
        const auto diff = mcxx::ifc::differences(unit.declarations, other);
        expect(diff.size() == 2) << diff.size();
        expect(diff.size() == 2 && diff[0].contains("type") && diff[1].contains("pointer"));
    };

    "paths beside a BMI"_test = [] {
        expect(mcxx::ifc::path_for("out/pcm.cache/app-part.pcm") == "out/pcm.cache/app-part.ifc");
        expect(mcxx::ifc::path_for("a.b/m") == "a.b/m.ifc");
    };

    "save leaves an unchanged file alone"_test = [] {
        const auto dir = std::filesystem::temp_directory_path() / std::format("mcxx-ifc-test-{}", std::random_device {}());
        std::filesystem::create_directories(dir);
        const std::string path { (dir / "m.ifc").string() };
        expect(!mcxx::ifc::save(path, sample()).has_value());
        const auto first = std::filesystem::last_write_time(path);
        std::this_thread::sleep_for(std::chrono::milliseconds { 20 });
        expect(!mcxx::ifc::save(path, sample()).has_value());
        expect(std::filesystem::last_write_time(path) == first);
        const auto back = mcxx::ifc::load(path);
        expect(back.has_value() && back->declarations.size() == sample().declarations.size());
        std::filesystem::remove_all(dir);
    };

    "an interface is found beside its BMI, or kept in the store by the BMI's content"_test = [] {
        const auto dir = std::filesystem::temp_directory_path() / std::format("mcxx-ifc-store-{}", std::random_device {}());
        std::filesystem::create_directories(dir / "build" / "pcm.cache");
        std::filesystem::create_directories(dir / "elsewhere");
        mcxx::ifc::use_store((dir / "store").string());
        const std::string bmi { (dir / "build/pcm.cache/app-part.pcm").string() };
        std::ofstream { bmi, std::ios::binary } << "not really a BMI, but bytes to hash";
        expect(!mcxx::ifc::save(mcxx::ifc::path_for(bmi), sample()).has_value());
        const auto beside = mcxx::ifc::interface_for(bmi);
        expect(beside != nullptr && beside->module == "app:part");
        mcxx::ifc::note_written(bmi, mcxx::ifc::path_for(bmi));
        mcxx::ifc::publish();
        // A build that copied the BMI (mcpp's caches) has nothing beside it: the store has it.
        const std::string copied { (dir / "elsewhere/app-part.pcm").string() };
        std::filesystem::copy_file(bmi, copied);
        std::string why;
        const auto kept = mcxx::ifc::interface_for(copied, &why);
        expect(kept != nullptr && kept->declarations.size() == sample().declarations.size()) << why;
        const std::string other { (dir / "elsewhere/other.pcm").string() };
        std::ofstream { other, std::ios::binary } << "a BMI no mcxx wrote an interface for";
        expect(mcxx::ifc::interface_for(other, &why) == nullptr && why.contains("none kept")) << why;
        mcxx::ifc::use_store({});
        std::filesystem::remove_all(dir);
    };

    // For tools/checks/ifc.py: the sample for the IFC SDK's own ifc-printer to read.
    if (const char* dump = std::getenv("MCXX_IFC_DUMP"); dump != nullptr && *dump != '\0') {
        if (const auto error = mcxx::ifc::save(dump, sample())) std::println(std::cerr, "{}", *error);
    }
    return report();
}
