// Names and template arguments: [::] (component ::)* component, and where a `<` opens an argument list.
module mcxx.frontend;

import std;
import mcxx.msa;
import mcxx.base;
import :bodyparser;

namespace mcxx::frontend::bodies {

namespace {

// Tokens that cannot begin an operand: after `a < b >` any of them means the `<...>` was a template-argument list,
// since a comparison `(a < b) > x` goes on with an operand.
bool cannot_start_operand(Kind k) {
    switch (k) {
    case Kind::raw_identifier: case Kind::numeric_constant: case Kind::char_constant: case Kind::wide_char_constant: case Kind::utf8_char_constant:
    case Kind::utf16_char_constant: case Kind::utf32_char_constant: case Kind::string_literal: case Kind::wide_string_literal: case Kind::utf8_string_literal:
    case Kind::utf16_string_literal: case Kind::utf32_string_literal: case Kind::l_square: case Kind::plus: case Kind::minus: case Kind::star: case Kind::amp:
    case Kind::exclaim: case Kind::tilde: case Kind::plusplus: case Kind::minusminus: case Kind::coloncolon: case Kind::caret:
        return false;
    default: return true;
    }
}

Resolution resolution_of(NameClass c) {
    switch (c) {
    case NameClass::type: return Resolution::type;
    case NameClass::class_template: return Resolution::class_template;
    case NameClass::alias_template: return Resolution::alias_template;
    case NameClass::function_template: return Resolution::function_template;
    case NameClass::variable_template: return Resolution::variable_template;
    case NameClass::concept_: return Resolution::concept_;
    case NameClass::value: return Resolution::value;
    case NameClass::namespace_: return Resolution::namespace_;
    case NameClass::unknown: return Resolution::unknown;
    }
    return Resolution::unknown;
}

} // namespace

bool BodyParser::at_splice() const { return is(Kind::l_square) && (is(Kind::colon, 1) || is(Kind::coloncolon, 1)) && adjacent(1); }

// Past the `:]` that closes the splice opened at k (tokens `[` `:` ... `:` `]`), counting what nests inside; k when it does not close.
std::size_t BodyParser::splice_end(std::size_t k) const {
    int depth { 0 };
    for (std::size_t j { k }; j < end_; ++j) {
        const Kind kd { t_[j].kind };
        if (kd == Kind::l_paren || kd == Kind::l_square || kd == Kind::l_brace) ++depth;
        else if (kd == Kind::r_paren || kd == Kind::r_brace) --depth;
        else if (kd == Kind::r_square) {
            if (--depth == 0) return j > 0 && t_[j - 1].kind == Kind::colon && t_[j - 1].at.end == t_[j].at.begin ? j + 1 : k;
        }
        if (depth <= 0 && j > k) return k;
    }
    return k;
}

bool BodyParser::starts_name() const {
    if (ident() || is(Kind::coloncolon) || is(Kind::tilde)) return true;
    if (word("operator") || word("template") || (word("decltype") && is(Kind::l_paren, 1))) return true;
    return is(Kind::l_square) && is(Kind::colon, 1) && adjacent(1);
}

// `operator` and what it names: a punctuator, new/delete (with []), (), [], a literal operator, a conversion.
bool BodyParser::operator_function_id(NameComponent& c) {
    next();   // operator
    const std::uint32_t at { here() };
    c.token = at - 1;
    c.form = NameComponent::Form::operator_function;
    const Kind k { kind() };
    const auto simple = [&](Op op) {
        c.op = op;
        next();
    };
    if (word("new") || word("delete")) {
        const bool is_new { word("new") };
        next();
        if (is(Kind::l_square) && is(Kind::r_square, 1)) {
            advance(2);
            c.op = is_new ? Op::new_array : Op::delete_array;
        } else c.op = is_new ? Op::new_ : Op::delete_;
        return true;
    }
    if (word("co_await")) {
        simple(Op::co_await_);
        return true;
    }
    if (k == Kind::l_paren && is(Kind::r_paren, 1)) {
        advance(2);
        c.op = Op::call;
        return true;
    }
    if (k == Kind::l_square && is(Kind::r_square, 1)) {
        advance(2);
        c.op = Op::subscript;
        return true;
    }
    if (is_string(tok().kind)) {
        // operator "" _x, operator ""_x
        c.form = NameComponent::Form::literal_operator;
        next();
        if (ident()) {
            c.token = here();
            next();
        } else if (tok().spelling.size() > 2 && tok().spelling.starts_with("\"\"") == false) {
            c.token = here() - 1;
        }
        return true;
    }
    static constexpr std::pair<Kind, Op> PUNCTUATORS[] {
        { Kind::plus, Op::add }, { Kind::minus, Op::subtract }, { Kind::star, Op::multiply }, { Kind::slash, Op::divide }, { Kind::percent, Op::remainder },
        { Kind::caret, Op::bit_xor }, { Kind::amp, Op::bit_and }, { Kind::pipe, Op::bit_or }, { Kind::tilde, Op::complement }, { Kind::exclaim, Op::not_ },
        { Kind::equal, Op::assign }, { Kind::less, Op::less }, { Kind::greater, Op::greater }, { Kind::plusequal, Op::add_assign },
        { Kind::minusequal, Op::subtract_assign }, { Kind::starequal, Op::multiply_assign }, { Kind::slashequal, Op::divide_assign },
        { Kind::percentequal, Op::remainder_assign }, { Kind::caretequal, Op::xor_assign }, { Kind::ampequal, Op::and_assign },
        { Kind::pipeequal, Op::or_assign }, { Kind::lessless, Op::shift_left }, { Kind::greatergreater, Op::shift_right },
        { Kind::lesslessequal, Op::shift_left_assign }, { Kind::greatergreaterequal, Op::shift_right_assign }, { Kind::equalequal, Op::equal },
        { Kind::exclaimequal, Op::not_equal }, { Kind::lessequal, Op::less_equal }, { Kind::greaterequal, Op::greater_equal },
        { Kind::spaceship, Op::spaceship }, { Kind::ampamp, Op::logical_and }, { Kind::pipepipe, Op::logical_or }, { Kind::plusplus, Op::pre_increment },
        { Kind::minusminus, Op::pre_decrement }, { Kind::comma, Op::comma }, { Kind::arrowstar, Op::member_pointer_arrow }, { Kind::arrow, Op::arrow },
    };
    for (const auto& [kind, op] : PUNCTUATORS)
        if (k == kind) {
            simple(op);
            return true;
        }
    // A conversion function: the type it converts to, up to its parameter list.
    c.form = NameComponent::Form::conversion_function;
    c.token = at - 1;
    DeclSpec spec { decl_specifiers(Site::parameter) };
    if (!spec.has_type) {
        fail("expected an operator");
        return false;
    }
    c.type = pointer_operators(spec.type);
    return true;
}

// One component of a name, the cursor at it.
bool BodyParser::name_component(NameComponent& c, NameUse use, bool first, bool after_template_kw, std::string& written, bool* dependent,
                                NameInfo* info, bool* more) {
    (void)first;
    (void)info;
    *more = false;
    c.token = here();
    if (is(Kind::l_square)) {
        for (const auto& ext : extensions_)
            if (ext->name_component(*this, c)) {
                written += "[:...:]";
                *more = is(Kind::coloncolon);
                if (dependent != nullptr) *dependent = true;
                return !failed_;
            }
    }
    if (word("decltype") && is(Kind::l_paren, 1)) {
        advance(2);
        c.form = NameComponent::Form::decltype_;
        c.expr = expression();
        expect(Kind::r_paren, "`)`");
        c.last = prev();
        *more = is(Kind::coloncolon);
        written += "decltype(...)";
        if (dependent != nullptr) *dependent = true;
        return !failed_;
    }
    if (is(Kind::tilde)) {
        next();
        c.form = NameComponent::Form::destructor;
        if (word("decltype") && is(Kind::l_paren, 1)) {
            const TypeId t { decltype_type() };
            c.type = t;
            c.token = prev();
        } else if (ident()) {
            c.token = here();
            written += "~" + std::string { spelled(c.token) };
            next();
        } else {
            fail("expected a class name after `~`");
            return false;
        }
        c.last = prev();
        return !failed_;
    }
    if (word("operator")) {
        if (!operator_function_id(c)) return false;
        c.last = prev();
        written += "operator";
        return !failed_;
    }
    if (!ident()) {
        fail("expected a name");
        return false;
    }
    c.form = NameComponent::Form::identifier;
    c.token = here();
    written += spelled(c.token);
    next();
    c.last = prev();

    // Template arguments.
    if (is(Kind::less)) {
        bool take { false };
        const std::size_t less_at { i_ };
        NameInfo prefix;
        if (after_template_kw) {
            take = true;
        } else if (use == NameUse::member) {
            prefix.what = NameClass::unknown;   // the object's class is not known here
        } else {
            prefix = classify(written, c.token);
            if (prefix.dependent && !after_template_kw && !template_like(prefix.what)) {
                // `T::f<` without `template` is a comparison.
                if (tracing_) base::trace::debug(TRACE, "dependent `{}` followed by `<`", written);
                take = false;
                if (dependent != nullptr) *dependent = true;
                note(Ambiguity::template_arguments, Resolution::less_than, ast::Basis::syntax, less_at);
                goto done;
            }
        }
        if (!take && template_like(prefix.what)) {
            take = true;
            note(Ambiguity::template_arguments, Resolution::arguments, prefix.basis, less_at);
        } else if (!take && prefix.what != NameClass::unknown) {
            note(Ambiguity::template_arguments, Resolution::less_than, prefix.basis, less_at);
        } else if (!take) {
            // Nothing knows what the name is: the code's shape. Arguments that read as such, followed by what
            // makes the comparison impossible or what a template-id is followed by.
            if (not_arguments_.contains(less_at)) {
                // decided before, at this place
            } else {
                const Mark m { mark() };
                Span32 args;
                bool good { false };
                {
                    ++speculating_;
                    ++tree_.stats.speculations;
                    const bool was_failed { std::exchange(failed_, false) };
                    good = template_arguments(args) && !failed_;
                    failed_ = was_failed;
                    --speculating_;
                }
                if (good) {
                    const Kind follower { kind() };
                    const bool typed { use == NameUse::type };
                    good = typed || follower == Kind::l_paren || follower == Kind::l_brace || follower == Kind::coloncolon || cannot_start_operand(follower);
                    // Closed by half of a `>>` or `>=` outside any argument list: the operator is the expression's, `a < b >> c`.
                    if (good && !typed && split_at_ == i_ && no_gt_ == 0) good = false;
                    if (!good) ++tree_.stats.rewinds;
                }
                if (!good) {
                    rewind(m);
                    not_arguments_.insert(less_at);
                    note(Ambiguity::template_arguments, Resolution::less_than, ast::Basis::shape, less_at);
                } else {
                    c.flags |= NameComponent::has_arguments;
                    c.arguments = args;
                    c.last = prev();
                    note(Ambiguity::template_arguments, Resolution::arguments, ast::Basis::shape, less_at);
                }
            }
            goto done;
        }
        if (take) {
            Span32 args;
            if (!template_arguments(args)) return false;
            c.flags |= NameComponent::has_arguments;
            c.arguments = args;
            c.last = prev();
        }
    }
done:
    if (c.has(NameComponent::template_keyword) && !c.has(NameComponent::has_arguments)) {
        // `T::template f` without arguments is an error, but a name nonetheless.
    }
    *more = is(Kind::coloncolon);
    if (dependent != nullptr && *dependent == false) {
        if (const Entry* e { scope_entry(spelled(c.token)) }; e != nullptr && e->dependent && written == spelled(c.token)) *dependent = true;
    }
    return !failed_;
}

NameId BodyParser::qualified_name(NameUse use) { return qualified_name(use, nullptr, nullptr); }

NameId BodyParser::qualified_name(NameUse use, bool* dependent, NameInfo* info) {
    if (failed_) return {};
    Name name;
    name.first = here();
    std::vector<NameComponent> parts;
    std::string written;
    if (is(Kind::coloncolon)) {
        name.global = true;
        written = "::";
        next();
    }
    bool after_template { false };
    for (;;) {
        NameComponent c;
        if (word("template") && (ident(1) || word("operator", 1) || is(Kind::tilde, 1))) {
            next();
            after_template = true;
            c.flags |= NameComponent::template_keyword;
        }
        bool more { false };
        if (!name_component(c, use, parts.empty(), after_template, written, dependent, info, &more)) break;
        after_template = false;
        parts.push_back(c);
        if (!more) break;
        // `::` followed by what continues a name.
        const bool continues { ident(1) || word("template", 1) || word("operator", 1) || is(Kind::tilde, 1) || (word("decltype", 1) && is(Kind::l_paren, 2)) ||
                               (is(Kind::l_square, 1) && is(Kind::colon, 2)) };
        if (!continues) break;
        next();   // ::
        written += "::";
    }
    if (failed_ || parts.empty()) return {};
    name.last = prev();
    name.components = { static_cast<std::uint32_t>(tree_.components.size()), static_cast<std::uint32_t>(parts.size()) };
    tree_.components.insert(tree_.components.end(), parts.begin(), parts.end());
    return add(name);
}

// `<` argument-list `>`, the cursor at the `<`: false (a failure) when it is not one.
bool BodyParser::template_arguments(Span32& out) {
    if (!is(Kind::less)) {
        fail("expected `<`");
        return false;
    }
    next();
    const int saved_no_gt { std::exchange(no_gt_, 1) };
    const bool saved_fold { std::exchange(fold_ok_, false) };
    std::vector<Handle> args;
    if (!at_gt()) {
        for (;;) {
            Handle h { template_argument() };
            if (failed_) break;
            if (is(Kind::ellipsis)) {
                const std::uint32_t first { tree_.tokens(h).first };
                next();
                if (h.sort == Sort::type) {
                    TypeNode pack;
                    pack.kind = TypeKind::pack_expansion;
                    pack.first = first;
                    pack.last = prev();
                    pack.inner = { h.index };
                    h = add(pack).handle();
                } else if (h.sort == Sort::expression) {
                    Expr pack;
                    pack.kind = ExprKind::pack_expansion;
                    pack.first = first;
                    pack.last = prev();
                    pack.a = { h.index };
                    h = add(pack).handle();
                }
            }
            args.push_back(h);
            if (accept(Kind::comma)) continue;
            break;
        }
    }
    no_gt_ = saved_no_gt;
    fold_ok_ = saved_fold;
    if (failed_) return false;
    if (!expect_gt()) return false;
    out = emit(args);
    return true;
}

Handle BodyParser::template_argument() { return type_or_expression(); }

} // namespace mcxx::frontend::bodies
