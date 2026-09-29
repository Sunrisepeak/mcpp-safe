// Writing an interface: its declarations in IFC scopes, their attributes, the dialect (specs/mc2-ifc.md §2 - §5).
module;

#include <ifc/abstract-sgraph.hxx>
#include <ifc/file.hxx>

module mcxx.ifc;

import std;
import mcxx.msa;
import :format;

namespace mcxx::ifc {

namespace {

class Writer {
public:
    explicit Writer(const Interface& unit) : unit_ { unit } {}

    std::vector<std::byte> run() {
        // The T1 declarations, then the reachable ones (1.2.0).
        std::vector<Declaration> decls { unit_.declarations };
        decls.insert(decls.end(), unit_.reachable.begin(), unit_.reachable.end());
        const std::size_t own { unit_.declarations.size() };
        const std::size_t n { decls.size() };
        // Line 0 is no place (a null SourceLocation); the unit's file is the only one.
        b_.add(P_LINES, sym::FileAndLine {});
        sym::SourceFileName file {};
        clear(file);
        file.name = b_.text(unit_.source);
        b_.add(P_FILES, file);

        // Where each declaration goes: its scope (the innermost namespace or class around it; -1 the
        // global scope), its function (a parameter), its enumeration (an enumerator). The facts come
        // in the order they were found, each declaration before what it holds.
        std::vector<std::ptrdiff_t> parent(n, -1), owner(n, -1);
        std::vector<sdk::DeclSort> sort(n);
        {
            std::vector<std::size_t> open;
            for (std::size_t i { 0 }; i < own; ++i) {
                const Range& r { decls[i].range };
                while (!open.empty() && !(decls[open.back()].range.begin <= r.begin && r.end <= decls[open.back()].range.end)) open.pop_back();
                const Kind k { decls[i].kind };
                sort[i] = sort_for(k);
                for (auto it = open.rbegin(); it != open.rend(); ++it) {
                    const Kind around { decls[*it].kind };
                    if (k == Kind::parameter ? is_function(around) : k == Kind::enumerator ? around == Kind::enum_ : is_scope(around)) {
                        (k == Kind::parameter || k == Kind::enumerator ? owner : parent)[i] = static_cast<std::ptrdiff_t>(*it);
                        break;
                    }
                    if (k != Kind::parameter && k != Kind::enumerator && is_function(around)) break;
                }
                if (k == Kind::enumerator && owner[i] < 0) sort[i] = sdk::DeclSort::Barren;   // no enumeration to hold it
                if (is_scope(k) || is_function(k) || k == Kind::enum_) open.push_back(i);
            }
            // A reachable declaration has no range: its scope is the one its qualified name is in, an
            // enumerator's enumeration the one its name is in (a scoped one's).
            std::map<std::string_view, std::size_t, std::less<>> named;
            for (std::size_t i { 0 }; i < n; ++i)
                if (is_scope(decls[i].kind) || decls[i].kind == Kind::enum_) named.try_emplace(decls[i].qualified_name, i);
            for (std::size_t i { own }; i < n; ++i) {
                const Kind k { decls[i].kind };
                sort[i] = sort_for(k);
                const std::string_view q { decls[i].qualified_name };
                const std::size_t cut { q.rfind("::") };
                const auto around = cut == std::string_view::npos ? named.end() : named.find(q.substr(0, cut));
                if (k == Kind::enumerator) {
                    if (around != named.end() && decls[around->second].kind == Kind::enum_) owner[i] = static_cast<std::ptrdiff_t>(around->second);
                    else sort[i] = sdk::DeclSort::Barren;
                } else if (around != named.end() && is_scope(decls[around->second].kind)) {
                    parent[i] = static_cast<std::ptrdiff_t>(around->second);
                }
            }
        }

        // Indices. Parameters and enumerators are sequences by value: each function's, each
        // enumeration's, together, in order; a parameter no function holds after them all.
        std::vector<std::uint32_t> index(n);
        std::map<sdk::DeclSort, std::uint32_t> next;
        next[sdk::DeclSort::Barren] = 1;   // barren 0 is the dialect
        std::vector<std::vector<std::size_t>> held(n);
        std::vector<std::size_t> orphans;
        for (std::size_t i { 0 }; i < n; ++i) {
            if (sort[i] == sdk::DeclSort::Parameter || sort[i] == sdk::DeclSort::Enumerator) {
                if (owner[i] >= 0) held[static_cast<std::size_t>(owner[i])].push_back(i);
                else orphans.push_back(i);
                continue;
            }
            index[i] = next[sort[i]]++;
        }
        for (std::size_t f { 0 }; f < n; ++f)
            for (const std::size_t i : held[f]) index[i] = next[sort[i]]++;
        for (const std::size_t i : orphans) index[i] = next[sort[i]]++;
        for (const auto& [s, count] : next) b_.reserve_decls(s, count);

        // Scopes: 1 is the global scope; each namespace and class its own, in order.
        std::vector<std::uint32_t> scope_of(n, 0);
        std::uint32_t scopes { 1 };
        for (std::size_t i { 0 }; i < n; ++i)
            if (sort[i] == sdk::DeclSort::Scope) scope_of[i] = ++scopes;
        std::vector<std::vector<sdk::DeclIndex>> members(scopes + 1);
        members[1].push_back(sdk::DeclIndex { sdk::DeclSort::Barren, 0 });
        for (std::size_t i { 0 }; i < n; ++i) {
            if (sort[i] == sdk::DeclSort::Parameter || sort[i] == sdk::DeclSort::Enumerator) continue;
            members[parent[i] < 0 ? 1 : scope_of[static_cast<std::size_t>(parent[i])]].push_back(sdk::DeclIndex { sort[i], index[i] });
        }
        for (std::uint32_t s { 1 }; s <= scopes; ++s) {
            sym::Scope scope {};
            clear(scope);
            scope.start = sdk::Index { b_.count(P_MEMBERS) };
            scope.cardinality = sdk::Cardinality { static_cast<std::uint32_t>(members[s].size()) };
            for (const auto& m : members[s]) b_.add(P_MEMBERS, sym::Declaration { m });
            b_.add(P_SCOPES, scope);
        }

        // The dialect: barren 0, an attribute declaration.
        {
            std::vector<sdk::AttrIndex> items;
            items.push_back(called("mc2", { std::string { MC2_VERSION } }));
            if (!unit_.target.empty()) items.push_back(called("target", { unit_.target }));
            for (const auto& p : unit_.dialect.profiles) items.push_back(called("profile", { p }));
            for (const auto& f : unit_.dialect.features) items.push_back(called("feature", { f.id, f.level }));
            for (const auto& ns : unit_.dialect.namespaces) items.push_back(called("namespace_feature", { ns.name, ns.feature, ns.level }));
            for (const auto& m : unit_.reexports) items.push_back(called("reexport", { m }));
            sym::AttributeDir dir {};
            clear(dir);
            dir.attr = tuple(items);
            const std::uint32_t d { b_.add(P_DIR_ATTRIBUTE, dir) };
            sym::BarrenDecl barren {};
            clear(barren);
            barren.directive = sdk::DirIndex { sdk::DirSort::Attribute, d };
            b_.set(P_BARREN, 0, barren);
        }

        // The declarations, and each one's [[mcxx::decl(...)]].
        std::vector<sym::trait::DeclAttributes> attributes;
        for (std::size_t i { 0 }; i < n; ++i) {
            const Declaration& d { decls[i] };
            const sdk::DeclIndex self { sort[i], index[i] };
            const sdk::DeclIndex home { parent[i] < 0 ? sdk::DeclIndex {} : sdk::DeclIndex { sort[static_cast<std::size_t>(parent[i])], index[static_cast<std::size_t>(parent[i])] } };
            const std::string_view name { simple_name(d.qualified_name) };
            const sym::SourceLocation locus { i < own ? place(d.name.begin) : sym::SourceLocation {} };   // a reachable one: no place here
            const sdk::BasicSpecifiers spec { d.exported ? sdk::BasicSpecifiers::Cxx : sdk::BasicSpecifiers::NonExported };
            switch (sort[i]) {
            case sdk::DeclSort::Scope: {
                sym::ScopeDecl s {};
                clear(s);
                s.identity = { identifier(name), locus };
                s.type = fundamental(d.kind == Kind::namespace_ ? sym::TypeBasis::Namespace
                                     : d.kind == Kind::union_   ? sym::TypeBasis::Union
                                     : d.kind == Kind::struct_  ? sym::TypeBasis::Struct
                                                                : sym::TypeBasis::Class);
                s.initializer = sdk::ScopeIndex { scope_of[i] };
                s.home_scope = home;
                s.basic_spec = spec;
                if (name.empty()) s.scope_spec = sdk::ScopeTraits::Unnamed;
                b_.set(P_SCOPE_DECL, index[i], s);
                break;
            }
            case sdk::DeclSort::Enumeration: {
                sym::EnumerationDecl e {};
                clear(e);
                e.identity = { b_.text(name), locus };
                e.type = fundamental(sym::TypeBasis::Enum);
                if (!held[i].empty()) {
                    e.initializer.start = sdk::Index { index[held[i].front()] };
                    e.initializer.cardinality = sdk::Cardinality { static_cast<std::uint32_t>(held[i].size()) };
                }
                e.home_scope = home;
                e.basic_spec = spec;
                b_.set(P_ENUM, index[i], e);
                break;
            }
            case sdk::DeclSort::Enumerator: {
                sym::EnumeratorDecl e {};
                clear(e);
                e.identity = { b_.text(name), locus };
                e.basic_spec = spec;
                b_.set(P_ENUMERATOR, index[i], e);
                break;
            }
            case sdk::DeclSort::Alias: {
                sym::AliasDecl a {};
                clear(a);
                a.identity = { b_.text(name), locus };
                a.type = fundamental(sym::TypeBasis::Typename);
                a.home_scope = home;
                a.basic_spec = spec;
                b_.set(P_ALIAS, index[i], a);
                break;
            }
            case sdk::DeclSort::Function: {
                sym::FunctionDecl f {};
                clear(f);
                f.identity = { identifier(name), locus };
                f.home_scope = home;
                f.chart = chart(held[i], index);
                f.basic_spec = spec;
                b_.set(P_FUNCTION, index[i], f);
                break;
            }
            case sdk::DeclSort::Method: {
                sym::NonStaticMemberFunctionDecl f {};
                clear(f);
                f.identity = { identifier(name), locus };
                f.home_scope = home;
                f.chart = chart(held[i], index);
                f.basic_spec = spec;
                b_.set(P_METHOD, index[i], f);
                break;
            }
            case sdk::DeclSort::Constructor: {
                sym::ConstructorDecl f {};
                clear(f);
                f.identity = { b_.text(name), locus };
                f.home_scope = home;
                f.chart = chart(held[i], index);
                f.basic_spec = spec;
                b_.set(P_CONSTRUCTOR, index[i], f);
                break;
            }
            case sdk::DeclSort::Destructor: {
                sym::DestructorDecl f {};
                clear(f);
                f.identity = { b_.text(name), locus };
                f.home_scope = home;
                f.basic_spec = spec;
                b_.set(P_DESTRUCTOR, index[i], f);
                break;
            }
            case sdk::DeclSort::Field: {
                sym::FieldDecl f {};
                clear(f);
                f.identity = { b_.text(name), locus };
                f.home_scope = home;
                f.basic_spec = spec;
                b_.set(P_FIELD, index[i], f);
                break;
            }
            case sdk::DeclSort::Variable: {
                sym::VariableDecl v {};
                clear(v);
                v.identity = { identifier(name), locus };
                v.home_scope = home;
                v.basic_spec = spec;
                b_.set(P_VARIABLE, index[i], v);
                break;
            }
            case sdk::DeclSort::Parameter: {
                sym::ParameterDecl p {};
                clear(p);
                p.identity = { b_.text(name), locus };
                p.level = 1;
                p.position = owner[i] < 0 ? 0 : position_in(held[static_cast<std::size_t>(owner[i])], i);
                p.sort = sdk::ParameterSort::Object;
                b_.set(P_PARAMETER, index[i], p);
                break;
            }
            default: {
                sym::BarrenDecl barren {};
                clear(barren);
                barren.basic_spec = spec;
                b_.set(P_BARREN, index[i], barren);
            }
            }
            std::string flags { flags_text(d) };
            if (i >= own) flags += flags.empty() ? "reachable" : ",reachable";
            std::vector<std::string> args { std::to_string(i),     kind_name(d.kind), d.entity, d.qualified_name, d.container, d.type,
                                            std::move(flags),      range_text(d.range), range_text(d.name) };
            for (const auto& t : d.templates) args.push_back(t);
            // 1.3.0: the bases, after a `|` (which no name holds).
            if (!d.bases.empty()) {
                args.emplace_back("|");
                for (const auto& b : d.bases) args.push_back(b);
            }
            // 1.4.0: a template's parameters, after a `<` (which neither a name nor a parameter is).
            if (!d.template_parameters.empty()) {
                args.emplace_back("<");
                for (const auto& p : d.template_parameters) args.push_back(p);
            }
            sym::trait::DeclAttributes association {};
            clear(association);
            association.entity = self;
            association.trait = called("decl", args);
            attributes.push_back(association);
        }
        std::ranges::sort(attributes, [](const auto& a, const auto& x) { return ::index_like::rep(a.entity) < ::index_like::rep(x.entity); });
        for (const auto& a : attributes) b_.add(P_DECL_ATTRS, a);

        sdk::Header header {};
        clear(header);
        header.version = sdk::FormatVersion { sdk::Version { IFC_MAJOR }, sdk::Version { IFC_MINOR } };
        header.arch = architecture_of(unit_.target);
        header.cplusplus = sdk::CPlusPlus { unit_.cplusplus };
        header.unit = sdk::UnitIndex { b_.text(unit_.module), unit_.module.contains(':') ? sdk::UnitSort::Partition : sdk::UnitSort::Primary };
        header.src_path = b_.text(unit_.source);
        header.global_scope = sdk::ScopeIndex { 1 };
        header.internal_partition = unit_.internal;
        return b_.finish(header);
    }

private:
    const Interface& unit_;
    struct Decls : Builder {
        void reserve_decls(sdk::DeclSort sort, std::uint32_t count) {
            switch (sort) {
            case sdk::DeclSort::Scope: reserve<sym::ScopeDecl>(P_SCOPE_DECL, count); break;
            case sdk::DeclSort::Function: reserve<sym::FunctionDecl>(P_FUNCTION, count); break;
            case sdk::DeclSort::Method: reserve<sym::NonStaticMemberFunctionDecl>(P_METHOD, count); break;
            case sdk::DeclSort::Variable: reserve<sym::VariableDecl>(P_VARIABLE, count); break;
            case sdk::DeclSort::Field: reserve<sym::FieldDecl>(P_FIELD, count); break;
            case sdk::DeclSort::Enumeration: reserve<sym::EnumerationDecl>(P_ENUM, count); break;
            case sdk::DeclSort::Enumerator: reserve<sym::EnumeratorDecl>(P_ENUMERATOR, count); break;
            case sdk::DeclSort::Alias: reserve<sym::AliasDecl>(P_ALIAS, count); break;
            case sdk::DeclSort::Parameter: reserve<sym::ParameterDecl>(P_PARAMETER, count); break;
            case sdk::DeclSort::Constructor: reserve<sym::ConstructorDecl>(P_CONSTRUCTOR, count); break;
            case sdk::DeclSort::Destructor: reserve<sym::DestructorDecl>(P_DESTRUCTOR, count); break;
            default: reserve<sym::BarrenDecl>(P_BARREN, count);
            }
        }
    } b_;
    std::map<std::pair<std::uint32_t, std::uint32_t>, std::uint32_t> lines_;
    std::map<sym::TypeBasis, std::uint32_t> fundamentals_;

