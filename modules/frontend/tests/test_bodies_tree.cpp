// mcxx.frontend:bodies, the tree itself: handles that name a node and give it back (A3.0.4), the walk and its
// consistency, ranges, the questions the parse asks (semantic feedback) and who answered, a language
// extension written outside the core, and what the outline mode does and does not do.
import std;
import mcxx.testing;
import mcxx.msa;
import mcxx.base;
import mcxx.frontend;

namespace f = mcxx::frontend;
namespace ast = mcxx::frontend::ast;
namespace msa = mcxx::msa;

namespace {

void same(const std::string& actual, std::string_view expected, std::source_location where = std::source_location::current()) {
    mcxx::testing::expect(actual == expected, where) << "\n    got:      " << actual << "\n    expected: " << expected;
}

const char* const SAMPLE = R"(struct Box { int w; int area() const { return w * w; } };
int sum(const std::vector<int>& v) {
    int total = 0;
    for (auto x : v) { if (x > 0) total += x; else continue; }
    auto twice = [&](int a) { return a * 2; };
    try { total = twice(total); } catch (...) { throw; }
    return total;
}
)";

// Every handle of the tree, by sort.
std::vector<ast::Handle> every(const ast::Tree& tree) {
    std::vector<ast::Handle> out;
    for (std::uint32_t i { 0 }; i < tree.expressions.size(); ++i) out.push_back({ ast::Sort::expression, i });
    for (std::uint32_t i { 0 }; i < tree.statements.size(); ++i) out.push_back({ ast::Sort::statement, i });
    for (std::uint32_t i { 0 }; i < tree.types.size(); ++i) out.push_back({ ast::Sort::type, i });
    for (std::uint32_t i { 0 }; i < tree.locals.size(); ++i) out.push_back({ ast::Sort::local, i });
    for (std::uint32_t i { 0 }; i < tree.names.size(); ++i) out.push_back({ ast::Sort::name, i });
    return out;
}

// What a name is to the test: `Widget` a type, `gadget` a value, anything else unknown.
class TestOracle final : public f::NameOracle {
public:
    int asked { 0 };
    f::NameAnswer classify(const f::NameQuery& query) override {
        ++asked;
        if (query.name == "Widget") return { f::NameClass::type, ast::Basis::feedback };
        if (query.name == "gadget") return { f::NameClass::value, ast::Basis::feedback };
        return {};
    }
};

// A language extension written here, not in the core: `unless (cond) stmt` and the primary `forty_two`.
class Unless final : public f::SyntaxExtension {
public:
    std::string_view name() const override { return "test.unless"; }
    std::span<const std::string_view> features() const override { return IDS; }
    bool expression_primary(f::SyntaxContext& c, ast::ExprId& out) const override {
        if (c.token().kind != f::Kind::raw_identifier || c.token().spelling != "forty_two") return false;
        ast::Expr e;
        e.kind = ast::ExprKind::integer;
        e.first = e.last = e.token = c.position();
        c.advance();
        out = c.make(e);
        return true;
    }
    bool statement(f::SyntaxContext& c, ast::StmtId& out) const override {
        if (c.token().kind != f::Kind::raw_identifier || c.token().spelling != "unless") return false;
        const std::uint32_t first { c.position() };
        c.advance();
        c.expect(f::Kind::l_paren, "`(`");
        ast::Expr negated;
        negated.kind = ast::ExprKind::unary;
        negated.op = ast::Op::not_;
        negated.first = c.position();
        negated.a = c.assignment_expression();
        negated.last = c.position() - 1;
        c.expect(f::Kind::r_paren, "`)`");
        ast::Stmt s;
        s.kind = ast::StmtKind::if_;
        s.first = first;
        s.e1 = c.make(negated);
        s.b = c.statement();
        s.last = c.position() - 1;
        out = c.make(s);
        return true;
    }

private:
    static constexpr std::string_view IDS[] { "test:unless" };
};

} // namespace

