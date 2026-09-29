// Reading an interface back, every offset checked (specs/mc2-ifc.md §6); two lists of declarations compared.
module;

#include <ifc/abstract-sgraph.hxx>
#include <ifc/file.hxx>

module mcxx.ifc;

import std;
import mcxx.msa;
import :format;

namespace mcxx::ifc {

namespace {

class Reader {
public:
    explicit Reader(std::span<const std::byte> bytes) : bytes_ { bytes } {}

    std::expected<Interface, std::string> run() {
        if (bytes_.size() < sizeof sdk::InterfaceSignature + sizeof(sdk::Header)) return fail("too short for an IFC header");
        if (std::memcmp(bytes_.data(), sdk::InterfaceSignature, sizeof sdk::InterfaceSignature) != 0) return fail("not an IFC file (signature)");
        std::memcpy(&header_, bytes_.data() + sizeof sdk::InterfaceSignature, sizeof header_);
        if (header_.version.major != sdk::Version { IFC_MAJOR } || header_.version.minor != sdk::Version { IFC_MINOR })
            return fail(std::format("IFC format {}.{}, MC2 v1 reads {}.{}", static_cast<int>(header_.version.major),
                                    static_cast<int>(header_.version.minor), IFC_MAJOR, IFC_MINOR));
        constexpr std::size_t hashed { sizeof sdk::InterfaceSignature + sizeof(sdk::SHA256Hash) };
        const sdk::SHA256Hash hash { sdk::hash_bytes(bytes_.data() + hashed, bytes_.data() + bytes_.size()) };
        if (std::memcmp(bytes_.data() + sizeof sdk::InterfaceSignature, hash.value.data(), sizeof hash.value) != 0)
            return fail("content hash does not match");
        const std::size_t strings_at { sdk::to_underlying(header_.string_table_bytes) };
        const std::size_t strings_size { sdk::to_underlying(header_.string_table_size) };
        if (strings_at + strings_size > bytes_.size() || strings_size == 0 || static_cast<char>(bytes_[strings_at + strings_size - 1]) != '\0')
            return fail("string table out of bounds");
        strings_ = { reinterpret_cast<const char*>(bytes_.data() + strings_at), strings_size };
        const std::size_t toc_at { sdk::to_underlying(header_.toc) };
        const std::size_t parts { sdk::to_underlying(header_.partition_count) };
        if (toc_at + parts * sizeof(sdk::PartitionSummaryData) > bytes_.size()) return fail("table of contents out of bounds");
        for (std::size_t i { 0 }; i < parts; ++i) {
            sdk::PartitionSummaryData s {};
            clear(s);
            std::memcpy(&s, bytes_.data() + toc_at + i * sizeof s, sizeof s);
            const auto name = text(s.name);
            if (!name) return fail("a partition's name is out of bounds");
            const std::size_t size { static_cast<std::size_t>(sdk::to_underlying(s.cardinality)) * sdk::to_underlying(s.entry_size) };
            if (sdk::to_underlying(s.offset) + size > bytes_.size()) return fail(std::format("partition {} out of bounds", *name));
            toc_.emplace(std::string { *name }, s);
        }

        Interface unit;
        const auto module = text(sdk::TextOffset { sdk::to_underlying(header_.unit.index()) });
        if (!module || (header_.unit.sort() != sdk::UnitSort::Primary && header_.unit.sort() != sdk::UnitSort::Partition))
            return fail("not a module interface unit");
        unit.module = *module;
        unit.internal = header_.internal_partition;
        unit.source = text(header_.src_path).value_or("");
        unit.cplusplus = sdk::to_underlying(header_.cplusplus);

        // The dialect: the global scope's attribute declarations.
        const auto global = entry<sym::Scope>(P_SCOPES, 0);
        if (!global) return fail("no global scope");
        bool versioned { false };
        for (std::uint32_t m { 0 }; m < sdk::to_underlying(global->cardinality); ++m) {
            const auto member = entry<sym::Declaration>(P_MEMBERS, sdk::to_underlying(global->start) + m);
            if (!member) return fail("a scope member out of bounds");
            if (member->index.sort() != sdk::DeclSort::Barren) continue;
            const auto barren = entry<sym::BarrenDecl>(P_BARREN, sdk::to_underlying(member->index.index()));
            if (!barren) return fail("a barren declaration out of bounds");
            if (barren->directive.sort() != sdk::DirSort::Attribute) continue;
            const auto dir = entry<sym::AttributeDir>(P_DIR_ATTRIBUTE, sdk::to_underlying(barren->directive.index()));
            if (!dir) return fail("an attribute declaration out of bounds");
            const auto items = elements(dir->attr);
            if (!items) return fail(items.error());
            for (const auto item : *items) {
                auto c = call(item);
                if (!c) return fail(c.error());
                auto& [name, args] = *c;
                const auto arity = [&](std::size_t k) { return args.size() == k; };
                if (name == "mc2" && arity(1)) {
                    if (!args[0].starts_with("1.")) return fail(std::format("MC2 {}; this reader reads 1.x", args[0]));
                    versioned = true;
                } else if (name == "target" && arity(1)) unit.target = args[0];
                else if (name == "profile" && arity(1)) unit.dialect.profiles.push_back(args[0]);
                else if (name == "feature" && arity(2)) unit.dialect.features.push_back({ args[0], args[1] });
                else if (name == "namespace_feature" && arity(3)) unit.dialect.namespaces.push_back({ args[0], args[1], args[2] });
                else if (name == "reexport" && arity(1)) unit.reexports.push_back(args[0]);
                else return fail(std::format("unknown dialect item mcxx::{} with {} arguments", name, args.size()));
            }
        }
        if (!versioned) return fail("no mcxx::mc2 version: not written by MC2");

        // The declarations: every one with an [[mcxx::decl]], back in the facts' order.
        std::vector<std::pair<std::size_t, std::pair<bool, Declaration>>> found;   // ordinal, (reachable, declaration)
        const auto it = toc_.find(P_DECL_ATTRS);
        const std::uint32_t count { it == toc_.end() ? 0 : sdk::to_underlying(it->second.cardinality) };
        for (std::uint32_t i { 0 }; i < count; ++i) {
            const auto association = entry<sym::trait::DeclAttributes>(P_DECL_ATTRS, i);
            if (!association) return fail("a declaration's attribute out of bounds");
            auto c = call(association->trait);
            if (!c) return fail(c.error());
            auto& [name, args] = *c;
            if (name != "decl" || args.size() < 9) return fail(std::format("declaration attribute mcxx::{} with {} arguments", name, args.size()));
            std::size_t ordinal { 0 };
            if (std::from_chars(args[0].data(), args[0].data() + args[0].size(), ordinal).ec != std::errc {}) return fail("a declaration's ordinal: " + args[0]);
            Declaration d;
            const auto kind = kind_from(args[1]);
            if (!kind) return fail("unknown kind " + args[1]);
            d.kind = *kind;
            const sdk::DeclSort sort { association->entity.sort() };
            if (sort != sort_for(d.kind) && !(d.kind == Kind::enumerator && sort == sdk::DeclSort::Barren))
                return fail(std::format("declaration {} ({}) is a {} written as sort {}", ordinal, args[3], args[1], static_cast<int>(sort)));
            if (!declared(association->entity)) return fail(std::format("declaration {} ({}) out of bounds", ordinal, args[3]));
            d.entity = std::move(args[2]);
            d.qualified_name = std::move(args[3]);
            d.container = std::move(args[4]);
            d.type = std::move(args[5]);
            bool reachable { false };
            for (const auto flag : std::views::split(std::string_view { args[6] }, ',')) {
                const std::string_view f { flag.begin(), flag.end() };
                if (f.empty()) continue;
                if (f == "reachable") {
                    reachable = true;
                    continue;
                }
                const auto known = std::ranges::find(FLAGS, f, &std::pair<bool Declaration::*, std::string_view>::second);
                if (known == std::end(FLAGS)) return fail(std::format("declaration {}: unknown flag {}", ordinal, f));
                d.*(known->first) = true;
            }
            const auto range = parse_range(args[7]);
            const auto name_range = parse_range(args[8]);
            if (!range || !name_range) return fail(std::format("declaration {}: a range is not line:column-line:column", ordinal));
            d.range = *range;
            d.name = *name_range;
            // The templates; 1.3.0: after `|`, the bases; 1.4.0: after `<`, the template parameters.
            auto* into { &d.templates };
            for (std::size_t t { 9 }; t < args.size(); ++t) {
                if (args[t] == "|") into = &d.bases;
                else if (args[t] == "<") into = &d.template_parameters;
                else into->push_back(std::move(args[t]));
            }
            found.emplace_back(ordinal, std::pair { reachable, std::move(d) });
        }
        std::ranges::sort(found, {}, &std::pair<std::size_t, std::pair<bool, Declaration>>::first);
        for (std::size_t i { 0 }; i < found.size(); ++i) {
            if (found[i].first != i) return fail(std::format("declaration ordinals are not 0..{}", found.size() - 1));
            auto& [reachable, d] = found[i].second;
            // The T1 declarations first, then the reachable ones.
            if (reachable) unit.reachable.push_back(std::move(d));
            else if (!unit.reachable.empty()) return fail(std::format("declaration {} (a T1 one) after a reachable one", i));
            else unit.declarations.push_back(std::move(d));
        }
        return unit;
    }

private:
    std::span<const std::byte> bytes_;
    sdk::Header header_ {};
    std::string_view strings_;
    std::map<std::string, sdk::PartitionSummaryData, std::less<>> toc_;