    sdk::NameIndex identifier(std::string_view name) { return sdk::NameIndex { sdk::NameSort::Identifier, sdk::to_underlying(b_.text(name)) }; }

    sym::SourceLocation place(Position p) {
        const auto [it, added] = lines_.emplace(std::pair { 0u, p.line }, static_cast<std::uint32_t>(lines_.size() + 1));
        if (added) {
            sym::FileAndLine line {};
            clear(line);
            line.file = sdk::NameIndex { sdk::NameSort::SourceFile, 0 };
            line.line = static_cast<sdk::LineNumber>(p.line + 1);
            b_.add(P_LINES, line);
        }
        sym::SourceLocation locus {};
        clear(locus);
        locus.line = sdk::LineIndex { it->second };
        locus.column = static_cast<sdk::ColumnNumber>(p.column + 1);
        return locus;
    }

    sdk::TypeIndex fundamental(sym::TypeBasis basis) {
        auto it = fundamentals_.find(basis);
        if (it == fundamentals_.end()) {
            sym::FundamentalType t {};
            clear(t);
            t.basis = basis;
            it = fundamentals_.emplace(basis, b_.add(P_FUNDAMENTAL, t)).first;
        }
        return sdk::TypeIndex { sdk::TypeSort::Fundamental, it->second };
    }

