// Expressions, with C++'s precedence: the comma, assignment, ?:, the binary operators, casts, unary and
// postfix expressions, primaries; braced initializers, new and delete, sizeof and its kin, named casts.
// Where tokens could be read as a type or as an expression, a name decides (parse_type.cpp's
// type_or_expression, try_cast): the parse's scopes, the oracle, then the shape.
module mcxx.frontend;

import std;
import mcxx.msa;
import mcxx.base;
import :bodyparser;

namespace mcxx::frontend::bodies {

namespace {


// A pp-number's reading: a floating literal (not hexadecimal with no `p` exponent), a user-defined suffix.
struct Numeric {
    bool floating { false };
    bool user_defined { false };
};

Numeric numeric(std::string_view s) {
    Numeric n;
    const bool hex { s.size() > 1 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X') };
    const bool binary { s.size() > 1 && s[0] == '0' && (s[1] == 'b' || s[1] == 'B') };
    for (std::size_t i { 0 }; i < s.size(); ++i) {
        const char c { s[i] };
        if (c == '.') n.floating = true;
        else if (!hex && !binary && (c == 'e' || c == 'E')) n.floating = true;
        else if (hex && (c == 'p' || c == 'P')) n.floating = true;
        else if (c == '_') n.user_defined = true;
    }
    if (!n.user_defined && !hex && !binary) {
        // The standard library's literal suffixes: 10s, 5ms, 1.5h, 2i, 3il, 1if, 1y, 1d.
        std::size_t end { s.size() };
        while (end > 0 && std::isalpha(static_cast<unsigned char>(s[end - 1])) != 0) --end;
        const std::string_view suffix { s.substr(end) };
        if (end > 0 && (suffix == "s" || suffix == "h" || suffix == "min" || suffix == "ms" || suffix == "us" || suffix == "ns" || suffix == "i" ||
                        suffix == "il" || suffix == "if" || suffix == "y" || suffix == "d" || suffix == "sv"))
            n.user_defined = true;
    }
    return n;
}

bool has_suffix(std::string_view spelling, char quote) {
    const auto last { spelling.rfind(quote) };
    return last != std::string_view::npos && last + 1 < spelling.size();
}

bool starts_unary_operand(Kind k) {
    switch (k) {
    case Kind::plus: case Kind::minus: case Kind::star: case Kind::amp: case Kind::plusplus: case Kind::minusminus: case Kind::exclaim:
    case Kind::tilde: case Kind::l_paren: case Kind::l_brace: case Kind::numeric_constant: case Kind::char_constant: case Kind::wide_char_constant:
    case Kind::utf8_char_constant: case Kind::utf16_char_constant: case Kind::utf32_char_constant: case Kind::string_literal:
    case Kind::wide_string_literal: case Kind::utf8_string_literal: case Kind::utf16_string_literal: case Kind::utf32_string_literal:
    case Kind::raw_identifier: case Kind::coloncolon:
        return true;
    default: return false;
    }
}

bool fold_operator(Kind k, Op* op) {
    struct Item {
        Kind kind;
        Op op;
    };
    static constexpr Item ITEMS[] {
        { Kind::plus, Op::add }, { Kind::minus, Op::subtract }, { Kind::star, Op::multiply }, { Kind::slash, Op::divide }, { Kind::percent, Op::remainder },
        { Kind::caret, Op::bit_xor }, { Kind::amp, Op::bit_and }, { Kind::pipe, Op::bit_or }, { Kind::lessless, Op::shift_left },
        { Kind::greatergreater, Op::shift_right }, { Kind::plusequal, Op::add_assign }, { Kind::minusequal, Op::subtract_assign },
        { Kind::starequal, Op::multiply_assign }, { Kind::slashequal, Op::divide_assign }, { Kind::percentequal, Op::remainder_assign },
        { Kind::caretequal, Op::xor_assign }, { Kind::ampequal, Op::and_assign }, { Kind::pipeequal, Op::or_assign },
        { Kind::lesslessequal, Op::shift_left_assign }, { Kind::greatergreaterequal, Op::shift_right_assign }, { Kind::equal, Op::assign },
        { Kind::equalequal, Op::equal }, { Kind::exclaimequal, Op::not_equal }, { Kind::less, Op::less }, { Kind::greater, Op::greater },
        { Kind::lessequal, Op::less_equal }, { Kind::greaterequal, Op::greater_equal }, { Kind::ampamp, Op::logical_and },
        { Kind::pipepipe, Op::logical_or }, { Kind::comma, Op::comma }, { Kind::periodstar, Op::member_pointer_dot },
        { Kind::arrowstar, Op::member_pointer_arrow },
    };
    for (const auto& item : ITEMS)
        if (item.kind == k) {
            *op = item.op;
            return true;
        }
    return false;
}

bool assignment_operator(Kind k, Op* op) {
    switch (k) {
    case Kind::equal: *op = Op::assign; return true;
    case Kind::starequal: *op = Op::multiply_assign; return true;
    case Kind::slashequal: *op = Op::divide_assign; return true;
    case Kind::percentequal: *op = Op::remainder_assign; return true;
    case Kind::plusequal: *op = Op::add_assign; return true;
    case Kind::minusequal: *op = Op::subtract_assign; return true;
    case Kind::lesslessequal: *op = Op::shift_left_assign; return true;
    case Kind::greatergreaterequal: *op = Op::shift_right_assign; return true;
    case Kind::ampequal: *op = Op::and_assign; return true;
    case Kind::caretequal: *op = Op::xor_assign; return true;
    case Kind::pipeequal: *op = Op::or_assign; return true;
    default: return false;
    }
}

} // namespace

// A name the compiler gives meaning to, whose arguments may be types: `__builtin_*` and Clang's type-trait keywords. A
// library's own `__is_x(...)` is a function like any other (libc++'s are).
bool BodyParser::is_builtin_name(std::string_view w) {
    if (w.starts_with("__builtin_")) return true;
    static constexpr std::string_view TRAITS[] {
        "__has_nothrow_assign", "__has_nothrow_move_assign", "__has_nothrow_copy", "__has_nothrow_constructor", "__has_trivial_assign",
        "__has_trivial_move_assign", "__has_trivial_copy", "__has_trivial_constructor", "__has_trivial_move_constructor", "__has_trivial_destructor",
        "__has_virtual_destructor", "__has_unique_object_representations", "__is_abstract", "__is_aggregate", "__is_base_of", "__is_class",
        "__is_convertible_to", "__is_empty", "__is_enum", "__is_final", "__is_literal", "__is_standard_layout", "__is_pod", "__is_polymorphic",
        "__is_sealed", "__is_trivial", "__is_union", "__is_assignable", "__is_constructible", "__is_nothrow_assignable", "__is_nothrow_constructible",
        "__is_trivially_assignable", "__is_trivially_constructible", "__is_trivially_copyable", "__is_lvalue_expr", "__is_rvalue_expr",
        "__is_arithmetic", "__is_floating_point", "__is_integral", "__is_complete_type", "__is_void", "__is_array", "__is_function", "__is_reference",
        "__is_lvalue_reference", "__is_rvalue_reference", "__is_fundamental", "__is_object", "__is_scalar", "__is_compound", "__is_pointer",
        "__is_member_object_pointer", "__is_member_function_pointer", "__is_member_pointer", "__is_const", "__is_volatile", "__is_signed",
        "__is_unsigned", "__is_same", "__is_same_as", "__is_convertible", "__is_nothrow_convertible", "__is_layout_compatible",
        "__is_pointer_interconvertible_base_of", "__is_bounded_array", "__is_unbounded_array", "__is_trivially_relocatable", "__is_nothrow_relocatable",
        "__is_referenceable", "__is_scoped_enum", "__is_implicit_lifetime", "__is_virtual_base_of", "__is_destructible", "__is_nothrow_destructible",
        "__is_trivially_destructible", "__is_core_convertible", "__underlying_type", "__reference_binds_to_temporary",
        "__reference_constructs_from_temporary", "__reference_converts_from_temporary", "__array_rank", "__array_extent", "__remove_cv",
        "__remove_const", "__remove_volatile", "__remove_reference_t", "__remove_pointer", "__remove_cvref", "__remove_extent", "__remove_all_extents",
        "__add_pointer", "__add_lvalue_reference", "__add_rvalue_reference", "__decay", "__make_signed", "__make_unsigned", "__type_pack_element",
        "__integer_pack", "__datasizeof", "__builtin_va_arg", "__null",
    };
    return std::ranges::contains(TRAITS, w);
}

int BodyParser::precedence(Kind k, Op* op) {
    switch (k) {
    case Kind::periodstar: *op = Op::member_pointer_dot; return 14;
    case Kind::arrowstar: *op = Op::member_pointer_arrow; return 14;
    case Kind::star: *op = Op::multiply; return 13;
    case Kind::slash: *op = Op::divide; return 13;
    case Kind::percent: *op = Op::remainder; return 13;
    case Kind::plus: *op = Op::add; return 12;
    case Kind::minus: *op = Op::subtract; return 12;
    case Kind::lessless: *op = Op::shift_left; return 11;
    case Kind::greatergreater: *op = Op::shift_right; return 11;
    case Kind::spaceship: *op = Op::spaceship; return 10;
    case Kind::less: *op = Op::less; return 9;
    case Kind::greater: *op = Op::greater; return 9;
    case Kind::lessequal: *op = Op::less_equal; return 9;
    case Kind::greaterequal: *op = Op::greater_equal; return 9;
    case Kind::equalequal: *op = Op::equal; return 8;
    case Kind::exclaimequal: *op = Op::not_equal; return 8;
    case Kind::amp: *op = Op::bit_and; return 7;
    case Kind::caret: *op = Op::bit_xor; return 6;
    case Kind::pipe: *op = Op::bit_or; return 5;
    case Kind::ampamp: *op = Op::logical_and; return 4;
    case Kind::pipepipe: *op = Op::logical_or; return 3;
    default: return 0;
    }
}

bool BodyParser::at_binary_operator(Op* op, int* prec) const {
    if (eof()) return false;
    const Kind k { kind() };
    if (no_gt_ > 0 && at_gt()) return false;
    const int p { precedence(k, op) };
    if (p == 0) return false;
    if (fold_ok_ && is(Kind::ellipsis, 1)) return false;
    *prec = p;
    return true;
}

// ---- the levels ----

ExprId BodyParser::expression() {
    const std::uint32_t first { here() };
    ExprId lhs { assignment_expression() };
    while (!failed_ && is(Kind::comma) && !(fold_ok_ && is(Kind::ellipsis, 1))) {
        next();
        const ExprId rhs { assignment_expression() };
        Expr e;
        e.kind = ExprKind::binary;
        e.op = Op::comma;
        e.first = first;
        e.last = prev();
        e.a = lhs;
        e.b = rhs;
        lhs = add(e);
    }
    return lhs;
}

ExprId BodyParser::throw_expression() {
    const std::uint32_t first { here() };
    next();   // throw
    ExprId operand;
    const Kind k { kind() };
    if (!(eof() || k == Kind::r_paren || k == Kind::semi || k == Kind::comma || k == Kind::colon || k == Kind::r_square || k == Kind::r_brace))
        operand = assignment_expression();
    Expr e;
    e.kind = ExprKind::throw_;
    e.first = first;
    e.last = prev();
    e.a = operand;
    return add(e);
}

ExprId BodyParser::assignment_expression() {
    if (failed_) return error_expr(here());
    if (eof()) {
        fail("expected an expression");
        return error_expr(here());
    }
    if (++depth_ > 400) {
        --depth_;
        fail("expression nested too deeply");
        return error_expr(here());
    }
    struct Depth {
        int& d;
        ~Depth() { --d; }
    } depth_guard { depth_ };
    const std::uint32_t first { here() };
    if (word("throw")) return throw_expression();
    if (word("co_yield")) {
        next();
        const ExprId operand { initializer_clause() };
        return make_expr(ExprKind::co_yield_, first, operand);
    }
    const ExprId lhs { conditional_expression() };
    Op op;
    if (!failed_ && !eof() && assignment_operator(kind(), &op) && !(no_gt_ > 0 && at_gt()) && !(fold_ok_ && is(Kind::ellipsis, 1))) {
        const std::uint32_t at { here() };
        next();
        const ExprId rhs { initializer_clause() };
        Expr e;
        e.kind = ExprKind::assign;
        e.op = op;
        e.first = first;
        e.last = prev();
        e.a = lhs;
        e.b = rhs;
        e.token = at;
        return add(e);
    }
    return lhs;
}

ExprId BodyParser::initializer_clause() {
    if (is(Kind::l_brace)) return braced_init_list();
    return assignment_expression();
}

ExprId BodyParser::conditional_expression() {
    const std::uint32_t first { here() };
    const ExprId condition { binary_expression(3) };
    if (failed_ || !is(Kind::question)) return condition;
    next();
    ExprId when_true;
    if (!is(Kind::colon)) {
        Nested nest { *this, false, no_gt_ };
        when_true = expression();
    }
    if (!expect(Kind::colon, "`:` of the conditional")) return error_expr(first);
    const ExprId when_false { assignment_expression() };
    Expr e;
    e.kind = ExprKind::conditional;
    e.first = first;
    e.last = prev();
    e.a = condition;
    e.b = when_true;
    e.c = when_false;
    return add(e);
}

ExprId BodyParser::binary_expression(int min_precedence) {
    ExprId lhs { cast_expression() };
    for (;;) {
        Op op;
        int prec;
        if (failed_ || !at_binary_operator(&op, &prec) || prec < min_precedence) break;
        const std::uint32_t at { here() };
        next();
        const ExprId rhs { binary_expression(prec + 1) };
        Expr e;
        e.kind = ExprKind::binary;
        e.op = op;
        e.first = tree_.expressions[lhs.index].first;
        e.last = prev();
        e.a = lhs;
        e.b = rhs;
        e.token = at;
        lhs = add(e);
    }
    return lhs;
}

// ---- casts ----

// `( type-id )` and what the cast is applied to, the cursor at the `(`. False, the cursor where it was, when
// the parentheses hold an expression.
bool BodyParser::try_cast(std::uint32_t first, ExprId& out) {
    // What the parentheses could hold as a type: a keyword that only a type starts, or a name that may be one.
    bool certain { false }, candidate { false };
    ast::Basis basis { ast::Basis::syntax };
    const std::size_t at { i_ + 1 };
    {
        const Mark m { mark() };
        next();   // (
        if (at_type_start(false)) {
            candidate = true;
            certain = true;
        } else if (ident() || is(Kind::coloncolon)) {
            // A name: a type when scopes or lookup say so; unknown is a candidate, decided by what follows.
            ++speculating_;
            const bool was_failed { std::exchange(failed_, false) };
            const NameId n { qualified_name(NameUse::type) };
            const bool parsed { !failed_ && n };
            failed_ = was_failed;
            --speculating_;
            if (parsed && is(Kind::r_paren) || (parsed && (is(Kind::star) || is(Kind::amp) || is(Kind::ampamp) || is(Kind::l_square) || is(Kind::coloncolon) ||
                                                           word("const") || word("volatile")))) {
                const NameInfo info { classify_name(n, at) };
                basis = info.basis;
                if (type_like(info.what) || type_template(info.what)) certain = candidate = true;
                else if (info.what == NameClass::unknown && !info.dependent) candidate = true;
                else if (info.dependent) {
                    // A dependent name is a type only with `typename`: the C-style cast of one is rare; as an expression.
                    candidate = false;
                }
            }
        }
        rewind(m);
    }
    if (!candidate) return false;
    const Mark m { mark() };
    TypeId type;
    bool closed { false };
    {
        ++speculating_;
        const bool was_failed { std::exchange(failed_, false) };
        next();   // (
        {
            Nested nest { *this, false, 0 };
            type = type_id();
        }
        closed = !failed_ && accept(Kind::r_paren);
        const bool ok { !failed_ && closed };
        failed_ = was_failed;
        --speculating_;
        if (!ok) {
            rewind(m);
            return false;
        }
    }
    // `(T *)`, `(T &)`, `(T [3])`: no expression reads that way, so it is a type whatever T is.
    switch (tree_.types[type.index].kind) {
    case TypeKind::pointer: case TypeKind::lvalue_ref: case TypeKind::rvalue_ref: case TypeKind::member_pointer: case TypeKind::array: case TypeKind::function:
        certain = true;
        break;
    default: break;
    }
    // What follows decides when the type was only a name that may be one: an operand that cannot continue a
    // binary expression makes it a cast; `+ - * &` could be either.
    const Kind follower { kind() };
    bool cast { false };
    if (eof()) cast = false;
    else if (certain) {
        cast = starts_unary_operand(follower) || word("this") || word("new") || word("sizeof") || word("true") || word("false") || word("nullptr") ||
               word("alignof") || word("co_await") || is(Kind::l_brace);
        // A `(` after a certain type is a cast of a parenthesized operand: `(int)(x)`.
    } else {
        const bool is_name_start { follower == Kind::raw_identifier };
        const bool literal { follower == Kind::numeric_constant || is_string(follower) || follower == Kind::char_constant };
        cast = literal || (is_name_start && !word("and") && !word("or")) || follower == Kind::exclaim || follower == Kind::tilde ||
               (follower == Kind::l_paren && tree_.types[type.index].kind == TypeKind::named &&
                (tree_.names[tree_.types[type.index].name.index].components.count > 1 ||
                 spelled(tree_.components[tree_.names[tree_.types[type.index].name.index].components.begin].token).ends_with("_t")));
    }
    if (!cast) {
        rewind(m);
        note(Ambiguity::cast_or_parenthesized, Resolution::parenthesized, certain ? ast::Basis::syntax : ast::Basis::shape, at);
        return false;
    }
    note(Ambiguity::cast_or_parenthesized, Resolution::cast, certain ? (basis == ast::Basis::shape ? ast::Basis::syntax : basis) : ast::Basis::shape, at);
    const ExprId operand { is(Kind::l_brace) ? braced_init_list() : cast_expression() };
    Expr e;
    e.kind = ExprKind::cast_c;
    e.first = first;
    e.last = prev();
    e.type = type;
    e.a = operand;
    out = add(e);
    return true;
}

ExprId BodyParser::cast_expression() {
    if (failed_) return error_expr(here());
    if (is(Kind::l_paren)) {
        ExprId e;
        if (try_cast(here(), e)) return e;
        if (failed_) return error_expr(here());
    }
    return unary_expression();
}

ExprId BodyParser::unary_expression() {
    if (failed_) return error_expr(here());
    if (eof()) {
        fail("expected an expression");
        return error_expr(here());
    }
    if (++depth_ > 400) {
        --depth_;
        fail("expression nested too deeply");
        return error_expr(here());
    }
    struct Depth {
        int& d;
        ~Depth() { --d; }
    } depth_guard { depth_ };
    const std::uint32_t first { here() };
    Op op { Op::none };
    switch (kind()) {
    case Kind::plusplus: op = Op::pre_increment; break;
    case Kind::minusminus: op = Op::pre_decrement; break;
    case Kind::plus: op = Op::plus; break;
    case Kind::minus: op = Op::minus; break;
    case Kind::exclaim: op = Op::not_; break;
    case Kind::tilde: op = Op::complement; break;
    case Kind::star: op = Op::deref; break;
    case Kind::amp: op = Op::address_of; break;
    case Kind::raw_identifier:
        if (word("__real__") || word("__real")) op = Op::real_part;
        else if (word("__imag__") || word("__imag")) op = Op::imag_part;
        break;
    case Kind::ampamp:
        if (ident(1)) {   // GNU: &&label
            next();
            Expr e;
            e.kind = ExprKind::label_address;
            e.first = first;
            e.token = here();
            next();
            e.last = prev();
            return add(e);
        }
        break;
    default: break;
    }
    if (op != Op::none) {
        const std::uint32_t at { here() };
        next();
        const ExprId operand { cast_expression() };
        Expr e;
        e.kind = ExprKind::unary;
        e.op = op;
        e.first = first;
        e.last = prev();
        e.a = operand;
        e.token = at;
        return add(e);
    }
    if (word("sizeof") || word("alignof") || word("_Alignof") || word("__alignof__") || word("__alignof")) return sizeof_expression();
    if (word("noexcept") && is(Kind::l_paren, 1)) {
        next(), next();
        const ExprId operand { [&] {
            Nested nest { *this, false, 0 };
            return expression();
        }() };
        expect(Kind::r_paren, "`)`");
        return make_expr(ExprKind::noexcept_, first, operand);
    }
    if (word("co_await")) {
        next();
        const ExprId operand { cast_expression() };
        return make_expr(ExprKind::co_await_, first, operand);
    }
    if (word("new") || (is(Kind::coloncolon) && word("new", 1))) return new_expression();
    if (word("delete") || (is(Kind::coloncolon) && word("delete", 1))) return delete_expression();
    if (word("throw")) return throw_expression();
    return postfix_expression();
}

// ---- postfix ----

ExprId BodyParser::postfix_expression() {
    const std::uint32_t first { here() };
    const ExprId primary { primary_expression() };
    if (failed_) return primary;
    return postfix_suffixes(primary, first);
}

ExprId BodyParser::postfix_suffixes(ExprId lhs, std::uint32_t first) {
    for (;;) {
        if (failed_ || eof()) return lhs;
        const Kind k { kind() };
        if (k == Kind::l_paren) {
            next();
            Nested nest { *this, false, 0 };
            const Span32 args { argument_list(Kind::r_paren) };
            Expr e;
            e.kind = ExprKind::call;
            e.first = first;
            e.last = prev();
            e.a = lhs;
            e.list = args;
            lhs = add(e);
        } else if (k == Kind::l_square) {
            next();
            Nested nest { *this, false, 0 };
            std::vector<Handle> indices;
            while (!failed_ && !is(Kind::r_square) && !eof()) {
                indices.push_back(initializer_clause().handle());
                if (!accept(Kind::comma)) break;
            }
            expect(Kind::r_square, "`]`");
            Expr e;
            e.kind = ExprKind::subscript;
            e.first = first;
            e.last = prev();
            e.a = lhs;
            e.list = emit(indices);
            lhs = add(e);
        } else if (k == Kind::period || k == Kind::arrow) {
            lhs = member_access(lhs, first);
        } else if (k == Kind::plusplus || k == Kind::minusminus) {
            const std::uint32_t at { here() };
            next();
            Expr e;
            e.kind = ExprKind::unary;
            e.op = k == Kind::plusplus ? Op::post_increment : Op::post_decrement;
            e.flags = Expr::postfix;
            e.first = first;
            e.last = prev();
            e.a = lhs;
            e.token = at;
            lhs = add(e);
        } else if (k == Kind::ellipsis && is(Kind::l_square, 1) && !extensions_.empty()) {
            bool taken { false };
            for (const auto& ext : extensions_)
                if (ext->expression_postfix(*this, lhs)) {
                    taken = true;
                    break;
                }
            if (!taken) return lhs;
        } else {
            return lhs;
        }
    }
}

ExprId BodyParser::member_access(ExprId lhs, std::uint32_t first) {
    const bool arrow { is(Kind::arrow) };
    next();
    Expr e;
    e.kind = ExprKind::member;
    e.first = first;
    e.a = lhs;
    if (arrow) e.flags |= Expr::arrow;
    e.name = qualified_name(NameUse::member);
    e.last = prev();
    return add(e);
}

Span32 BodyParser::argument_list(Kind close, bool*) {
    std::vector<Handle> args;
    while (!failed_ && !is(close) && !eof()) {
        ExprId arg { initializer_clause() };
        if (failed_) break;
        if (is(Kind::ellipsis) && !is(Kind::l_square, 1)) {
            const std::uint32_t first { tree_.expressions[arg.index].first };
            next();
            Expr pack;
            pack.kind = ExprKind::pack_expansion;
            pack.first = first;
            pack.last = prev();
            pack.a = arg;
            arg = add(pack);
        }
        args.push_back(arg.handle());
        if (!accept(Kind::comma)) break;
    }
    if (!failed_) expect(close, close == Kind::r_paren ? "`)`" : "`]`");
    return emit(args);
}

// ---- primaries ----

ExprId BodyParser::literal_expression() {
    const std::uint32_t first { here() };
    Expr e;
    e.first = first;
    e.token = first;
    const Kind k { kind() };
    if (k == Kind::numeric_constant) {
        const Numeric n { numeric(tok().spelling) };
        e.kind = n.floating ? ExprKind::floating : ExprKind::integer;
        if (n.user_defined) e.flags |= Expr::user_defined;
        next();
    } else if (is_string(k)) {
        e.kind = ExprKind::string;
        while (is_string(kind())) {
            if (has_suffix(tok().spelling, '"')) e.flags |= Expr::user_defined;
            next();
        }
    } else {
        e.kind = ExprKind::character;
        if (has_suffix(tok().spelling, '\'')) e.flags |= Expr::user_defined;
        next();
    }
    e.last = prev();
    return add(e);
}

ExprId BodyParser::parenthesized_expression() {
    const std::uint32_t first { here() };
    next();   // (
    if (is(Kind::l_brace)) {   // GNU statement expression: ({ ... })
        Nested nest { *this, false, 0 };
        const StmtId block { committed_compound() };
        expect(Kind::r_paren, "`)`");
        Expr e;
        e.kind = ExprKind::statement_expression;
        e.first = first;
        e.last = prev();
        e.stmt = block;
        return add(e);
    }
    Nested nest { *this, true, 0 };
    return fold_or_paren(first);
}

// Inside parentheses: a fold-expression or an expression. The cursor after the `(`.
ExprId BodyParser::fold_or_paren(std::uint32_t first) {
    Op op { Op::none };
    const auto make_fold = [&](ExprId pattern, ExprId init, Op fold_op, bool left, bool binary) {
        FoldInfo f;
        f.pattern = pattern;
        f.init = init;
        f.op = fold_op;
        f.left = left;
        f.binary = binary;
        tree_.folds.push_back(f);
        Expr e;
        e.kind = ExprKind::fold;
        e.first = first;
        e.last = prev();
        e.aux = static_cast<std::uint32_t>(tree_.folds.size() - 1);
        return add(e);
    };
    if (is(Kind::ellipsis)) {   // ( ... op e )
        next();
        if (!fold_operator(kind(), &op)) {
            fail("expected an operator after `...`");
            return error_expr(first);
        }
        next();
        const ExprId pattern { cast_expression() };
        if (!expect(Kind::r_paren, "`)`")) return error_expr(first);
        return make_fold(pattern, {}, op, true, false);
    }
    const ExprId inner { expression() };
    if (failed_) return error_expr(first);
    if (fold_operator(kind(), &op) && is(Kind::ellipsis, 1)) {   // ( e op ... ) or ( e op ... op e )
        next();
        next();
        if (is(Kind::r_paren)) {
            next();
            return make_fold(inner, {}, op, false, false);
        }
        Op again;
        if (!fold_operator(kind(), &again)) {
            fail("expected `)` or an operator after `...`");
            return error_expr(first);
        }
        next();
        const ExprId other { cast_expression() };
        if (!expect(Kind::r_paren, "`)`")) return error_expr(first);
        return make_fold(inner, other, op, false, true);
    }
    if (!expect(Kind::r_paren, "`)`")) return error_expr(first);
    return make_expr(ExprKind::paren, first, inner);
}

ExprId BodyParser::braced_init_list() {
    const std::uint32_t first { here() };
    next();   // {
    Nested nest { *this, false, 0 };
    std::vector<Handle> elements;
    while (!failed_ && !is(Kind::r_brace) && !eof()) {
        elements.push_back(designator_or_element().handle());
        if (failed_) break;
        if (is(Kind::ellipsis)) {
            // a pack expansion of the element
            const auto last { elements.back() };
            const std::uint32_t at { tree_.tokens(last).first };
            next();
            Expr pack;
            pack.kind = ExprKind::pack_expansion;
            pack.first = at;
            pack.last = prev();
            pack.a = { last.index };
            elements.back() = add(pack).handle();
        }
        if (!accept(Kind::comma)) break;
    }
    if (!failed_) expect(Kind::r_brace, "`}`");
    Expr e;
    e.kind = ExprKind::init_list;
    e.first = first;
    e.last = prev();
    e.list = emit(elements);
    return add(e);
}

// One element of a braced list: `.a.b = e`, `[i] = e`, `.a{...}` or an initializer-clause.
ExprId BodyParser::designator_or_element() {
    const std::uint32_t first { here() };
    const bool field { is(Kind::period) && ident(1) };
    // `[i] = v` (C99, GNU) and not a lambda `[&] { ... }`: the `]` is followed by `=`.
    const bool index { is(Kind::l_square) && !(is(Kind::l_square, 1)) && !(is(Kind::colon, 1) && adjacent(1)) && balanced(i_) < end_ &&
                       t_[balanced(i_)].kind == Kind::equal };
    if (!field && !index) return initializer_clause();
    std::vector<Designator> designators;
    while (!failed_ && ((is(Kind::period) && ident(1)) || (is(Kind::l_square) && !(is(Kind::colon, 1) && adjacent(1))))) {
        Designator d;
        d.first = here();
        if (is(Kind::period)) {
            next();
            d.token = here();
            next();
        } else {
            d.field = false;
            next();
            d.index = conditional_expression();
            if (accept(Kind::ellipsis)) d.end = conditional_expression();
            expect(Kind::r_square, "`]`");
        }
        d.last = prev();
        designators.push_back(d);
    }
    ExprId value;
    if (is(Kind::l_brace)) value = braced_init_list();   // `.a{...}`
    else if (accept(Kind::equal)) value = initializer_clause();
    else {
        fail("expected `=` after a designator");
        return error_expr(first);
    }
    Expr e;
    e.kind = ExprKind::designated;
    e.first = first;
    e.last = prev();
    e.a = value;
    e.aux = static_cast<std::uint32_t>(tree_.designators.size());
    e.list.count = static_cast<std::uint32_t>(designators.size());
    tree_.designators.insert(tree_.designators.end(), designators.begin(), designators.end());
    return add(e);
}

ExprId BodyParser::primary_expression() {
    if (failed_) return error_expr(here());
    if (eof()) {
        fail("expected an expression");
        return error_expr(here());
    }
    const std::uint32_t first { here() };
    const Kind k { kind() };
    if (k == Kind::numeric_constant || is_string(k) || k == Kind::char_constant || k == Kind::wide_char_constant || k == Kind::utf8_char_constant ||
        k == Kind::utf16_char_constant || k == Kind::utf32_char_constant)
        return literal_expression();
    if (k == Kind::l_paren) return parenthesized_expression();
    // `[: r :]::member`: a name whose first part is a splice, not a splice expression.
    if (at_splice() && !extensions_.empty() && is(Kind::coloncolon, splice_end(i_) - i_)) return name_expression();
    if (!extensions_.empty()) {
        ExprId out;
        for (const auto& ext : extensions_)
            if (ext->expression_primary(*this, out)) return out;
    }
    if (k == Kind::l_square) return lambda_expression();
    if (k == Kind::raw_identifier) {
        const std::string_view w { tok().spelling };
        if (w == "this") {
            next();
            return make_expr(ExprKind::this_, first);
        }
        if (w == "true" || w == "false") {
            next();
            return make_expr(ExprKind::boolean, first);
        }
        if (w == "nullptr" || w == "__null") {
            next();
            return make_expr(ExprKind::null_pointer, first);
        }
        if (w == "static_cast" || w == "dynamic_cast" || w == "const_cast" || w == "reinterpret_cast") return named_cast();
        if (w == "typeid") return typeid_expression();
        if (w == "requires") return requires_expression();
        if (w == "__extension__") {
            next();
            return primary_expression();
        }
        if (w == "typename" || w == "decltype" || w == "auto" || is_type_keyword(w)) {
            // A functional cast: T(x), T{x}, with a builtin or decltype type.
            const TypeId type { simple_type() };
            if (failed_) return error_expr(first);
            if (!(is(Kind::l_paren) || is(Kind::l_brace))) {
                fail("expected `(` or `{` after a type in an expression");
                return error_expr(first);
            }
            return functional_cast(type, first);
        }
    }
    if (starts_name()) return name_expression();
    fail(std::format("expected an expression, found `{}`", tok().spelling));
    return error_expr(first);
}

ExprId BodyParser::functional_cast(TypeId type, std::uint32_t first) {
    Expr e;
    e.kind = ExprKind::cast_functional;
    e.first = first;
    e.type = type;
    if (is(Kind::l_brace)) {
        const ExprId list { braced_init_list() };
        e.flags |= Expr::brace;
        e.list = tree_.expressions[list.index].list;
    } else {
        next();   // (
        Nested nest { *this, false, 0 };
        e.list = argument_list(Kind::r_paren);
    }
    e.last = prev();
    return add(e);
}

ExprId BodyParser::name_expression() {
    const std::uint32_t first { here() };
    if (ident() && is(Kind::l_paren, 1) && is_builtin_name(spelled(first))) return builtin_expression();
    bool dependent { false };
    const NameId n { qualified_name(NameUse::expression, &dependent, nullptr) };
    if (failed_ || !n) {
        if (!failed_) fail("expected a name");
        return error_expr(first);
    }
    if (is(Kind::l_paren) || (is(Kind::l_brace) && !no_brace_)) {
        const NameInfo info { classify_name(n, first) };
        const bool brace { is(Kind::l_brace) && !no_brace_ };
        const bool type { type_like(info.what) || type_template(info.what) };
        if (type || brace) {
            note(Ambiguity::name, type ? Resolution::type : Resolution::unknown, type ? info.basis : ast::Basis::shape, first);
            TypeNode named;
            named.kind = TypeKind::named;
            named.first = first;
            named.last = prev();
            named.name = n;
            return functional_cast(add(named), first);
        }
    }
    Expr e;
    e.kind = ExprKind::id;
    e.first = first;
    e.last = prev();
    e.name = n;
    return add(e);
}

// A GNU builtin or a type trait: `__builtin_offsetof(T, m)`, `__is_same(A, B)`, `__builtin_va_arg(ap, T)`.
ExprId BodyParser::builtin_expression() {
    const std::uint32_t first { here() };
    next();   // the name
    next();   // (
    Nested nest { *this, false, 0 };
    std::vector<Handle> args;
    const std::string_view name { spelled(first) };
    // Which arguments are values where nothing says: a `__builtin_` function's, but the types it takes; the
    // type traits (`__is_same`) take types.
    const auto value_at = [&](std::size_t index) {
        if (!name.starts_with("__builtin_")) return false;
        if (name == "__builtin_offsetof" || name == "__builtin_bit_cast" || name == "__builtin_convertvector") return index != 0;
        if (name == "__builtin_va_arg") return index == 0;
        return name != "__builtin_types_compatible_p";
    };
    while (!failed_ && !is(Kind::r_paren) && !eof()) {
        args.push_back(type_or_expression_biased(value_at(args.size())));
        if (!accept(Kind::comma)) break;
    }
    expect(Kind::r_paren, "`)`");
    Expr e;
    e.kind = ExprKind::builtin;
    e.first = first;
    e.last = prev();
    e.token = first;
    e.list = emit(args);
    return add(e);
}

ExprId BodyParser::named_cast() {
    const std::uint32_t first { here() };
    const std::string_view w { tok().spelling };
    Op op { Op::static_cast_ };
    if (w == "dynamic_cast") op = Op::dynamic_cast_;
    else if (w == "const_cast") op = Op::const_cast_;
    else if (w == "reinterpret_cast") op = Op::reinterpret_cast_;
    next();
    if (!expect(Kind::less, "`<`")) return error_expr(first);
    TypeId type;
    {
        Nested nest { *this, false, 1 };
        type = type_id();
    }
    if (failed_ || !expect_gt() || !expect(Kind::l_paren, "`(`")) return error_expr(first);
    ExprId operand;
    {
        Nested nest { *this, false, 0 };
        operand = expression();
    }
    expect(Kind::r_paren, "`)`");
    Expr e;
    e.kind = ExprKind::cast_named;
    e.op = op;
    e.first = first;
    e.last = prev();
    e.type = type;
    e.a = operand;
    return add(e);
}

ExprId BodyParser::typeid_expression() {
    const std::uint32_t first { here() };
    next();
    if (!expect(Kind::l_paren, "`(`")) return error_expr(first);
    Nested nest { *this, false, 0 };
    const Handle operand { type_or_expression() };
    expect(Kind::r_paren, "`)`");
    Expr e;
    e.kind = ExprKind::typeid_;
    e.first = first;
    e.last = prev();
    if (operand.sort == Sort::type) e.type = { operand.index };
    else e.a = { operand.index };
    return add(e);
}

ExprId BodyParser::sizeof_expression() {
    const std::uint32_t first { here() };
    const bool is_alignof { !word("sizeof") };
    next();
    Expr e;
    e.first = first;
    if (is_alignof) e.flags |= Expr::alignof_;
    if (!is_alignof && is(Kind::ellipsis)) {   // sizeof...( pack )
        next();
        if (!expect(Kind::l_paren, "`(`")) return error_expr(first);
        e.kind = ExprKind::sizeof_pack;
        e.name = qualified_name(NameUse::expression);
        expect(Kind::r_paren, "`)`");
        e.last = prev();
        return add(e);
    }
    e.kind = ExprKind::sizeof_;
    if (is(Kind::l_paren)) {
        // `sizeof ( type-id )` when the parentheses hold a type; else the operand is a unary-expression that starts with one.
        const Mark m { mark() };
        bool as_type { false };
        {
            ++speculating_;
            const bool was_failed { std::exchange(failed_, false) };
            next();
            Nested nest { *this, false, 0 };
            if (at_type_start(true) || ident() || is(Kind::coloncolon)) {
                const Handle h { type_or_expression_biased(true) };
                if (!failed_ && is(Kind::r_paren) && h.sort == Sort::type) {
                    next();
                    e.type = { h.index };
                    as_type = true;
                }
            }
            failed_ = was_failed;
            --speculating_;
        }
        if (as_type) {
            e.last = prev();
            return add(e);
        }
        rewind(m);
    }
    const ExprId operand { unary_expression() };
    e.a = operand;
    e.last = prev();
    return add(e);
}

ExprId BodyParser::new_expression() {
    const std::uint32_t first { here() };
    News n;
    Expr e;
    e.kind = ExprKind::new_;
    e.first = first;
    if (accept(Kind::coloncolon)) e.flags |= Expr::global;
    next();   // new
    Nested nest { *this, false, 0 };
    std::vector<Handle> placement;
    // `new (placement) T`, `new (T)`, `new T`: parentheses first hold a type or a placement.
    if (is(Kind::l_paren)) {
        const Mark m { mark() };
        bool as_type { false };
        {
            ++speculating_;
            const bool was_failed { std::exchange(failed_, false) };
            next();
            if (at_type_start(true) || ident() || is(Kind::coloncolon)) {
                const TypeId t { type_id() };
                if (!failed_ && is(Kind::r_paren)) {
                    next();
                    // `new (T)(args)` / `new (T)`: unless what follows starts the type of a placement-new
                    // (`new (buf) T`), which then has a type of its own.
                    const bool placement_follows { ident() || at_type_start(false) || is(Kind::coloncolon) };
                    if (!placement_follows) {
                        n.type = t;
                        n.paren_type = true;
                        as_type = true;
                    }
                }
            }
            failed_ = was_failed;
            --speculating_;
        }
        if (!as_type) {
            rewind(m);
            next();   // (
            Span32 args { argument_list(Kind::r_paren) };
            n.placement = args;
        }
    }
    if (!n.type) {
        // new-type-id: specifiers and a new-declarator (pointers, array bounds).
        const DeclSpec spec { decl_specifiers(Site::parameter) };
        if (!spec.has_type) {
            fail("expected a type after `new`");
            return error_expr(first);
        }
        Declarator d { declarator(spec.type, Mode::new_declarator, Site::parameter) };
        TypeId type { d.type ? d.type : spec.type };
        // The first array bound is the allocation's count: `new T[n]` allocates n Ts.
        if (tree_.types[type.index].kind == TypeKind::array) {
            e.flags |= Expr::array;
            n.bound = tree_.types[type.index].expr;
            type = tree_.types[type.index].inner;
        }
        n.type = type;
    }
    // The initializer: (args) or {init}.
    if (is(Kind::l_paren)) {
        next();
        const std::uint32_t at { prev() };
        Expr init;
        init.kind = ExprKind::paren_list;
        init.first = at;
        init.list = argument_list(Kind::r_paren);
        init.last = prev();
        n.init = add(init);
        n.initialized = true;
    } else if (is(Kind::l_brace)) {
        n.init = braced_init_list();
        n.initialized = true;
        n.brace_init = true;
    }
    e.last = prev();
    tree_.news.push_back(n);
    e.aux = static_cast<std::uint32_t>(tree_.news.size() - 1);
    return add(e);
}

ExprId BodyParser::delete_expression() {
    const std::uint32_t first { here() };
    Expr e;
    e.kind = ExprKind::delete_;
    e.first = first;
    if (accept(Kind::coloncolon)) e.flags |= Expr::global;
    next();   // delete
    if (is(Kind::l_square) && is(Kind::r_square, 1)) {
        advance(2);
        e.flags |= Expr::array;
    }
    e.a = cast_expression();
    e.last = prev();
    return add(e);
}

// requires-clause operands: primaries joined by && and ||.
ExprId BodyParser::constraint_expression() {
    const auto operand = [&]() -> ExprId {
        const std::uint32_t first { here() };
        if (is(Kind::l_paren)) return parenthesized_expression();
        if (is(Kind::exclaim)) {
            next();
            const ExprId inner { constraint_primary_for_not() };
            Expr e;
            e.kind = ExprKind::unary;
            e.op = Op::not_;
            e.first = first;
            e.last = prev();
            e.a = inner;
            return add(e);
        }
        return postfix_expression();
    };
    const std::uint32_t first { here() };
    const bool saved_brace { std::exchange(no_brace_, true) };
    struct Restore {
        bool& flag;
        bool saved;
        ~Restore() { flag = saved; }
    } restore_brace { no_brace_, saved_brace };
    ExprId lhs { operand() };
    while (!failed_ && (is(Kind::ampamp) || is(Kind::pipepipe))) {
        const Op op { is(Kind::ampamp) ? Op::logical_and : Op::logical_or };
        next();
        const ExprId rhs { operand() };
        Expr e;
        e.kind = ExprKind::binary;
        e.op = op;
        e.first = first;
        e.last = prev();
        e.a = lhs;
        e.b = rhs;
        lhs = add(e);
    }
    return lhs;
}

ExprId BodyParser::constraint_primary_for_not() {
    if (is(Kind::l_paren)) return parenthesized_expression();
    return postfix_expression();
}

} // namespace mcxx::frontend::bodies
