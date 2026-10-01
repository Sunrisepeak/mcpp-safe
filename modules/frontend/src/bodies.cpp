// The full parse's infrastructure: the cursor, the tree's arenas, tentative parsing, what the parse's own
// scopes know, the host's questions and how they are recorded -- and the driver that reads each part.
module mcxx.frontend;

import std;
import mcxx.msa;
import mcxx.base;
import :bodyparser;
import :resolver;

namespace mcxx::frontend {

namespace bodies {

namespace {

// The words that are not identifiers. Contextual ones (final, override, import, module, pre, post) are.
constexpr std::string_view RESERVED[] {
    "alignas", "alignof", "and", "and_eq", "asm", "auto", "bitand", "bitor", "bool", "break", "case", "catch", "char", "char8_t", "char16_t",
    "char32_t", "class", "compl", "concept", "const", "consteval", "constexpr", "constinit", "const_cast", "continue", "co_await", "co_return",
    "co_yield", "decltype", "default", "delete", "do", "double", "dynamic_cast", "else", "enum", "explicit", "export", "extern", "false",
    "float", "for", "friend", "goto", "if", "inline", "int", "long", "mutable", "namespace", "new", "noexcept", "not", "not_eq", "nullptr",
    "operator", "or", "or_eq", "private", "protected", "public", "register", "reinterpret_cast", "requires", "return", "short", "signed",
    "sizeof", "static", "static_assert", "static_cast", "struct", "switch", "template", "this", "thread_local", "throw", "true", "try",
    "typedef", "typeid", "typename", "union", "unsigned", "using", "virtual", "void", "volatile", "wchar_t", "while", "xor", "xor_eq",
    "__attribute__", "__declspec", "__extension__", "__int128", "__restrict", "__restrict__", "__typeof__", "__typeof", "__asm__", "__asm",
    "__volatile__", "__inline", "__inline__", "_Alignas", "_Bool", "_Noreturn", "_Thread_local", "__thread", "_Atomic", "_Complex",
    "__signed", "__signed__", "__unsigned", "__int128_t", "__uint128_t", "__null",
};

const std::unordered_set<std::string_view>& reserved_words() {
    static const std::unordered_set<std::string_view> words { std::begin(RESERVED), std::end(RESERVED) };
    return words;
}

// C++'s alternative tokens, as the punctuator they are.
Kind alternative(std::string_view w) {
    if (w.size() < 2 || w.size() > 6) return Kind::raw_identifier;
    switch (w[0]) {
    case 'a':
        if (w == "and") return Kind::ampamp;
        if (w == "and_eq") return Kind::ampequal;
        break;
    case 'b':
        if (w == "bitand") return Kind::amp;
        if (w == "bitor") return Kind::pipe;
        break;
    case 'c':
        if (w == "compl") return Kind::tilde;
        break;
    case 'n':
        if (w == "not") return Kind::exclaim;
        if (w == "not_eq") return Kind::exclaimequal;
        break;
    case 'o':
        if (w == "or") return Kind::pipepipe;
        if (w == "or_eq") return Kind::pipeequal;
        break;
    case 'x':
        if (w == "xor") return Kind::caret;
        if (w == "xor_eq") return Kind::caretequal;
        break;
    default: break;
    }
    return Kind::raw_identifier;
}

} // namespace

BodyParser::BodyParser(const Syntax& syntax, Tree& tree, NameOracle& oracle, std::span<const std::shared_ptr<const SyntaxExtension>> extensions)
    : s_ { syntax }, tree_ { tree }, oracle_ { oracle }, extensions_ { extensions }, t_ { syntax.pp.tokens } {}

// ---- cursor ----

Kind BodyParser::kind(std::size_t ahead) const {
    if (ahead == 0 && split_at_ == i_) return split_;
    if (i_ + ahead >= end_) return Kind::unknown;
    const Kind k { t_[i_ + ahead].kind };
    return k == Kind::raw_identifier ? alternative(t_[i_ + ahead].spelling) : k;
}

bool BodyParser::ident(std::size_t ahead) const {
    if (eof(ahead) || t_[i_ + ahead].kind != Kind::raw_identifier) return false;
    const std::string_view w { t_[i_ + ahead].spelling };
    return !reserved_words().contains(w);
}

bool BodyParser::any_word(std::span<const std::string_view> words, std::size_t ahead) const {
    return !eof(ahead) && t_[i_ + ahead].kind == Kind::raw_identifier && std::ranges::contains(words, t_[i_ + ahead].spelling);
}

void BodyParser::next() {
    if (split_at_ == i_) split_at_ = NPOS;
    if (i_ < end_) ++i_;
}

bool BodyParser::at_gt() const {
    const Kind k { kind() };
    return k == Kind::greater || k == Kind::greatergreater || k == Kind::greaterequal || k == Kind::greatergreaterequal;
}

bool BodyParser::expect_gt() {
    switch (kind()) {
    case Kind::greater: next(); return true;
    case Kind::greatergreater: split_at_ = i_; split_ = Kind::greater; return true;
    case Kind::greaterequal: split_at_ = i_; split_ = Kind::equal; return true;
    case Kind::greatergreaterequal: split_at_ = i_; split_ = Kind::greaterequal; return true;
    default: fail("expected `>`"); return false;
    }
}

bool BodyParser::adjacent(std::size_t ahead) const {
    if (ahead == 0 || i_ + ahead >= end_) return false;
    const auto& a = t_[i_ + ahead - 1];
    const auto& b = t_[i_ + ahead];
    return !a.expanded && !b.expanded && a.at.end == b.at.begin;
}

bool BodyParser::accept(Kind k) {
    if (!is(k)) return false;
    next();
    return true;
}

bool BodyParser::accept_word(std::string_view w) {
    if (!word(w)) return false;
    next();
    return true;
}

bool BodyParser::expect(Kind k, std::string_view what) {
    if (is(k)) {
        next();
        return true;
    }
    fail(std::format("expected {}", what));
    return false;
}

void BodyParser::fail(std::string message) {
    if (failed_) return;
    failed_ = true;
    fail_at_ = i_;
    fail_message_ = std::move(message);
}

// The bracket pair opened at k: past its closer, within the part; the part's end when broken.
std::size_t BodyParser::balanced(std::size_t k) const {
    int depth { 0 };
    for (; k < end_; ++k) {
        const Kind kind { t_[k].kind };
        if (kind == Kind::l_paren || kind == Kind::l_square || kind == Kind::l_brace) ++depth;
        else if (kind == Kind::r_paren || kind == Kind::r_square || kind == Kind::r_brace) {
            if (--depth <= 0) return k + 1;
        }
    }
    return k;
}

void BodyParser::skip_balanced() {
    i_ = balanced(i_);
    split_at_ = NPOS;
}

// ---- nodes ----

ExprId BodyParser::add(Expr e) {
    if (e.last < e.first) e.last = e.first;
    tree_.expressions.push_back(e);
    return { static_cast<std::uint32_t>(tree_.expressions.size() - 1) };
}
StmtId BodyParser::add(Stmt s) {
    if (s.last < s.first) s.last = s.first;
    tree_.statements.push_back(s);
    return { static_cast<std::uint32_t>(tree_.statements.size() - 1) };
}
TypeId BodyParser::add(TypeNode n) {
    if (n.last < n.first && n.kind != TypeKind::function) n.last = n.first;
    tree_.types.push_back(n);
    return { static_cast<std::uint32_t>(tree_.types.size() - 1) };
}
LocalId BodyParser::add(Local l) {
    if (l.last < l.first) l.last = l.first;
    tree_.locals.push_back(l);
    return { static_cast<std::uint32_t>(tree_.locals.size() - 1) };
}
NameId BodyParser::add(Name n) {
    if (n.last < n.first) n.last = n.first;
    tree_.names.push_back(n);
    return { static_cast<std::uint32_t>(tree_.names.size() - 1) };
}

ExprId BodyParser::error_expr(std::uint32_t first) {
    Expr e;
    e.kind = ExprKind::error;
    e.first = first;
    e.last = std::max(first, prev());
    return add(e);
}
StmtId BodyParser::error_stmt(std::uint32_t first, std::uint32_t last) {
    Stmt s;
    s.kind = StmtKind::error;
    s.first = first;
    s.last = std::max(first, last);
    return add(s);
}
TypeId BodyParser::error_type(std::uint32_t first) {
    TypeNode n;
    n.kind = TypeKind::error;
    n.first = first;
    n.last = std::max(first, prev());
    return add(n);
}

Span32 BodyParser::emit(const std::vector<Handle>& handles) {
    Span32 s { static_cast<std::uint32_t>(tree_.links.size()), static_cast<std::uint32_t>(handles.size()) };
    tree_.links.insert(tree_.links.end(), handles.begin(), handles.end());
    return s;
}
Span32 BodyParser::list(std::span<const Handle> handles) {
    Span32 s { static_cast<std::uint32_t>(tree_.links.size()), static_cast<std::uint32_t>(handles.size()) };
    tree_.links.insert(tree_.links.end(), handles.begin(), handles.end());
    return s;
}
std::uint32_t BodyParser::contract(Contract c) {
    tree_.contracts.push_back(c);
    return static_cast<std::uint32_t>(tree_.contracts.size() - 1);
}

ExprId BodyParser::make_expr(ExprKind k, std::uint32_t first, ExprId a, ExprId b, ExprId c) {
    Expr e;
    e.kind = k;
    e.first = first;
    e.last = prev();
    e.a = a;
    e.b = b;
    e.c = c;
    return add(e);
}

std::string BodyParser::name_text(NameId n) const {
    if (!n) return {};
    const Name& name { tree_.names[n.index] };
    std::string out { name.global ? "::" : "" };
    bool first { true };
    for (const auto& c : tree_.parts(name)) {
        if (!first) out += "::";
        first = false;
        switch (c.form) {
        case NameComponent::Form::identifier: out += s_.pp.tokens[c.token].spelling; break;
        case NameComponent::Form::destructor: out += "~" + std::string { s_.pp.tokens[c.token].spelling }; break;
        case NameComponent::Form::operator_function: out += "operator" + std::string { spelling(c.op) }; break;
        default: out += "?"; break;
        }
    }
    return out;
}

// ---- tentative parsing ----

BodyParser::Mark BodyParser::mark() const {
    return { i_, split_at_, split_, tree_.expressions.size(), tree_.statements.size(), tree_.types.size(), tree_.locals.size(), tree_.names.size(),
             tree_.components.size(), tree_.links.size(), tree_.news.size(), tree_.lambdas.size(), tree_.requires_infos.size(),
             tree_.requirements.size(), tree_.folds.size(), tree_.designators.size(), tree_.bases.size(), tree_.contracts.size(),
             tree_.attributes.size(), tree_.diagnostics.size(), tree_.decisions.size(), scope_.size(), scope_marks_.size(), no_gt_, fold_ok_, part_failed_ };
}

void BodyParser::rewind(const Mark& m) {
    i_ = m.i;
    split_at_ = m.split_at;
    split_ = m.split;
    tree_.expressions.resize(m.exprs);
    tree_.statements.resize(m.stmts);
    tree_.types.resize(m.types);
    tree_.locals.resize(m.locals);
    tree_.names.resize(m.names);
    tree_.components.resize(m.components);
    tree_.links.resize(m.links);
    tree_.news.resize(m.news);
    tree_.lambdas.resize(m.lambdas);
    tree_.requires_infos.resize(m.requires_infos);
    tree_.requirements.resize(m.requirements);
    tree_.folds.resize(m.folds);
    tree_.designators.resize(m.designators);
    tree_.bases.resize(m.bases);
    tree_.contracts.resize(m.contracts);
    tree_.attributes.resize(m.attributes);
    tree_.diagnostics.resize(m.diagnostics);
    tree_.decisions.resize(m.decisions);
    scope_.resize(m.scopes);
    scope_marks_.resize(m.marks);
    no_gt_ = m.no_gt;
    fold_ok_ = m.fold_ok;
    part_failed_ = m.part_failed;
}

void BodyParser::diagnostic(std::string message, std::size_t at) {
    const auto& t = t_.empty() ? PpToken {} : t_[std::min(at, t_.size() - 1)];
    if (tracing_) base::trace::debug(TRACE, "{}:{} cannot follow: {}", t.at.line, t.at.column, message);
    tree_.diagnostics.push_back({ Diagnostic::Severity::error, std::move(message), t.at });
}

void BodyParser::report_failure() {
    part_failed_ = true;
    diagnostic(fail_message_.empty() ? "cannot read this" : fail_message_, std::min(fail_at_, end_ > 0 ? end_ - 1 : 0));
}

// To past the `;` that ends the statement the cursor is in, or before the `}` that closes its block.
void BodyParser::recover_to_statement_end() {
    split_at_ = NPOS;
    while (i_ < end_) {
        const Kind k { t_[i_].kind };
        if (k == Kind::semi) {
            ++i_;
            return;
        }
        if (k == Kind::r_brace) return;
        if (k == Kind::l_brace) {   // a block ends the statement it was in
            i_ = std::max(i_ + 1, balanced(i_));
            return;
        }
        if (k == Kind::l_paren || k == Kind::l_square) {
            i_ = std::max(i_ + 1, balanced(i_));
            continue;
        }
        ++i_;
    }
}

// ---- what a name is ----

void BodyParser::note(Ambiguity what, Resolution chose, ast::Basis basis, std::size_t at) {
    tree_.decisions.push_back({ static_cast<std::uint32_t>(at), what, chose, basis });
}

const Entry* BodyParser::scope_entry(std::string_view name) const {
    for (auto it = scope_.rbegin(); it != scope_.rend(); ++it)
        if (it->name == name) return &*it;
    return nullptr;
}

void BodyParser::declare_name(std::size_t token, NameClass what, bool dependent) {
    if (token < t_.size()) scope_.push_back({ t_[token].spelling, what, dependent });
}

void BodyParser::declare_entry(std::string_view name, NameClass what, bool dependent) {
    if (tracing_) base::trace::debug(TRACE, "a part sees {} `{}`{}", what == NameClass::value ? "a value" : what == NameClass::type ? "a type" : "a template", name, dependent ? " (dependent)" : "");
    scope_.push_back({ name, what, dependent });
}
void BodyParser::declare(std::uint32_t name_token, NameClass what) { declare_name(name_token, what); }
void BodyParser::pop_scope() {
    if (scope_marks_.empty()) return;
    scope_.resize(scope_marks_.back());
    scope_marks_.pop_back();
}

NameInfo BodyParser::classify(std::string_view written, std::size_t at, bool) {
    NameInfo info;
    const auto colons { written.find("::") };
    const std::string_view first { colons == std::string_view::npos ? written : written.substr(0, colons) };
    if (!first.empty()) {
        if (const Entry* e { scope_entry(first) }) {
            // A dependent name's qualified members are not known until instantiation: nothing is told of them.
            if (colons != std::string_view::npos) {
                if (e->dependent) {
                    info.dependent = true;
                    info.basis = ast::Basis::scope;
                    return info;
                }
            } else {
                info.what = e->what;
                info.basis = ast::Basis::scope;
                info.dependent = e->dependent;
                return info;
            }
        }
    }
    const NameAnswer a { oracle_.classify({ written, static_cast<std::uint32_t>(at) }) };
    if (tracing_ && a.what != NameClass::unknown) {
        const auto& where = t_[std::min(at, t_.size() - 1)].at;
        base::trace::debug(TRACE, "{}:{} the oracle says `{}` is {}", where.line, where.column, written, static_cast<int>(a.what));
    }
    if (a.what != NameClass::unknown) {
        info.what = a.what;
        info.basis = a.basis;
    }
    return info;
}

NameInfo BodyParser::classify_name(NameId n, std::size_t at, bool record) { return classify(name_text(n), at, record); }

// ---- parts ----

void BodyParser::begin_part(std::int32_t declaration, std::size_t from, std::size_t to, bool open_ended) {
    owner_ = declaration;
    i_ = from;
    begin_ = from;
    guess_ = open_ended ? std::min(to, t_.size()) : 0;
    end_ = open_ended ? t_.size() : std::min(to, t_.size());
    split_at_ = NPOS;
    failed_ = false;
    part_failed_ = false;
    speculating_ = 0;
    no_gt_ = 0;
    fold_ok_ = false;
    depth_ = 0;
    scope_.clear();   // what the last part declared is not visible in this one
    scope_marks_.clear();
    class_names_.clear();
}

void BodyParser::finish_part() {
    if (failed_) {
        report_failure();
        failed_ = false;
    }
    // An open-ended part ends where the grammar does; what the outline guessed shorter was left unread.
    if (guess_ != 0 ? i_ < guess_ : i_ < end_) {
        part_failed_ = true;
        diagnostic(std::format("unexpected `{}` after the construct", t_[i_].spelling), i_);
        i_ = guess_ != 0 ? guess_ : end_;
    }
}

// ---- what a part holds ----

StmtId BodyParser::read_function_body() { return function_body(LocalId {}); }

StmtId BodyParser::read_static_assert() {
    const std::uint32_t first { here() };
    const LocalId l { static_assert_declaration() };
    Stmt s;
    s.kind = StmtKind::static_assert_;
    s.first = first;
    s.last = prev();
    if (!failed_) {
        s.e1 = tree_.locals[l.index].init;
        s.e2 = tree_.locals[l.index].width;
    }
    return add(s);
}

// `= e`, `{ ... }`, `( ... )`: the initializer of a variable, a field.
ExprId BodyParser::read_initializer() {
    const std::uint32_t first { here() };
    if (accept(Kind::equal)) return initializer_clause();
    if (is(Kind::l_brace)) return braced_init_list();
    if (is(Kind::l_paren)) {
        next();
        Expr list;
        list.kind = ExprKind::init_list;
        list.first = first;
        list.list = argument_list(Kind::r_paren);
        list.last = prev();
        return add(list);
    }
    fail("expected an initializer");
    return error_expr(first);
}

ExprId BodyParser::read_expression_part(Part::Role role) {
    switch (role) {
    case Part::Role::default_argument: return initializer_clause();
    case Part::Role::constraint:
        if (accept_word("requires")) return constraint_expression();
        return constraint_expression();
    case Part::Role::concept_definition: return expression();
    default: return conditional_expression();
    }
}

// `noexcept ( expr )`: the condition.
ExprId BodyParser::read_noexcept_part() {
    const std::uint32_t first { here() };
    next();
    if (!expect(Kind::l_paren, "`(`")) return error_expr(first);
    const ExprId e { expression() };
    expect(Kind::r_paren, "`)`");
    return e;
}

// `pre ( cond )`, `post ( [r:] cond )`: read by the extension that has them; the root is the condition.
ExprId BodyParser::read_contract_part() {
    const std::uint32_t first { here() };
    FunctionSpecifiers specs;
    for (const auto& ext : extensions_)
        if (ext->function_specifier(*this, specs) && !specs.contracts.empty()) return tree_.contracts[specs.contracts.front()].condition;
    if (!failed_) fail("a contract specifier needs the C++26 syntax extension");
    return error_expr(first);
}

} // namespace bodies

using namespace bodies;

namespace {

// The kind a template parameter has, from the tokens before its name (the outline records no more): `class T` and
// `typename T` are types, `template <...> class T` a template, a name before the parameter's is the concept that
// constrains a type (`std::integral T`, `C<U> T`) or the type of a value (`std::size_t N`): the concept when the
// oracle says so, or when it has arguments (a value of a class type is rare), else a value.
NameClass template_parameter_class(const Syntax& syntax, const Declaration& d, NameOracle& oracle) {
    const auto& t = syntax.pp.tokens;
    std::size_t name { d.name_token };
    if (name >= 1 && t[name - 1].kind == Kind::ellipsis) --name;
    // Back to the start of this parameter: the `<` that opens the list, or the `,` before it, at its own depth.
    std::size_t start { name };
    int depth { 0 };
    while (start > 0) {
        const Kind k { t[start - 1].kind };
        if (k == Kind::greater) ++depth;
        else if (k == Kind::greatergreater) depth += 2;
        else if (k == Kind::less) {
            if (depth == 0) break;
            --depth;
        } else if (k == Kind::comma && depth == 0) break;
        else if (k == Kind::r_paren || k == Kind::r_square) {
            int nested { 0 };
            while (start > 0) {
                const Kind c { t[start - 1].kind };
                if (c == Kind::r_paren || c == Kind::r_square) ++nested;
                else if ((c == Kind::l_paren || c == Kind::l_square) && --nested == 0) break;
                --start;
            }
        }
        --start;
    }
    if (start >= name) return NameClass::value;
    const std::string_view first { t[start].spelling };
    if (first == "template") return NameClass::class_template;
    if (first == "class" || first == "typename") return NameClass::type;
    static constexpr std::string_view VALUE_WORDS[] { "auto", "decltype", "const", "volatile", "signed", "unsigned", "int", "long", "short", "char", "bool",
                                                      "float", "double", "void", "wchar_t", "char8_t", "char16_t", "char32_t", "constexpr" };
    if (std::ranges::contains(VALUE_WORDS, first)) return NameClass::value;
    // A (qualified) name, perhaps with arguments: a constraint when it is a concept, or has arguments.
    std::string text;
    bool arguments { false };
    for (std::size_t k { start }; k < name; ++k) {
        if (t[k].kind == Kind::less) {
            arguments = true;
            break;
        }
        if (t[k].kind == Kind::raw_identifier || t[k].kind == Kind::coloncolon) text += t[k].spelling;
    }
    if (!text.empty() && oracle.classify({ text, static_cast<std::uint32_t>(start) }).what == NameClass::concept_) return NameClass::type;
    return arguments ? NameClass::type : NameClass::value;
}

class Driver {
public:
    Driver(const Syntax& syntax, const BodyOptions& options, Tree& tree) : syntax_ { syntax }, options_ { options }, tree_ { tree } {}