int main() {
    using namespace mcxx::testing;

    "every node has a handle that gives the node back; a handle of nothing gives nothing (A3.0.4)"_test = [] {
        const auto syntax = f::parse(SAMPLE);
        const auto tree = f::parse_bodies(syntax);
        expect(!tree.roots.empty());
        std::size_t seen { 0 };
        for (const auto h : every(tree)) {
            const auto view = tree.get(h);
            const void* node = view.expression != nullptr ? static_cast<const void*>(view.expression) : view.statement != nullptr ? static_cast<const void*>(view.statement)
                               : view.type != nullptr ? static_cast<const void*>(view.type) : view.local != nullptr ? static_cast<const void*>(view.local)
                                                                                                                       : static_cast<const void*>(view.name);
            expect(node != nullptr);
            ++seen;
        }
        expect(seen > 50) << seen;
        expect(tree.get({ ast::Sort::expression, 1000000 }).expression == nullptr);
        expect(tree.get(ast::Handle {}).expression == nullptr);
        // A typed id and its handle are the same node.
        const ast::ExprId some { 3 };
        expect(tree.expression(some.handle()) == &tree[some]);
        // The outline's declarations are named by a handle too.
        expect(tree.get({ ast::Sort::declaration, 0 }).declaration == &syntax.declarations[0]);
    };

    "handles are the same for the same file: a second parse gives the same tree"_test = [] {
        const auto a = f::parse(SAMPLE);
        const auto b = f::parse(SAMPLE);
        const auto ta = f::parse_bodies(a);
        const auto tb = f::parse_bodies(b);
        expect(ta.expressions.size() == tb.expressions.size() && ta.statements.size() == tb.statements.size() && ta.types.size() == tb.types.size());
        expect(ta.roots.size() == tb.roots.size());
        for (std::size_t i { 0 }; i < ta.roots.size(); ++i) {
            expect(ta.roots[i].node == tb.roots[i].node);
            expect(ast::dump(ta, ta.roots[i].node) == ast::dump(tb, tb.roots[i].node));
        }
    };

    "a reflection names its operand by handle: the entity can be taken back from the tree"_test = [] {
        const auto fragment = f::parse_fragment("auto r = ^^int; auto s = ^^Widget;");
        const auto& tree = fragment.tree;
        std::size_t reflections { 0 };
        for (const auto& e : tree.expressions)
            if (e.kind == ast::ExprKind::reflect) {
                ++reflections;
                expect(e.list.count == 1);
                const auto operand = tree.list(e.list)[0];
                expect(tree.type(operand) != nullptr || tree.expression(operand) != nullptr);
                expect(!tree.text(operand).empty());
            }
        expect(reflections == 2);
    };

    "a node's children are what it holds, in order; every statement and expression is reachable from a root; the tree is sound"_test = [] {
        const auto syntax = f::parse(SAMPLE);
        const auto tree = f::parse_bodies(syntax);
        expect(ast::validate(tree).empty());
        std::set<std::pair<int, std::uint32_t>> reached;
        std::function<void(ast::Handle)> walk = [&](ast::Handle h) {
            if (!reached.emplace(static_cast<int>(h.sort), h.index).second) return;
            for (const auto child : ast::children(tree, h)) walk(child);
        };
        for (const auto& r : tree.roots) walk(r.node);
        std::size_t orphans { 0 };
        for (std::uint32_t i { 0 }; i < tree.expressions.size(); ++i) orphans += !reached.contains({ static_cast<int>(ast::Sort::expression), i });
        for (std::uint32_t i { 0 }; i < tree.statements.size(); ++i) orphans += !reached.contains({ static_cast<int>(ast::Sort::statement), i });
        expect(orphans == 0) << orphans << " nodes no root reaches";
        const auto children = ast::children(tree, tree.roots.back().node);
        expect(!children.empty());
    };

    "ranges are the tokens a node spans, as Clang gives them: a statement, an expression, a declaration"_test = [] {
        const auto fragment = f::parse_fragment("int total = a + b * 2;\nif (total > 1) {\n  g(total);\n}\n");
        const auto& tree = fragment.tree;
        const auto* function = tree.statement(tree.roots[0].node);
        const auto statements = tree.list(tree[function->a].list);
        expect(statements.size() == 2);
        const auto declaration = tree.range(statements[0]);
        // 0-based, as MC3 has them; the fragment is wrapped in `void mcxx_fragment() { `, 23 characters, on the first line.
        expect(declaration.begin.line == 0 && declaration.begin.column == 23 && declaration.end.line == 0 && declaration.end.column == 23 + 22);
        const auto if_range = tree.range(statements[1]);
        expect(if_range.begin.line == 1 && if_range.begin.column == 0 && if_range.end.line == 3 && if_range.end.column == 1);
        // `a + b * 2`: the initializer, and inside it the product.
        const ast::LocalId variable_id { tree.list(tree.statement(statements[0])->list)[0].index };
        const auto& variable = tree[variable_id];
        const auto sum = tree.range(variable.init.handle());
        expect(tree.text(variable.init.handle()) == "a + b * 2");
        expect(sum.begin.column == 23 + 12 && sum.end.column == 23 + 21);
        expect(tree.text(variable.init.handle()) == tree.text(tree[variable.init].first, tree[variable.init].last));
    };

    "the questions a parse asks: who answered a type-or-value question -- the parse's scopes, lookup, the shape of the code"_test = [] {
        f::BodyOptions options;
        options.record_decisions = true;
        // a name the parse declared, a name the file declares, a name nothing knows
        const auto syntax = f::parse("struct S {};\ntemplate <class T> void f() { T * a; }\nvoid g(int n) { S * b; n * 2; U * c; }\n");
        const auto tree = f::parse_bodies(syntax, options);
        std::map<std::string, ast::Basis> by_text;
        for (const auto& d : tree.decisions)
            if (d.what == ast::Ambiguity::declaration_or_expression)
                by_text[std::string { syntax.pp.tokens[d.token].spelling } + (d.chose == ast::Resolution::declaration ? "*decl" : "*expr")] = d.basis;
        expect(by_text.at("T*decl") == ast::Basis::scope);      // T: the template's parameter
        expect(by_text.at("S*decl") == ast::Basis::lookup);     // S: declared by the file, found by name lookup
        expect(by_text.at("n*expr") == ast::Basis::scope);      // n: the parameter
        expect(by_text.at("U*decl") == ast::Basis::shape);      // U: nothing knows it
        expect(tree.stats.decisions == tree.decisions.size());
        expect(tree.stats.by_basis[static_cast<std::size_t>(ast::Basis::shape)] >= 1);
    };

    "semantic feedback from the host: an oracle it supplies answers before the code's shape does"_test = [] {
        TestOracle oracle;
        f::BodyOptions options;
        options.oracle = &oracle;
        options.record_decisions = true;
        // `Widget * w;` is a declaration because the oracle says Widget is a type, `gadget * g;` a product because it says value.
        const auto syntax = f::parse("void f() { Widget * w; gadget * g; other * o; }");
        const auto tree = f::parse_bodies(syntax, options);
        expect(oracle.asked > 0);
        std::size_t feedback { 0 };
        for (const auto& d : tree.decisions) feedback += d.basis == ast::Basis::feedback;
        expect(feedback >= 1);
        const auto text = ast::dump(tree, tree.roots[0].node);
        expect(text.find("(declaration (variable w (pointer (named Widget))))") != std::string::npos) << text;
        expect(text.find("(declaration (variable o (pointer (named other))))") != std::string::npos) << text;
        expect(tree.stats.by_basis[static_cast<std::size_t>(ast::Basis::feedback)] >= 1);
    };

    "what the imports bring in: a class template known to lookup is a template where code alone would not tell"_test = [] {
        f::Imported imported;
        msa::fact::Declaration vector;
        vector.kind = msa::Kind::class_;
        vector.qualified_name = "std::vector";
        vector.template_parameters = { "class T" };
        imported.declarations.push_back(vector);
        f::BodyOptions options;
        options.imported = &imported;
        options.record_decisions = true;
        // `std::vector < 3` is a comparison in no reading once the template is known; unknown, the shape says arguments.
        const auto syntax = f::parse("void f(int n) { std::vector<int> v; bool b = n < 3; }");
        const auto tree = f::parse_bodies(syntax, options);
        bool by_lookup { false };
        for (const auto& d : tree.decisions) by_lookup = by_lookup || (d.what == ast::Ambiguity::template_arguments && d.chose == ast::Resolution::arguments && d.basis == ast::Basis::lookup);
        expect(by_lookup);
    };

    "a language extension written outside the core reads its own syntax through the context"_test = [] {
        f::BodyOptions options;
        options.extensions = { std::make_shared<Unless>() };
        const auto fragment = f::parse_fragment("unless (a) b(); int x = forty_two;", options);
        expect(fragment.tree.stats.failed == 0);
        const auto text = ast::dump(fragment.tree, fragment.tree.roots[0].node);
        expect(text.find("(if (unary ! (id a)) (expression (call (id b))))") != std::string::npos) << text;
        expect(text.find("(variable x (builtin int) = (integer forty_two))") != std::string::npos) << text;
        // Not registered, it is not syntax.
        f::BodyOptions core;
        core.extensions = {};
        expect(f::parse_fragment("unless (a) b();", core).tree.stats.failed == 1);
    };

    "the outline mode reads no body; the full parse is asked for, and reads what the outline marked"_test = [] {
        const auto syntax = f::parse(SAMPLE);
        expect(!syntax.parts.empty());
        std::size_t bodies { 0 };
        for (const auto& part : syntax.parts) bodies += part.role == f::Part::Role::function_body;
        expect(bodies == 2);
        // The outline is what it was: two top-level declarations, the first with its method.
        const auto outline = f::symbols(syntax);
        expect(outline.size() == 2);
        expect(outline[0].name == "Box" && outline[0].children.size() == 2);
        expect(outline[1].name == "sum");
        // Reading the bodies does not change the syntax it reads.
        const auto before = syntax.declarations.size();
        const auto tree = f::parse_bodies(syntax);
        expect(syntax.declarations.size() == before);
        expect(tree.stats.parts == syntax.parts.size());
    };

    "stats and trace: a parse counts its parts, its failures and its tentative parses; decisions are traced at debug"_test = [] {
        std::vector<std::string> lines;
        mcxx::base::trace::set_sink([&](mcxx::base::trace::Level, std::string_view category, std::string_view message) {
            if (category == "frontend.syntax") lines.emplace_back(message);
        });
        mcxx::base::trace::Config config;
        config.categories.emplace("frontend.syntax", mcxx::base::trace::Level::debug);
        mcxx::base::trace::configure(config);
        const auto fragment = f::parse_fragment("T * p; f(x);");
        mcxx::base::trace::configure({});
        mcxx::base::trace::set_sink({});
        expect(fragment.tree.stats.parts == 1 && fragment.tree.stats.failed == 0);
        expect(fragment.tree.stats.speculations >= 2);
        expect(std::ranges::any_of(lines, [](const std::string& l) { return l.find("declaration-or-expression") != std::string::npos; }));
        expect(std::ranges::any_of(lines, [](const std::string& l) { return l.find("bodies:") != std::string::npos && l.find("1 parts") != std::string::npos; }));
    };

    return report();
}