    static std::unexpected<std::string> fail(std::string message) { return std::unexpected { std::move(message) }; }

    std::optional<std::string_view> text(sdk::TextOffset offset) const {
        const std::size_t at { sdk::to_underlying(offset) };
        if (at == 0) return std::string_view {};
        if (at >= strings_.size()) return std::nullopt;
        return std::string_view { strings_.data() + at };   // the table ends with a NUL (checked)
    }

    template<typename T>
    std::optional<T> entry(std::string_view partition, std::uint32_t index) const {
        const auto it = toc_.find(partition);
        if (it == toc_.end() || sdk::to_underlying(it->second.entry_size) != sizeof(T) || index >= sdk::to_underlying(it->second.cardinality))
            return std::nullopt;
        T value;
        std::memcpy(&value, bytes_.data() + sdk::to_underlying(it->second.offset) + static_cast<std::size_t>(index) * sizeof(T), sizeof(T));
        return value;
    }

    bool declared(sdk::DeclIndex index) const {
        const auto it = toc_.find(partition_of(index.sort()));
        return it != toc_.end() && sdk::to_underlying(index.index()) < sdk::to_underlying(it->second.cardinality);
    }

    std::expected<std::vector<sdk::AttrIndex>, std::string> elements(sdk::AttrIndex index) const {
        if (index.sort() != sdk::AttrSort::Tuple) return fail("an attribute list is not a tuple");
        const auto t = entry<sym::TupleAttr>(P_ATTR_TUPLE, sdk::to_underlying(index.index()));
        if (!t) return fail("a tuple attribute out of bounds");
        std::vector<sdk::AttrIndex> out;
        for (std::uint32_t k { 0 }; k < sdk::to_underlying(t->cardinality); ++k) {
            const auto item = entry<sdk::AttrIndex>(P_ATTR_HEAP, sdk::to_underlying(t->start) + k);
            if (!item) return fail("an attribute heap entry out of bounds");
            out.push_back(*item);
        }
        return out;
    }

