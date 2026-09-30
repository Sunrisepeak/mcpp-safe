// What an expression's type is, for member access (mcxx.frontend:lookup): :resolver's typing. A
// declaration's type as written; `auto` deduced from its initializer or its range; a member's type read
// in its object's specialization -- the class template's parameters bound to the arguments written and
// then to their defaults (MC3 0.6.0's template-parameters for an imported template, the template-head
// for the file's own); an alias followed to what it names.
module mcxx.frontend;

import std;
import mcxx.msa;
import :resolver;

namespace mcxx::frontend::resolution {

std::optional<Typed> Resolver::typed(const Target& t, int depth, bool chosen) {
    // A function F1 cannot tell the overload of: its overloads' return types differ (a getter and a
    // setter), and which is called needs the arguments' types (call_typed chooses by them). `reference`
    // and `const_reference` of a const and a non-const one are the same type for this.
    if (!chosen && (t.kind == msa::Kind::function || t.kind == msa::Kind::method)) {
        if (const auto s = scopes_.find(scope_of(t.qualified)); s != scopes_.end())
            if (const auto m = s->second.find(last_component(t.qualified)); m != s->second.end() && m->second.size() > 1) {
                const auto plain = [](std::string type) {
                    type = bare(std::move(type));
                    for (std::size_t at { type.find("const_") }; at != std::string::npos; at = type.find("const_"))
                        type.erase(at, 6);
                    return type;
                };
                const std::string mine { plain(t.type) };
                for (const auto& other : m->second)
                    if (other.kind == t.kind && plain(other.type) != mine) return std::nullopt;
            }
    }
    if (t.declaration >= 0) {
        const auto& d = ds_[static_cast<std::size_t>(t.declaration)];
        if (t.kind == msa::Kind::variable && d.binding) {
            auto element { binding_type(static_cast<std::size_t>(t.declaration), depth) };
            if (!element) deduced_placeholder_ = true;
            return element;
        }
        if (t.kind == msa::Kind::variable && placeholder(d)) {
            auto deduced { deduce(static_cast<std::size_t>(t.declaration), depth) };
            if (!deduced) {
                deduced_placeholder_ = true;
                return std::nullopt;
            }
            // `auto*` keeps what is written around the placeholder: what is deduced is what
            // it points to. (`auto&` refers to it: the same type for a member access.)
            bool pointer { false };
            for (std::uint32_t k { d.declarator_begin }; k < d.id_begin && k < t_.size(); ++k) pointer = pointer || is(k, Kind::star);
            if (pointer && !bare(deduced->text).ends_with('*')) deduced->text += " *";
            return deduced;
        }
        if (t.type.empty()) return std::nullopt;
        return iterator_of(Typed { t.type, d.name_token, {}, false, {} });
    }
    if (t.type.empty()) return std::nullopt;
    // A function template's return type deduced from its body: not in its interface (A2.2.3).
    if (placeholder_text(t.type)) {
        deduced_placeholder_ = true;
        return std::nullopt;
    }
    return iterator_of(Typed { t.type, 0, scope_of(t.qualified), true, {} });
}

bool Resolver::iterator_name(std::string_view text) {
    const std::string name { class_name_of(std::string { text }) };
    const std::string_view last { last_component(name) };
    return last.ends_with("iterator");
}

Typed Resolver::iterator_of(Typed type) const {
    const std::string text { bare(type.text) };
    const auto at = text.rfind("::");
    // A container's member iterator (`std::vector<T>::iterator`), not a class of a namespace whose
    // name ends so (`std::filesystem::directory_iterator`: its own operators say what it gives).
    if (at != std::string::npos && iterator_name(text) && at > 0 && !namespaces_.contains(text.substr(0, at))) type.iterating = text.substr(0, at);
    return type;
}

bool Resolver::placeholder_text(std::string_view type) {
    for (std::size_t at { type.find("auto") }; at != std::string_view::npos; at = type.find("auto", at + 1)) {
        const bool starts { at == 0 || !(std::isalnum(static_cast<unsigned char>(type[at - 1])) || type[at - 1] == '_') };
        const bool ends { at + 4 >= type.size() || !(std::isalnum(static_cast<unsigned char>(type[at + 4])) || type[at + 4] == '_') };
        if (starts && ends) return true;
    }
    return false;
}

bool Resolver::placeholder(const Declaration& d) const {
    if (d.capture) return true;   // an init-capture: deduced as `auto` ([expr.prim.lambda.capture]/6)
    for (std::uint32_t k { d.specifiers_begin }; k < d.specifiers_end && k < t_.size(); ++k)
        if (word(k, "auto")) return true;
    return false;
}

std::optional<Typed> Resolver::deduce(std::size_t i, int depth) {
    if (const auto known = deduced_.find(i); known != deduced_.end()) return known->second;
    deduced_[i] = std::nullopt;   // while being worked out: a variable deduced from itself is not
    const auto& d = ds_[i];
    std::optional<Typed> out;
    std::size_t k { d.declarator_end };
    if (k < t_.size() && is(k, Kind::colon)) {
        // `for (auto& x : range)`: the range runs to the `)` that closes the for's parentheses.
        int depth_parens { 0 };
        std::size_t end { k + 1 };
        for (; end < t_.size(); ++end) {
            if (is(end, Kind::l_paren) || is(end, Kind::l_square) || is(end, Kind::l_brace)) ++depth_parens;
            else if (is(end, Kind::r_paren) || is(end, Kind::r_square) || is(end, Kind::r_brace)) {
                if (depth_parens == 0) break;
                --depth_parens;
            }
        }
        // `for (auto* x : { &a, &b })`: an initializer list's elements, its first one's type.
        if (end > k + 3 && is(k + 1, Kind::l_brace) && opening_forward(k + 1) == end - 1) {
            std::size_t last { k + 2 };
            for (int nesting { 0 }; last < end - 1; ++last) {
                if (is(last, Kind::l_paren) || is(last, Kind::l_square) || is(last, Kind::l_brace)) ++nesting;
                else if (is(last, Kind::r_paren) || is(last, Kind::r_square) || is(last, Kind::r_brace)) --nesting;
                else if (nesting == 0 && is(last, Kind::comma)) break;
            }
            if (last > k + 2) out = initializer_type(k + 2, last - 1, depth + 1);
        } else if (end > k + 1) {
            if (splits(k + 1, end - 1)) {
                // `r | std::views::split(d)`: its elements are subranges of r ([range.split.view]); which
                // specialization, Clang's sugar says (not listed as certain).
                Typed element { "std::ranges::subrange", k + 1, {}, false, {}, nullptr };
                element.through_template = true;
                out = element;
            } else if (auto range = initializer_type(k + 1, end - 1, depth + 1)) {
                out = range_element(*range);
            }
        }
    } else if (k < t_.size() && (is(k, Kind::equal) || is(k, Kind::l_brace) || is(k, Kind::l_paren))) {
        // The initializer's last token: before the `;` that ends the declaration, inside the
        // braces or parentheses that are the initializer.
        std::size_t end { d.last_token };
        while (end > k && (is(end, Kind::semi) || is(end, Kind::comma))) --end;
        if (!is(k, Kind::equal) && end > k && (is(end, Kind::r_brace) || is(end, Kind::r_paren))) --end;
        if (end > k) out = initializer_type(k + 1, end, depth + 1);
        if (out) out->text = bare(out->text);
    }
    deduced_[i] = out;
    return out;
}

std::optional<Typed> Resolver::initializer_type(std::size_t begin, std::size_t end, int depth) {
    if (begin > end || end >= t_.size()) return std::nullopt;
    // A lambda: its closure type, a class of its own that names nothing -- unless it is called at once
    // (`[&]() -> std::string { ... }()`), which gives what it returns: its trailing return type.
    if (is(begin, Kind::l_square) && !is(begin + 1, Kind::l_square)) {
        const auto lambda { lambda_at(begin, end) };
        const bool called { lambda && lambda->end < end && is(lambda->end + 1, Kind::l_paren) && opening_forward(lambda->end + 1) == end };
        if (!called) return Typed { "(lambda)", begin, {}, false, {}, nullptr };
        // Its return type deduced from its body: its first `return`'s expression's (every one has it).
        if (lambda->arrow == 0) return first_return(lambda->body, lambda->end, depth);
        const std::string returned { type_text(std::span { t_ }.subspan(lambda->arrow + 1, lambda->body - lambda->arrow - 1)) };
        if (returned.empty()) return std::nullopt;
        return Typed { returned, lambda->arrow + 1, {}, false, {}, nullptr };
    }
    // The operators between operands at the top. Most make an expression F1 does not type (its last
    // operand's type is not the whole's); these it does: a condition whose branches have one type, a
    // string's `+`, a path's `/`.
    const auto operand_end = [&](std::size_t k) {
        return is(k, Kind::raw_identifier) || is(k, Kind::numeric_constant) || is(k, Kind::r_paren) || is(k, Kind::r_square) ||
               is(k, Kind::r_brace) || is(k, Kind::string_literal) || is(k, Kind::char_constant);
    };
    int nesting { 0 };
    // A condition: all before its `?` is the condition, whatever operators are there.
    for (std::size_t k { begin }; k <= end; ++k) {
        if (is(k, Kind::l_paren) || is(k, Kind::l_square) || is(k, Kind::l_brace)) ++nesting;
        else if (is(k, Kind::r_paren) || is(k, Kind::r_square) || is(k, Kind::r_brace)) --nesting;
        else if (nesting == 0 && is(k, Kind::question)) return conditional_type(k, end, depth);
    }
    std::vector<std::size_t> pluses, slashes;
    nesting = 0;
    for (std::size_t k { begin }; k <= end; ++k) {
        if (is(k, Kind::l_paren) || is(k, Kind::l_square) || is(k, Kind::l_brace)) ++nesting;
        else if (is(k, Kind::r_paren) || is(k, Kind::r_square) || is(k, Kind::r_brace)) --nesting;
        if (nesting != 0) continue;
        const Kind kind { t_[k].kind };
        if (kind == Kind::pipepipe || kind == Kind::ampamp || kind == Kind::percent || kind == Kind::pipe || kind == Kind::caret ||
            kind == Kind::equalequal || kind == Kind::exclaimequal || kind == Kind::lessequal || kind == Kind::greaterequal ||
            kind == Kind::spaceship || kind == Kind::lessless)
            return std::nullopt;
        if (kind == Kind::slash) slashes.push_back(k);
        else if (kind == Kind::plus && k > begin && operand_end(k - 1)) pluses.push_back(k);
        else if ((kind == Kind::minus || kind == Kind::star || kind == Kind::amp) && k > begin && operand_end(k - 1)) return std::nullopt;
        // `a < b`: a comparison when what precedes is an object, not a template.
        if (kind == Kind::less && k > begin && resolved_[k - 1] &&
            (resolved_[k - 1]->kind == msa::Kind::variable || resolved_[k - 1]->kind == msa::Kind::field || resolved_[k - 1]->kind == msa::Kind::parameter))
            return std::nullopt;
    }
    if (!pluses.empty() && !slashes.empty()) return std::nullopt;
    // `s + "x" + t`: a std::basic_string when one of the operands is one ([string.op.plus]).
    if (!pluses.empty()) {
        std::size_t from { begin };
        pluses.push_back(end + 1);
        for (const auto op : pluses) {
            if (op > from)
                if (auto operand { initializer_type(from, op - 1, depth + 1) })
                    if (class_of(*operand) == std::optional<std::string> { "std::basic_string" }) return operand;
            from = op + 1;
        }
        return std::nullopt;
    }
    // `p / "x" / q`: a std::filesystem::path when the first operand is one ([fs.path.nonmember]).
    if (!slashes.empty()) {
        auto first { initializer_type(begin, slashes.front() - 1, depth + 1) };
        if (first && class_of(*first) == std::optional<std::string> { "std::filesystem::path" }) return first;
        return std::nullopt;
    }
    if (is(begin, Kind::star)) {
        const auto inner { initializer_type(begin + 1, end, depth + 1) };
        return inner ? pointee(*inner) : std::nullopt;
    }
    if (is(begin, Kind::amp)) {
        auto inner { initializer_type(begin + 1, end, depth + 1) };
        if (inner) inner->text = bare(inner->text) + " *";
        return inner;
    }
    if (is(begin, Kind::exclaim) || is(begin, Kind::tilde) || is(begin, Kind::plus) || is(begin, Kind::minus) || is(begin, Kind::plusplus) ||
        is(begin, Kind::minusminus))
        return std::nullopt;
    return expression_type(end, depth);
}

std::optional<Typed> Resolver::conditional_type(std::size_t question, std::size_t end, int depth) {
    // Its `:`: the one at the top that is not a nested condition's.
    std::size_t colon { 0 };
    int nesting { 0 }, inner { 0 };
    for (std::size_t k { question + 1 }; k <= end && colon == 0; ++k) {
        if (is(k, Kind::l_paren) || is(k, Kind::l_square) || is(k, Kind::l_brace)) ++nesting;
        else if (is(k, Kind::r_paren) || is(k, Kind::r_square) || is(k, Kind::r_brace)) --nesting;
        else if (nesting == 0 && is(k, Kind::question)) ++inner;
        else if (nesting == 0 && is(k, Kind::colon) && inner-- == 0) colon = k;
    }
    if (colon == 0 || colon == question + 1 || colon == end) return std::nullopt;
    // Both branches of one type: the whole's ([expr.cond]); otherwise F1 does not say which.
    const auto first { initializer_type(question + 1, colon - 1, depth + 1) };
    const auto second { initializer_type(colon + 1, end, depth + 1) };
    if (!first || !second) return std::nullopt;
    if (bare(first->text) == bare(second->text)) return first;
    const auto a { class_of(*first) }, b { class_of(*second) };
    const bool plain { bare(first->text).find('<') == std::string::npos && bare(second->text).find('<') == std::string::npos };
    if (a && a == b && plain) return first;
    return std::nullopt;
}

std::optional<Typed> Resolver::binding_type(std::size_t i, int depth) {
    std::size_t group { i };
    while (group > 0 && !ds_[--group].name.starts_with('[')) {}
    if (!ds_[group].name.starts_with('[')) return std::nullopt;
    std::size_t place { 0 };
    for (std::size_t j { group + 1 }; j < i; ++j) place += ds_[j].binding ? 1 : 0;
    const auto whole { binding_group(group, depth) };
    if (!whole) return std::nullopt;
    const Typed type { unbound(*whole) };
    const std::string text { bare(type.text) };
    const auto cls { class_of(type) };
    if (!cls) return std::nullopt;
    const auto args { arguments_of(text) };
    if ((*cls == "std::pair" || *cls == "std::tuple") && place < args.size()) {
        const Typed element { derived(type, bare(args[place])) };
        // `auto [it, added] = m.try_emplace(k, v)`: the container's `iterator`, of the object's type.
        const std::string text { bare(element.text) };
        if (element.iterating.empty() && iterator_name(text) && text.find("::") == std::string::npos && whole->bindings &&
            whole->bindings->specialization) {
            const Typed& container { *whole->bindings->specialization };
            return Typed { element.text, container.at, container.context, container.imported, bare(container.text), container.bindings };
        }
        return element;
    }
    if (*cls == "std::array" && !args.empty()) return derived(type, bare(args[0]));
    if (const auto c = classes_.find(*cls); c != classes_.end()) {
        std::size_t n { 0 };
        for (std::size_t j { static_cast<std::size_t>(c->second) + 1 }; j < ds_.size(); ++j)
            if (ds_[j].parent == c->second && ds_[j].kind == msa::Kind::field && n++ == place)
                return member_typed(target_of(static_cast<std::int32_t>(j)), type, depth + 1);
    }
    return std::nullopt;
}

std::optional<Typed> Resolver::binding_group(std::size_t group, int depth) {
    // The `]` that closes the names, then the initializer or the range.
    std::size_t k { ds_[group].name_token };
    while (k < t_.size() && !is(k, Kind::r_square)) ++k;
    ++k;
    std::optional<Typed> whole;
    if (is(k, Kind::colon)) {
        int nesting { 0 };
        std::size_t end { k + 1 };
        for (; end < t_.size(); ++end) {
            if (is(end, Kind::l_paren) || is(end, Kind::l_square) || is(end, Kind::l_brace)) ++nesting;
            else if (is(end, Kind::r_paren) || is(end, Kind::r_square) || is(end, Kind::r_brace)) {
                if (nesting == 0) break;
                --nesting;
            }
        }
        if (end > k + 1)
            if (auto range = initializer_type(k + 1, end - 1, depth + 1)) whole = range_element(*range);
    } else if (is(k, Kind::equal) || is(k, Kind::l_brace) || is(k, Kind::l_paren)) {
        std::size_t end { ds_[group].last_token };
        while (end > k && (is(end, Kind::semi) || is(end, Kind::comma))) --end;
        if (!is(k, Kind::equal) && end > k && (is(end, Kind::r_brace) || is(end, Kind::r_paren))) --end;
        if (end > k) whole = initializer_type(k + 1, end, depth + 1);
    }
    return whole;
}

std::optional<Typed> Resolver::range_element(const Typed& written) {
    const Typed type { unbound(written) };
    const std::string text { bare(type.text) };
    if (text.ends_with('*')) return std::nullopt;   // a pointer is no range
    if (const auto square = text.find('['); square != std::string::npos && text.find('<') > square)
        return derived(type, bare(text.substr(0, square)));
    const auto cls { class_of(type) };
    if (!cls) return std::nullopt;
    const auto args { arguments_of(text) };
    static constexpr std::string_view sequences[] { "std::vector", "std::array", "std::span", "std::deque", "std::list", "std::set",
                                                     "std::unordered_set", "std::initializer_list", "std::multiset", "std::forward_list" };
    static constexpr std::string_view maps[] { "std::map", "std::unordered_map", "std::multimap", "std::flat_map" };
    // An associative container's element: its class is right, but Clang's type keeps libc++'s node
    // sugar (`__get_node_value_type_t`), which MC3's templates list names.
    static constexpr std::string_view associative[] { "std::set", "std::unordered_set", "std::multiset" };
    if (std::ranges::contains(sequences, *cls) && !args.empty()) {
        auto element { derived(type, args[0]) };
        element.through_template = std::ranges::contains(associative, *cls);
        return element;
    }
    if (std::ranges::contains(maps, *cls) && args.size() >= 2) {
        auto element { derived(type, std::format("std::pair<const {}, {}>", args[0], args[1])) };
        element.through_template = true;
        return element;
    }
    // A json value's elements are json values (nlohmann's iter_impl gives a reference to one).
    if (*cls == "nlohmann::basic_json") return type;
    // A directory's iterators give its entries ([fs.class.directory.iterator]).
    if (*cls == "std::filesystem::directory_iterator" || *cls == "std::filesystem::recursive_directory_iterator")
        return Typed { "::std::filesystem::directory_entry", type.at, {}, false };
    return std::nullopt;
}

std::optional<std::string> Resolver::class_of(const Typed& written) {
    const Typed type { unbound(written) };
    const std::string name { class_name_of(type.text) };
    if (name.empty()) return std::nullopt;
    return type.imported ? scope_named_in(name, type.context) : scope_named(name, type.at);
}

std::string Resolver::bare(std::string text) {
    for (std::string_view prefix : { "const ", "volatile " })
        while (text.starts_with(prefix)) text.erase(0, prefix.size());
    while (!text.empty() && (text.back() == '&' || text.back() == ' ')) text.pop_back();
    for (std::string_view suffix : { " const", " volatile" })
        while (text.ends_with(suffix)) text.erase(text.size() - suffix.size());
    return text;
}

std::vector<std::string> Resolver::arguments_of(const std::string& text) {
    std::vector<std::string> out;
    const auto open = text.find('<');
    if (open == std::string::npos) return out;
    int depth { 0 };
    std::string current;
    for (std::size_t i { open + 1 }; i < text.size(); ++i) {
        const char c { text[i] };
        if (c == '<' || c == '(' || c == '[') ++depth;
        if ((c == '>' || c == ')' || c == ']') && depth-- == 0) {
            out.push_back(current);
            return out;
        }
        if (c == ',' && depth == 0) {
            out.push_back(current);
            current.clear();
            continue;
        }
        if (!(current.empty() && c == ' ')) current += c;
    }
    return {};
}

std::optional<Typed> Resolver::pointee(const Typed& written) {
    // An iterator's `*` and `->`: its container's element (a json value's iterators give json values).
    if (!written.iterating.empty()) {
        const Typed container { written.iterating, written.at, written.context, written.imported, {}, written.bindings };
        if (const auto cls = class_of(container); cls && *cls == "nlohmann::basic_json") return container;
        if (auto element = range_element(container)) return element;
    }
    const Typed type { unbound(written) };
    std::string text { bare(type.text) };
    if (text.ends_with('*')) {
        text.pop_back();
        return derived(type, bare(text));
    }
    const auto cls { class_of(type) };
    if (!cls) return std::nullopt;
    if (*cls == "std::unique_ptr" || *cls == "std::shared_ptr" || *cls == "std::optional" || *cls == "std::weak_ptr") {
        const auto args { arguments_of(text) };
        if (args.empty()) return std::nullopt;
        return derived(type, args.front());
    }
    // A class's own `operator->`: what the pointer it gives points to (`std::expected`'s value).
    if (const auto arrow = operator_typed(type, "operator->")) {
        const Typed pointer { unbound(*arrow) };
        std::string to { bare(pointer.text) };
        if (to.ends_with('*')) {
            to.pop_back();
            return derived(pointer, bare(to));
        }
    }
    return std::nullopt;
}

std::optional<Typed> Resolver::element(const Typed& written) {
    const Typed type { unbound(written) };
    std::string text { bare(type.text) };
    if (text.ends_with('*')) {
        text.pop_back();
        return derived(type, bare(text));
    }
    if (const auto square = text.find('['); square != std::string::npos && text.find('<') > square)
        return derived(type, bare(text.substr(0, square)));
    const auto cls { class_of(type) };
    if (!cls) return std::nullopt;
    const auto args { arguments_of(text) };
    static constexpr std::string_view sequences[] { "std::vector", "std::array", "std::span", "std::deque", "std::basic_string_view",
                                                     "std::initializer_list", "std::valarray" };
    static constexpr std::string_view maps[] { "std::map", "std::unordered_map", "std::flat_map" };
    if (std::ranges::contains(sequences, *cls) && !args.empty()) return derived(type, args[0]);
    if (std::ranges::contains(maps, *cls) && args.size() >= 2) return derived(type, args[1]);
    // A class's own `operator[]` (a json value's gives a json value).
    return operator_typed(type, "operator[]");
}

std::optional<std::size_t> Resolver::opening(std::size_t close) const {
    const Kind open_kind { is(close, Kind::r_paren) ? Kind::l_paren : is(close, Kind::r_brace) ? Kind::l_brace : Kind::l_square };
    const Kind close_kind { t_[close].kind };
    int depth { 0 };
    for (std::size_t k { close + 1 }; k-- > 0;) {
        if (is(k, close_kind)) ++depth;
        else if (is(k, open_kind) && --depth == 0) return k;
    }
    return std::nullopt;
}

std::optional<std::size_t> Resolver::opening_forward(std::size_t open) const {
    if (!is(open, Kind::l_paren) && !is(open, Kind::l_square) && !is(open, Kind::l_brace)) return std::nullopt;
    const Kind open_kind { t_[open].kind };
    const Kind close_kind { is(open, Kind::l_paren) ? Kind::r_paren : is(open, Kind::l_brace) ? Kind::r_brace : Kind::r_square };
    int depth { 0 };
    for (std::size_t k { open }; k < t_.size(); ++k) {
        if (is(k, open_kind)) ++depth;
        else if (is(k, close_kind) && --depth == 0) return k;
    }
    return std::nullopt;
}

std::optional<Typed> Resolver::type_named_before(std::size_t open) {
    std::size_t begin { open };
    while (begin > 0) {
        const std::size_t k { begin - 1 };
        if (is(k, Kind::greater)) {
            std::size_t less { k };
            for (int angle { 0 }; less > 0; --less) {
                if (is(less, Kind::greater)) ++angle;
                else if (is(less, Kind::greatergreater)) angle += 2;
                else if (is(less, Kind::less) && --angle == 0) break;
            }
            if (less == 0) return std::nullopt;
            begin = less;
            continue;
        }
        if ((is(k, Kind::raw_identifier) && !keyword(t_[k].spelling)) || is(k, Kind::coloncolon)) {
            begin = k;
            continue;
        }
        break;
    }
    if (begin == open) return std::nullopt;
    return Typed { type_text(std::span { t_ }.subspan(begin, open - begin)), begin, {}, false };
}

std::optional<Resolver::Lambda> Resolver::lambda_at(std::size_t k, std::size_t limit) const {
    int nesting { 0 };
    for (; k <= limit && k < t_.size(); ++k) {   // the captures' `]`
        if (is(k, Kind::l_square)) ++nesting;
        else if (is(k, Kind::r_square) && --nesting == 0) break;
    }
    if (k > limit || k >= t_.size()) return std::nullopt;
    Lambda out;
    // Its template parameters, its parameters, its specifiers, its trailing return type, its body.
    for (std::size_t j { k + 1 }; j <= limit && j < t_.size(); ++j) {
        if (j == k + 1 && is(j, Kind::less)) {
            for (int angle { 0 }; j <= limit && j < t_.size(); ++j) {
                if (is(j, Kind::less)) ++angle;
                else if (is(j, Kind::greater) && --angle == 0) break;
            }
            continue;
        }
        if (is(j, Kind::l_paren)) {
            if (const auto close = opening_forward(j)) j = *close;
            continue;
        }
        if (is(j, Kind::arrow) && out.arrow == 0) out.arrow = j;
        if (is(j, Kind::l_brace)) {
            const auto close { opening_forward(j) };
            if (!close) return std::nullopt;
            out.body = j;
            out.end = *close;
            return out;
        }
        // Not a lambda's: a structured binding's names (`auto& [a, b] = e;`, `: m`), a subscript's.
        if (is(j, Kind::semi) || is(j, Kind::equal) || is(j, Kind::colon) || is(j, Kind::r_paren) || is(j, Kind::r_brace)) return std::nullopt;
    }
    return std::nullopt;
}

std::optional<Typed> Resolver::called(std::size_t name, int depth) {
    const auto object { expression_type(name, depth + 1) };
    if (!object) return std::nullopt;
    // A lambda's closure: its trailing return type, or what its body's first `return` gives.
    if (object->text == "(lambda)" && is(object->at, Kind::l_square)) {
        const auto lambda { lambda_at(object->at, t_.size() - 1) };
        if (!lambda) return std::nullopt;
        if (lambda->arrow == 0) return first_return(lambda->body, lambda->end, depth + 1);
        const std::string returned { type_text(std::span { t_ }.subspan(lambda->arrow + 1, lambda->body - lambda->arrow - 1)) };
        if (returned.empty()) return std::nullopt;
        return Typed { returned, lambda->arrow + 1, {}, false, {}, nullptr };
    }
    // `std::function<R(A...)>`: an R.
    const auto cls { class_of(*object) };
    if (!cls || (*cls != "std::function" && *cls != "std::move_only_function" && *cls != "std::copyable_function")) return std::nullopt;
    const auto args { arguments_of(bare(unbound(*object).text)) };
    if (args.empty()) return std::nullopt;
    std::string signature { args.front() };
    int nesting { 0 };
    std::size_t open { std::string::npos };
    for (std::size_t i { 0 }; i < signature.size(); ++i) {
        if (signature[i] == '<') ++nesting;
        else if (signature[i] == '>') --nesting;
        else if (signature[i] == '(' && nesting == 0) {
            open = i;
            break;
        }
    }
    if (open == std::string::npos) return std::nullopt;
    std::string returned { signature.substr(0, open) };
    while (!returned.empty() && returned.back() == ' ') returned.pop_back();
    if (returned.empty() || returned == "void") return std::nullopt;
    return derived(unbound(*object), returned);
}

bool Resolver::splits(std::size_t begin, std::size_t end) const {
    // The last top-level `|`'s right side, or the whole: `[std::][ranges::]views::split` called.
    std::size_t from { begin };
    for (int nesting { 0 }; std::size_t k : std::views::iota(begin, end + 1)) {
        if (is(k, Kind::l_paren) || is(k, Kind::l_square) || is(k, Kind::l_brace)) ++nesting;
        else if (is(k, Kind::r_paren) || is(k, Kind::r_square) || is(k, Kind::r_brace)) --nesting;
        else if (nesting == 0 && is(k, Kind::pipe)) from = k + 1;
    }
    std::size_t k { from };
    if (word(k, "std") && is(k + 1, Kind::coloncolon)) k += 2;
    if (word(k, "ranges") && is(k + 1, Kind::coloncolon)) k += 2;
    return word(k, "views") && is(k + 1, Kind::coloncolon) && word(k + 2, "split") && is(k + 3, Kind::l_paren);
}

std::optional<Typed> Resolver::first_return(std::size_t body, std::size_t end, int depth) {
    if (depth > 8) return std::nullopt;
    for (std::size_t k { body + 1 }; k < end; ++k) {
        // Not a nested lambda's, nor a local class's member function's.
        if (introduces_lambda(k))
            if (const auto inner = lambda_at(k, end)) {
                k = inner->end;
                continue;
            }
        if (is(k, Kind::l_brace) && k >= 2 && (word(k - 2, "struct") || word(k - 2, "class") || word(k - 2, "union")))
            if (const auto close = opening_forward(k)) {
                k = *close;
                continue;
            }
        if (!word(k, "return") || is(k + 1, Kind::semi) || is(k + 1, Kind::l_brace)) continue;
        std::size_t last { k + 1 };
        for (int nesting { 0 }; last < end; ++last) {
            if (is(last, Kind::l_paren) || is(last, Kind::l_square) || is(last, Kind::l_brace)) ++nesting;
            else if (is(last, Kind::r_paren) || is(last, Kind::r_square) || is(last, Kind::r_brace)) --nesting;
            else if (nesting == 0 && is(last, Kind::semi)) break;
        }
        if (last >= end) return std::nullopt;
        return initializer_type(k + 1, last - 1, depth + 1);
    }
    return std::nullopt;
}

bool Resolver::introduces_lambda(std::size_t j) const {
    if (!is(j, Kind::l_square) || is(j + 1, Kind::l_square) || j == 0) return false;
    const std::size_t p { j - 1 };
    if (is(p, Kind::r_paren) || is(p, Kind::r_square) || is(p, Kind::l_square) || is(p, Kind::greater) || is(p, Kind::string_literal)) return false;
    return !(is(p, Kind::raw_identifier) && (!keyword(t_[p].spelling) || word(p, "operator") || word(p, "new") || word(p, "delete")));
}

std::optional<Typed> Resolver::returned_at(std::size_t k) {
    const std::int32_t e { k < enclosing_.size() ? enclosing_[k] : -1 };
    if (e < 0 || !function_kind(ds_[static_cast<std::size_t>(e)].kind)) return std::nullopt;
    const auto& f = ds_[static_cast<std::size_t>(e)];
    // The lambdas around k: the latest whose body holds k is the innermost.
    std::optional<Lambda> inner;
    for (std::size_t j { f.first_token }; j < k; ++j) {
        if (!introduces_lambda(j)) continue;
        if (const auto lambda = lambda_at(j, k); lambda && lambda->body < k && k < lambda->end) inner = lambda;
    }
    if (inner) {
        if (inner->arrow == 0) return std::nullopt;   // deduced from its body
        const std::string returned { type_text(std::span { t_ }.subspan(inner->arrow + 1, inner->body - inner->arrow - 1)) };
        if (returned.empty()) return std::nullopt;
        return Typed { returned, inner->arrow + 1, {}, false, {}, nullptr };
    }
    return typed(target_of(e), 0, true);
}

std::optional<Typed> Resolver::expression_type(std::size_t end, int depth) {
    if (depth > 8 || end >= t_.size()) return std::nullopt;
    if (word(end, "this")) {
        for (const auto& scope : chain_at(end))
            if (classes_.contains(scope) || imported_classes_.contains(scope)) return Typed { "::" + scope + " *", end, {}, false };
        return std::nullopt;
    }
    if (is(end, Kind::raw_identifier)) {
        if (!resolved_[end]) return std::nullopt;
        const auto& o = *resolved_[end];
        if (function_kind(o.kind)) return std::nullopt;   // a function's name, not called
        // A member: read in its object's specialization.
        if (const auto object = objects_.find(end); object != objects_.end()) return member_typed(o, object->second, depth + 1);
        return typed(o, depth + 1);
    }
    if (is(end, Kind::r_brace)) {
        // `T { ... }`: a T.
        const auto open { opening(end) };
        if (!open || *open == 0) return std::nullopt;
        return type_named_before(*open);
    }
    if (is(end, Kind::r_paren)) {
        const auto open { opening(end) };
        if (!open || *open == 0) return std::nullopt;
        const std::size_t callee { *open - 1 };
        if (is(callee, Kind::greater) || is(callee, Kind::greatergreater)) {
            // `f<T>(...)`: the named casts and the standard's makers say what they make; another
            // template's call is its function's. A `>>` that ends it closes its last argument's too.
            std::size_t less { callee };
            for (int angle { 0 }; less > 0; --less) {
                if (is(less, Kind::greater)) ++angle;
                else if (is(less, Kind::greatergreater)) angle += 2;
                else if (is(less, Kind::less) && --angle == 0) break;
            }
            if (less == 0 || !is(less - 1, Kind::raw_identifier)) return std::nullopt;
            const std::string_view name { t_[less - 1].spelling };
            std::vector<PpToken> written { t_.begin() + static_cast<std::ptrdiff_t>(less + 1), t_.begin() + static_cast<std::ptrdiff_t>(callee) };
            if (is(callee, Kind::greatergreater)) {
                PpToken closing { t_[callee] };
                closing.kind = Kind::greater;
                closing.spelling = ">";
                written.push_back(closing);
            }
            const std::string argument { type_text(written) };
            if (name == "static_cast" || name == "dynamic_cast" || name == "const_cast" || name == "reinterpret_cast")
                return Typed { argument, less, {}, false };
            // std::chrono's conversions: a duration's to a duration give what they convert to; a time
            // point's give a time point of its clock (not typed here).
            if (name == "duration_cast") return Typed { argument, less, {}, false };
            if (name == "floor" || name == "ceil" || name == "round") {
                const auto from { expression_type(end - 1, depth + 1) };
                const auto cls { from ? class_of(*from) : std::nullopt };
                if (cls && *cls == "std::chrono::duration") return Typed { argument, less, {}, false };
                return std::nullopt;
            }
            if (name == "make_shared" || name == "make_unique") {
                const std::string maker { name == "make_shared" ? "std::shared_ptr" : "std::unique_ptr" };
                return Typed { std::format("{}<{}>", maker, argument), less, {}, false };
            }
            if (resolved_[less - 1] && function_kind(resolved_[less - 1]->kind)) return typed(*resolved_[less - 1], depth + 1);
            // `T<A>(...)`: a construction, a T<A> (`std::optional<Manifest>(m)`).
            if (resolved_[less - 1] && (class_kind(resolved_[less - 1]->kind) || resolved_[less - 1]->kind == msa::Kind::type_alias))
                return type_named_before(*open);
            return std::nullopt;
        }
        if (is(callee, Kind::raw_identifier) && !keyword(t_[callee].spelling)) {
            if (!resolved_[callee]) return std::nullopt;
            const auto& c = *resolved_[callee];
            // `T(...)`: a T.
            if (class_kind(c.kind) || c.kind == msa::Kind::type_alias) return type_named_before(*open);
            // An object called: a lambda's closure, a std::function.
            if (c.kind == msa::Kind::variable || c.kind == msa::Kind::field || c.kind == msa::Kind::parameter) return called(callee, depth);
            if (!function_kind(c.kind)) return std::nullopt;
            const auto object = objects_.find(callee);
            auto result { call_typed(c, *open, end, object != objects_.end() ? &object->second : nullptr, depth + 1) };
            // A member function that returns its class's iterator (`find`, `begin`): an iterator of
            // the object's type, arguments and all.
            if (result && result->iterating.empty() && iterator_name(bare(result->text)) && callee >= 2 &&
                (is(callee - 1, Kind::period) || is(callee - 1, Kind::arrow))) {
                auto container { expression_type(callee - 2, depth + 1) };
                if (container && is(callee - 1, Kind::arrow)) container = pointee(*container);
                if (container) {
                    Typed iterator { result->text, container->at, container->context, container->imported, bare(container->text), container->bindings };
                    return iterator;
                }
            }
            return result;
        }
        // `(x)`: what is inside, as an initializer is typed (`(*x)`, `(p / "x")`, `(a ? b : c)`).
        if (!is(callee, Kind::raw_identifier) && !is(callee, Kind::greater) && !is(callee, Kind::r_square) && !is(callee, Kind::r_paren) && end > *open + 1)
            return initializer_type(*open + 1, end - 1, depth + 1);
        return std::nullopt;
    }
    if (is(end, Kind::r_square)) {
        const auto open { opening(end) };
        if (!open || *open == 0) return std::nullopt;
        const auto base { expression_type(*open - 1, depth + 1) };
        if (!base) return std::nullopt;
        return element(*base);
    }
    return std::nullopt;
}
Typed Resolver::derived(const Typed& from, std::string text) {
    return Typed { std::move(text), from.at, from.context, from.imported, {}, from.bindings };
}

namespace {

// A type's text without its top-level pointers (`T *const *` -> T), and how many there were.
std::string without_pointers(std::string text, std::size_t& stars, std::string (*bare)(std::string)) {
    for (;;) {
        text = bare(std::move(text));
        if (text.ends_with("*const")) text.erase(text.size() - 5);
        else if (text.ends_with("*volatile")) text.erase(text.size() - 8);
        if (!text.ends_with('*')) return text;
        text.pop_back();
        ++stars;
    }
}

std::string with_pointers(std::string text, std::size_t stars) {
    if (stars == 0) return text;
    return text + " " + std::string(stars, '*');
}

} // namespace

Typed Resolver::unbound(Typed type, int depth) {
    for (int step { 0 }; step < 8 && depth < 8; ++step) {
        std::size_t stars { 0 };
        const std::string text { without_pointers(type.text, stars, &Resolver::bare) };
        // A template parameter the type is read with: its argument.
        if (type.bindings) {
            if (const auto bound = type.bindings->by_name.find(text); bound != type.bindings->by_name.end()) {
                Typed next { bound->second };
                next.text = with_pointers(bare(next.text), stars);
                type = std::move(next);
                continue;
            }
        }
        // An alias: what it names. Not a member of a specialization (`std::vector<int>::iterator`):
        // what that is depends on the specialization's own definition.
        if (text.find('(') != std::string::npos || text.find(">::") != std::string::npos) break;
        const std::string name { class_name_of(text) };
        if (name.empty()) break;
        const auto found { named_in(name, type) };
        if (!found || found->kind != msa::Kind::type_alias) break;
        Typed aliased;
        if (found->declaration >= 0) {
            const auto& d = ds_[static_cast<std::size_t>(found->declaration)];
            aliased = Typed { type_text(syntax_, d), d.name_token, {}, false, {}, nullptr };
        } else {
            aliased = Typed { found->type, 0, scope_of(found->qualified), true, {}, nullptr };
        }
        if (aliased.text.empty()) break;
        const auto args { arguments_of(text) };
        const auto parameters { parameters_of(*found) };
        if (!parameters.empty()) {
            // An alias template: its parameters are the arguments written.
            auto bindings { std::make_shared<Bindings>() };
            bindings->owner = found->qualified;
            for (std::size_t i { 0 }; i < parameters.size() && i < args.size(); ++i) {
                if (parameters[i].pack) break;
                if (!parameters[i].name.empty()) bindings->by_name.insert_or_assign(parameters[i].name, derived(type, args[i]));
            }
            aliased.bindings = std::move(bindings);
        } else if (!args.empty()) {
            break;   // arguments to what F1 does not know as a template
        } else if (type.bindings && scope_of(found->qualified) == type.bindings->owner) {
            aliased.bindings = type.bindings;   // a member alias of the specialization: its arguments
        }
        aliased.text = with_pointers(bare(aliased.text), stars);
        type = std::move(aliased);
    }
    return type;
}

std::vector<Parameter> Resolver::parameters_of(const std::string& qualified) {
    if (const auto imported = imported_parameters_.find(qualified); imported != imported_parameters_.end()) {
        std::vector<Parameter> out;
        for (const auto& written : imported->second) out.push_back(parameter_of(written));
        return out;
    }
    if (const auto c = classes_.find(qualified); c != classes_.end()) return parameters_of(target_of(c->second));
    return {};
}

std::vector<Parameter> Resolver::parameters_of(const Target& t) {
    if (t.declaration < 0) return parameters_of(t.qualified);
    const auto& d = ds_[static_cast<std::size_t>(t.declaration)];
    std::vector<Parameter> out;
    // Its template-head: `template <...>` before its name.
    std::size_t k { d.first_token };
    while (k < d.name_token && !(word(k, "template") && is(k + 1, Kind::less))) ++k;
    if (k >= d.name_token) return out;
    std::size_t close { k + 1 };
    for (int angle { 0 }, nesting { 0 }; close < t_.size(); ++close) {
        if (is(close, Kind::l_paren) || is(close, Kind::l_square) || is(close, Kind::l_brace)) ++nesting;
        else if (is(close, Kind::r_paren) || is(close, Kind::r_square) || is(close, Kind::r_brace)) --nesting;
        else if (nesting == 0 && is(close, Kind::less)) ++angle;
        else if (nesting == 0 && is(close, Kind::greater) && --angle == 0) break;
        else if (nesting == 0 && is(close, Kind::greatergreater) && (angle -= 2) <= 0) break;
    }
    if (close >= t_.size()) return out;
    // Each parameter: its tokens between the commas at the list's own depth.
    std::size_t begin { k + 2 };
    int angle { 0 }, nesting { 0 };
    for (std::size_t j { k + 2 }; j <= close; ++j) {
        const bool end { j == close || (angle == 0 && nesting == 0 && is(j, Kind::comma)) };
        if (!end) {
            if (is(j, Kind::l_paren) || is(j, Kind::l_square) || is(j, Kind::l_brace)) ++nesting;
            else if (is(j, Kind::r_paren) || is(j, Kind::r_square) || is(j, Kind::r_brace)) --nesting;
            else if (nesting == 0 && is(j, Kind::less)) ++angle;
            else if (nesting == 0 && is(j, Kind::greater)) --angle;
            else if (nesting == 0 && is(j, Kind::greatergreater)) angle -= 2;
            continue;
        }
        if (j > begin) {
            Parameter p;
            std::size_t equal { begin };
            for (int a { 0 }, n { 0 }; equal < j; ++equal) {
                if (is(equal, Kind::l_paren) || is(equal, Kind::l_square) || is(equal, Kind::l_brace)) ++n;
                else if (is(equal, Kind::r_paren) || is(equal, Kind::r_square) || is(equal, Kind::r_brace)) --n;
                else if (n == 0 && is(equal, Kind::less)) ++a;
                else if (n == 0 && is(equal, Kind::greater)) --a;
                else if (n == 0 && a == 0 && is(equal, Kind::equal)) break;
            }
            // Its name: the identifier before the default (or the end), after `...` for a pack.
            std::size_t name_at { equal };
            if (name_at > begin && is(name_at - 1, Kind::raw_identifier) && !keyword(t_[name_at - 1].spelling)) {
                --name_at;
                p.name = std::string { t_[name_at].spelling };
            }
            p.pack = name_at > begin && is(name_at - 1, Kind::ellipsis);
            const std::size_t head_end { p.pack ? name_at - 1 : name_at };
            if (word(begin, "template")) {
                p.sort = Parameter::Sort::template_;
            } else if ((word(begin, "class") || word(begin, "typename")) && head_end == begin + 1) {
                p.sort = Parameter::Sort::type;
            } else {
                // `Concept T` is a type parameter; `int N`, `auto N`, `std::size_t N` are values.
                std::string head;
                for (std::size_t h { begin }; h < head_end; ++h) head += t_[h].spelling;
                const auto named { head.empty() ? std::nullopt : named_in(head, Typed { {}, begin, {}, false, {}, nullptr }) };
                p.sort = named && named->kind == msa::Kind::concept_ ? Parameter::Sort::type : Parameter::Sort::value;
                if (p.sort == Parameter::Sort::value) p.value_type = type_text(std::span { t_ }.subspan(begin, head_end - begin));
            }
            if (p.sort == Parameter::Sort::type && equal < j) p.default_type = type_text(std::span { t_ }.subspan(equal + 1, j - equal - 1));
            out.push_back(std::move(p));
        }
        begin = j + 1;
    }
    return out;
}

std::shared_ptr<const Bindings> Resolver::bindings_for(const Typed& written, const std::string& owner) {
    const Typed type { unbound(written) };
    const auto cls { class_of(type) };
    if (!cls || *cls != owner) return nullptr;
    const auto parameters { parameters_of(owner) };
    if (parameters.empty()) return nullptr;
    const auto args { arguments_of(bare(type.text)) };
    auto bindings { std::make_shared<Bindings>() };
    bindings->owner = owner;
    bindings->specialization = type;
    // Where a default is read: an imported template's are written fully qualified; the file's own
    // where its template is.
    const auto own = classes_.find(owner);
    for (std::size_t i { 0 }; i < parameters.size(); ++i) {
        const auto& p = parameters[i];
        if (p.pack) break;
        if (p.name.empty()) continue;
        if (i < args.size()) {
            bindings->by_name.insert_or_assign(p.name, derived(type, args[i]));
            continue;
        }
        if (p.sort != Parameter::Sort::type || p.default_type.empty()) continue;
        // A default names the parameters before it: read with those bound.
        auto earlier { std::make_shared<Bindings>(*bindings) };
        Typed fallback { p.default_type, 0, owner, true, {}, std::move(earlier) };
        if (own != classes_.end() && !imported_parameters_.contains(owner)) {
            fallback.at = ds_[static_cast<std::size_t>(own->second)].name_token;
            fallback.context.clear();
            fallback.imported = false;
        }
        bindings->by_name.insert_or_assign(p.name, std::move(fallback));
    }
    return bindings;
}

std::optional<Typed> Resolver::member_typed(const Target& member, const Typed& object, int depth, bool chosen) {
    auto result { typed(member, depth + 1, chosen) };
    if (!result || result->bindings) return result;
    if (auto bindings = bindings_for(object, scope_of(member.qualified))) result->bindings = std::move(bindings);
    return result;
}

std::optional<Typed> Resolver::operator_typed(const Typed& written, std::string_view name) {
    const Typed type { unbound(written) };
    const auto cls { class_of(type) };
    if (!cls) return std::nullopt;
    const auto op { in_scope(*cls, std::string { name }, t_.size()) };
    if (!op || !function_kind(op->kind)) return std::nullopt;
    return member_typed(*op, type, 0);
}

} // namespace mcxx::frontend::resolution