    void run() {
        const Imported none;
        std::unique_ptr<NameOracle> own;
        NameOracle* oracle { options_.oracle };
        if (oracle == nullptr) {
            own = lookup_oracle(syntax_, options_.imported != nullptr ? *options_.imported : none);
            oracle = own.get();
        }
        BodyParser parser { syntax_, tree_, *oracle, options_.extensions };
        const auto& ds = syntax_.declarations;
        // Where the outline has the declaration a full-parse declaration names: by the name's token.
        std::unordered_map<std::uint32_t, std::int32_t> outline;
        outline.reserve(ds.size());
        for (std::size_t i { 0 }; i < ds.size(); ++i)
            if (!ds[i].name.empty()) outline.try_emplace(ds[i].name_token, static_cast<std::int32_t>(i));
        parser.set_outline(&outline);
        std::vector<std::vector<std::int32_t>> parameters(ds.size() + 1);
        std::vector<std::int32_t> template_parameters;
        for (std::size_t i { 0 }; i < ds.size(); ++i) {
            if (ds[i].kind == msa::Kind::parameter && ds[i].parent >= 0 && !ds[i].in_lambda) parameters[static_cast<std::size_t>(ds[i].parent)].push_back(static_cast<std::int32_t>(i));
            if (ds[i].kind == msa::Kind::template_parameter && !ds[i].in_lambda) template_parameters.push_back(static_cast<std::int32_t>(i));
        }
        std::vector<Part> parts { syntax_.parts };
        std::ranges::stable_sort(parts, {}, &Part::begin);
        std::uint32_t covered { 0 };
        for (const auto& part : parts) {
            // What a body holds that the outline read as a part of its own (a local class's method) was read with it.
            if (part.begin < covered) continue;
            if (options_.roots_only_bodies && part.role != Part::Role::function_body) continue;
            const bool open_ended { part.role == Part::Role::initializer || part.role == Part::Role::default_argument ||
                                    part.role == Part::Role::enumerator_value || part.role == Part::Role::bit_width };
            covered = part.end;
            parser.begin_part(part.declaration, part.begin, part.end, open_ended);
            // What is visible in it: the template parameters in scope, and the function's parameters.
            for (const auto p : template_parameters) {
                const auto& d = ds[static_cast<std::size_t>(p)];
                if (d.name_token >= part.begin) continue;
                // Visible to the end of its template; when the outline did not say where that is, within the scope it was declared in.
                bool visible { d.visible_end != 0 && part.begin <= d.visible_end };
                if (d.visible_end == 0) {
                    const std::int32_t scope { d.parent };
                    visible = scope < 0 || (part.begin >= ds[static_cast<std::size_t>(scope)].first_token && part.begin <= ds[static_cast<std::size_t>(scope)].last_token);
                }
                if (visible) parser.declare_entry(d.name, template_parameter_class(syntax_, d, *oracle), true);
            }
            if (part.declaration >= 0)
                for (const auto p : parameters[static_cast<std::size_t>(part.declaration)]) {
                    const auto& d = ds[static_cast<std::size_t>(p)];
                    // The outline reads a lone type (`void f(index_sequence<Is...>)`) as a parameter's name: no type written, no name.
                    if (!d.name.empty() && d.specifiers_end > d.specifiers_begin) parser.declare_entry(d.name, NameClass::value);
                }
            Root root;
            root.declaration = part.declaration;
            root.first = part.begin;
            root.last = part.end - 1;
            switch (part.role) {
            case Part::Role::function_body: root.role = Root::Role::body; root.node = parser.read_function_body().handle(); break;
            case Part::Role::static_assert_: root.role = Root::Role::static_assert_; root.node = parser.read_static_assert().handle(); break;
            case Part::Role::initializer: root.role = Root::Role::initializer; root.node = parser.read_initializer().handle(); break;
            case Part::Role::default_argument: root.role = Root::Role::default_argument; root.node = parser.read_expression_part(part.role).handle(); break;
            case Part::Role::enumerator_value: root.role = Root::Role::enumerator_value; root.node = parser.read_expression_part(part.role).handle(); break;
            case Part::Role::bit_width: root.role = Root::Role::bit_width; root.node = parser.read_expression_part(part.role).handle(); break;
            case Part::Role::constraint: root.role = Root::Role::constraint; root.node = parser.read_expression_part(part.role).handle(); break;
            case Part::Role::noexcept_spec: root.role = Root::Role::noexcept_spec; root.node = parser.read_noexcept_part().handle(); break;
            case Part::Role::concept_definition: root.role = Root::Role::concept_definition; root.node = parser.read_expression_part(part.role).handle(); break;
            case Part::Role::contract: root.role = Root::Role::contract; root.node = parser.read_contract_part().handle(); break;
            }
            parser.finish_part();
            if (open_ended) {
                // Where the grammar ended, which the outline's guess may have cut short (or run past).
                root.last = static_cast<std::uint32_t>(std::max<std::size_t>(parser.position_after(), root.first + 1) - 1);
                covered = std::max<std::uint32_t>(covered, root.last + 1);
            }
            ++tree_.stats.parts;
            if (parser.failed_part()) ++tree_.stats.failed;
            tree_.roots.push_back(root);
        }
        for (const auto& e : tree_.expressions) tree_.stats.errors += e.kind == ExprKind::error;
        for (const auto& s : tree_.statements) tree_.stats.errors += s.kind == StmtKind::error;
        for (const auto& t : tree_.types) tree_.stats.errors += t.kind == TypeKind::error;
        tree_.stats.decisions = tree_.decisions.size();
        for (const auto& d : tree_.decisions) ++tree_.stats.by_basis[static_cast<std::size_t>(d.basis)];
        if (base::trace::enabled(TRACE, base::trace::Level::debug))
            for (const auto& d : tree_.decisions) {
                const auto& t = syntax_.pp.tokens[std::min<std::size_t>(d.token, syntax_.pp.tokens.size() - 1)];
                base::trace::debug(TRACE, "{}:{} {}: {} (by {})", t.at.line, t.at.column, ast::to_string(d.what), ast::to_string(d.chose), ast::to_string(d.basis));
            }
        if (!options_.record_decisions) tree_.decisions.clear();
    }

private:
    const Syntax& syntax_;
    const BodyOptions& options_;
    Tree& tree_;
};

} // namespace

ast::Tree parse_bodies(const Syntax& syntax, const BodyOptions& options) {
    const base::trace::Span span { TRACE, "bodies", std::format("{} parts", syntax.parts.size()) };
    ast::Tree tree;
    tree.syntax = &syntax;
    Driver { syntax, options, tree }.run();
    if (base::trace::enabled(TRACE, base::trace::Level::debug))
        base::trace::debug(TRACE, "bodies: {} parts, {} failed, {} errors, {} decisions (scope {}, lookup {}, feedback {}, shape {}), {} speculations, {} rewound",
                           tree.stats.parts, tree.stats.failed, tree.stats.errors, tree.stats.decisions,
                           tree.stats.by_basis[static_cast<std::size_t>(ast::Basis::scope)], tree.stats.by_basis[static_cast<std::size_t>(ast::Basis::lookup)],
                           tree.stats.by_basis[static_cast<std::size_t>(ast::Basis::feedback)], tree.stats.by_basis[static_cast<std::size_t>(ast::Basis::shape)],
                           tree.stats.speculations, tree.stats.rewinds);
    return tree;
}

Fragment parse_fragment(std::string_view text, const BodyOptions& options) {
    Fragment out;
    const std::string wrapped { "void mcxx_fragment() { " + std::string { text } + "\n}\n" };
    // The syntax keeps views into the text it was parsed from: the fragment keeps both.
    auto storage { std::make_shared<std::pair<std::string, Syntax>>() };
    storage->first = wrapped;
    storage->second = parse(storage->first);
    out.owned = std::shared_ptr<Syntax> { storage, &storage->second };
    out.tree = parse_bodies(*out.owned, options);
    return out;
}

} // namespace mcxx::frontend