    sdk::ChartIndex chart(const std::vector<std::size_t>& parameters, const std::vector<std::uint32_t>& index) {
        if (parameters.empty()) return sdk::ChartIndex {};
        sym::UnilevelChart c {};
        clear(c);
        c.start = sdk::Index { index[parameters.front()] };
        c.cardinality = sdk::Cardinality { static_cast<std::uint32_t>(parameters.size()) };
        return sdk::ChartIndex { sdk::ChartSort::Unilevel, b_.add(P_CHART, c) };
    }

    static std::uint32_t position_in(const std::vector<std::size_t>& list, std::size_t i) {
        return static_cast<std::uint32_t>(std::ranges::find(list, i) - list.begin()) + 1;
    }

    sym::Word word(std::string_view spelling, sdk::WordSort sort) {
        sym::Word w {};
        clear(w);
        w.text = b_.text(spelling);
        if (sort == sdk::WordSort::Literal) w.src_literal = sdk::source::Literal::String;
        else w.src_identifier = sdk::source::Identifier::Plain;
        w.algebra_sort = sort;
        return w;
    }

    sdk::AttrIndex tuple(const std::vector<sdk::AttrIndex>& items) {
        sym::TupleAttr t {};
        clear(t);
        t.start = sdk::Index { b_.count(P_ATTR_HEAP) };
        t.cardinality = sdk::Cardinality { static_cast<std::uint32_t>(items.size()) };
        for (const auto& item : items) b_.add(P_ATTR_HEAP, item);
        return sdk::AttrIndex { sdk::AttrSort::Tuple, b_.add(P_ATTR_TUPLE, t) };
    }