    // mcxx::name("arg", ...) -> name, the arguments unquoted.
    std::expected<std::pair<std::string, std::vector<std::string>>, std::string> call(sdk::AttrIndex index) const {
        if (index.sort() != sdk::AttrSort::Called) return fail("an MC2 attribute is not a call");
        const auto c = entry<sym::CalledAttr>(P_ATTR_CALLED, sdk::to_underlying(index.index()));
        if (!c || c->function.sort() != sdk::AttrSort::Scoped) return fail("an MC2 attribute's name is not scoped");
        const auto scoped = entry<sym::ScopedAttr>(P_ATTR_SCOPED, sdk::to_underlying(c->function.index()));
        if (!scoped) return fail("a scoped attribute out of bounds");
        const auto scope = text(scoped->scope.text);
        const auto member = text(scoped->member.text);
        if (!scope || !member || *scope != "mcxx") return fail("an attribute outside mcxx::");
        const auto items = elements(c->arguments);
        if (!items) return std::unexpected { items.error() };
        std::vector<std::string> args;
        for (const auto item : *items) {
            if (item.sort() != sdk::AttrSort::Basic) return fail("an MC2 attribute's argument is not a word");
            const auto basic = entry<sym::BasicAttr>(P_ATTR_BASIC, sdk::to_underlying(item.index()));
            if (!basic || basic->word.algebra_sort != sdk::WordSort::Literal) return fail("an MC2 attribute's argument is not a literal");
            const auto spelling = text(basic->word.text);
            if (!spelling) return fail("an argument's text out of bounds");
            auto value = unquote(*spelling);
            if (!value) return fail(std::format("an argument is not a string literal: {}", *spelling));
            args.push_back(std::move(*value));
        }
        return std::pair { std::string { *member }, std::move(args) };
    }
};

} // namespace

std::expected<Interface, std::string> read(std::span<const std::byte> bytes) { return Reader { bytes }.run(); }

std::expected<Interface, std::string> load(const std::string& path) {
    std::ifstream in { path, std::ios::binary };
    if (!in) return std::unexpected { std::format("cannot read {}", path) };
    std::vector<char> text { std::istreambuf_iterator<char> { in }, {} };
    std::vector<std::byte> bytes(text.size());
    std::memcpy(bytes.data(), text.data(), text.size());
    auto unit = read(bytes);
    if (!unit) return std::unexpected { std::format("{}: {}", path, unit.error()) };
    return unit;
}

std::vector<std::string> differences(std::span<const msa::fact::Declaration> expected, std::span<const msa::fact::Declaration> actual,
                                     std::size_t limit) {
    std::vector<std::string> out;
    const auto note = [&](std::string line) {
        if (out.size() < limit) out.push_back(std::move(line));
    };
    if (expected.size() != actual.size()) note(std::format("{} declarations, {} read back", expected.size(), actual.size()));
    for (std::size_t i { 0 }; i < std::min(expected.size(), actual.size()); ++i) {
        const auto& e = expected[i];
        const auto& a = actual[i];
        const auto field = [&](std::string_view name, const auto& x, const auto& y, auto show) {
            if (!(x == y)) note(std::format("declaration {} ({}): {} `{}` != `{}`", i, e.qualified_name, name, show(x), show(y)));
        };
        const auto str = [](const std::string& s) { return s; };
        const auto rng = [](const Range& r) { return range_text(r); };
        const auto kind = [](Kind k) { return kind_name(k); };
        const auto flag = [](bool b) { return std::string { b ? "true" : "false" }; };
        const auto list = [](const std::vector<std::string>& v) {
            std::string s;
            for (const auto& x : v) s += (s.empty() ? "" : ",") + x;
            return s;
        };
        field("range", e.range, a.range, rng);
        field("name", e.name, a.name, rng);
        field("container", e.container, a.container, str);
        field("entity", e.entity, a.entity, str);
        field("qualified name", e.qualified_name, a.qualified_name, str);
        field("kind", e.kind, a.kind, kind);
        field("type", e.type, a.type, str);
        field("templates", e.templates, a.templates, list);
        field("bases", e.bases, a.bases, list);
        field("template parameters", e.template_parameters, a.template_parameters, list);
        for (const auto& [member, name] : FLAGS) field(name, e.*member, a.*member, flag);
    }
    return out;
}

} // namespace mcxx::ifc