    // mcxx::name("arg", ...): a call whose function is a scoped name and whose arguments are a tuple
    // of string literals.
    sdk::AttrIndex called(std::string_view name, const std::vector<std::string>& args) {
        sym::ScopedAttr scoped {};
        clear(scoped);
        scoped.scope = word("mcxx", sdk::WordSort::Identifier);
        scoped.member = word(name, sdk::WordSort::Identifier);
        const sdk::AttrIndex function { sdk::AttrSort::Scoped, b_.add(P_ATTR_SCOPED, scoped) };
        std::vector<sdk::AttrIndex> items;
        for (const auto& a : args) {
            sym::BasicAttr basic {};
            clear(basic);
            basic.word = word(quote(a), sdk::WordSort::Literal);
            items.push_back(sdk::AttrIndex { sdk::AttrSort::Basic, b_.add(P_ATTR_BASIC, basic) });
        }
        sym::CalledAttr call {};
        clear(call);
        call.function = function;
        call.arguments = tuple(items);
        return sdk::AttrIndex { sdk::AttrSort::Called, b_.add(P_ATTR_CALLED, call) };
    }
};

} // namespace

std::vector<msa::fact::Declaration> interface_declarations(const msa::fact::Facts& facts) {
    std::vector<msa::fact::Declaration> out;
    for (const auto& d : facts.declarations)
        if (!d.local) out.push_back(d);
    return out;
}

std::vector<std::byte> write(const Interface& unit) { return Writer { unit }.run(); }

std::string path_for(std::string_view bmi) {
    const std::size_t slash { bmi.find_last_of('/') };
    const std::size_t dot { bmi.find_last_of('.') };
    if (dot == std::string_view::npos || (slash != std::string_view::npos && dot < slash)) return std::string { bmi } + ".ifc";
    return std::string { bmi.substr(0, dot) } + ".ifc";
}

std::optional<std::string> save(const std::string& path, const Interface& unit) {
    const std::vector<std::byte> bytes { write(unit) };
    std::error_code ec;
    if (std::filesystem::file_size(path, ec) == bytes.size() && !ec) {
        std::ifstream in { path, std::ios::binary };
        std::vector<std::byte> old(bytes.size());
        if (in.read(reinterpret_cast<char*>(old.data()), static_cast<std::streamsize>(old.size())) && old == bytes) return std::nullopt;
    }
    const std::string tmp { path + ".tmp" + std::to_string(std::hash<std::thread::id> {}(std::this_thread::get_id())) };
    {
        std::ofstream out { tmp, std::ios::binary | std::ios::trunc };
        out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
        if (!out) return std::format("cannot write {}", tmp);
    }
    std::filesystem::rename(tmp, path, ec);
    if (ec) {
        std::filesystem::remove(tmp, ec);
        return std::format("cannot place {}: {}", path, ec.message());
    }
    return std::nullopt;
}

} // namespace mcxx::ifc
