// mcxx.plugins.flow over hand-made control flow (the Clang backend's own test checks the flow Clang gives):
// a read that some path reaches indeterminate, a function that flows off its end, a [[noreturn]] one
// that returns -- and the paths that make each one fine.
import std;
import mcxx.testing;
import mcxx.msa;
import mcxx.plugin;
import mcxx.plugins.flow;

namespace msa = mcxx::msa;
namespace fact = mcxx::msa::fact;
namespace flow = mcxx::plugins::flow;
using fact::EventKind;

namespace {

msa::Range at(std::uint32_t line) { return { { line, 0 }, { line, 1 } }; }

fact::Event event(EventKind kind, std::uint32_t line, std::string variable = "x", bool flag = false) {
    fact::Event e { kind, at(line), variable.empty() ? std::string {} : "c:@F@f#@" + variable, variable };
    if (kind == EventKind::declare) e.indeterminate = flag;
    if (kind == EventKind::call) e.noreturn = flag;
    return e;
}

// Blocks in order, each with its events and successors; the last block is the exit, the first the entry.
fact::Flow function(std::vector<std::pair<std::vector<fact::Event>, std::vector<std::uint32_t>>> blocks, bool returns_value = false,
                    bool noreturn = false) {
    fact::Flow f;
    f.entity = "c:@F@f#";
    f.function = "f";
    f.returns_value = returns_value;
    f.noreturn = noreturn;
    f.end = at(99);
    for (auto& [events, successors] : blocks) f.blocks.push_back({ std::move(events), std::move(successors) });
    f.entry = 0;
    f.exit = static_cast<std::uint32_t>(f.blocks.size() - 1);
    return f;
}

std::vector<mcxx::plugin::Finding> check(fact::Flow f) {
    fact::Facts facts;
    facts.collected = fact::Kinds::control_flow;
    facts.control_flow.push_back(std::move(f));
    std::vector<mcxx::plugin::Finding> out;
    flow::Rules {}.check({ "a.cpp", "", facts }, out);
    return out;
}

} // namespace

int main() {
    using namespace mcxx::testing;

    "a read on the path where nothing wrote the local is reported; the path that wrote it is not"_test = [] {
        // int x; if (c) x = 1; return x;   -- the else edge reaches the read with x indeterminate
        const auto f = function({ { { event(EventKind::declare, 1, "x", true), event(EventKind::read, 2, "c") }, { 1, 2 } },
                                  { { event(EventKind::write, 3) }, { 2 } },
                                  { { event(EventKind::read, 4), event(EventKind::return_, 4, "") }, { 3 } },
                                  { {}, {} } });
        const auto reads = flow::uninitialized_reads(f);
        expect(reads.size() == 1 && reads[0].variable == "x" && reads[0].read.begin.line == 4 && reads[0].declared.begin.line == 1)
            << std::format("{} reads", reads.size());
        const auto findings = check(f);
        expect(findings.size() == 1 && findings[0].feature == flow::UNINITIALIZED_READ) << std::format("{} findings", findings.size());
    };

    "written on every path, initialized, or its address taken: no finding"_test = [] {
        const auto both = function({ { { event(EventKind::declare, 1, "x", true) }, { 1, 2 } },
                                     { { event(EventKind::write, 2) }, { 3 } },
                                     { { event(EventKind::write, 3) }, { 3 } },
                                     { { event(EventKind::read, 4) }, { 4 } },
                                     { {}, {} } });
        expect(flow::uninitialized_reads(both).empty());
        const auto initialized = function({ { { event(EventKind::declare, 1, "x", false), event(EventKind::read, 2) }, { 1 } }, { {}, {} } });
        expect(flow::uninitialized_reads(initialized).empty());
        const auto escaped = function({ { { event(EventKind::declare, 1, "x", true), event(EventKind::address, 2), event(EventKind::read, 3) }, { 1 } },
                                        { {}, {} } });
        expect(flow::uninitialized_reads(escaped).empty());
    };

    "a loop: the read at the top of the body sees the first iteration's indeterminate value"_test = [] {
        // int x; for (;;) { use(x); x = 1; }  -- block 1 is the body, its own successor
        const auto f = function({ { { event(EventKind::declare, 1, "x", true) }, { 1 } },
                                  { { event(EventKind::read, 2), event(EventKind::write, 3) }, { 1 } },
                                  { {}, {} } });
        const auto reads = flow::uninitialized_reads(f);
        expect(reads.size() == 1 && reads[0].read.begin.line == 2);
    };

    "a function returning a value whose end a path reaches: missing-return at its closing brace"_test = [] {
        // int f(bool c) { if (c) return 1; }
        const auto f = function({ { { event(EventKind::read, 1, "c") }, { 1, 2 } },
                                  { { event(EventKind::return_, 2, "") }, { 2 } },
                                  { {}, {} } },
                                true);
        const auto findings = check(f);
        expect(findings.size() == 1 && findings[0].feature == flow::MISSING_RETURN && findings[0].range.begin.line == 99);
    };

    "every path returns, throws or calls what does not return: no missing-return"_test = [] {
        const auto f = function({ { { event(EventKind::read, 1, "c") }, { 1, 2 } },
                                  { { event(EventKind::return_, 2, ""), event(EventKind::scope_end, 2, "y") }, { 4 } },
                                  { { event(EventKind::read, 3, "d") }, { 3, 5 } },
                                  { { event(EventKind::throw_, 4, "") }, { 4 } },
                                  { {}, {} },
                                  { { event(EventKind::call, 5, "", true) }, { 4 } } },
                                true);
        // the exit is block 4 here: rebuild with it last
        auto g = f;
        g.exit = 4;
        expect(check(g).empty()) << std::format("{} findings", check(g).size());
    };

    "a [[noreturn]] function: a return statement and a reachable end are each reported"_test = [] {
        const auto f = function({ { { event(EventKind::read, 1, "c") }, { 1, 2 } },
                                  { { event(EventKind::return_, 2, "") }, { 3 } },
                                  { { event(EventKind::call, 3, "") }, { 3 } },
                                  { {}, {} } },
                                false, true);
        const auto findings = check(f);
        expect(findings.size() == 2 && std::ranges::all_of(findings, [](const auto& x) { return x.feature == flow::NORETURN_RETURNS; }))
            << std::format("{} findings", findings.size());
    };

    "a function whose flow is not known (a template's pattern) is not judged"_test = [] {
        auto f = function({ { { event(EventKind::read, 1) }, { 1 } }, { {}, {} } }, true);
        f.known = false;
        expect(check(f).empty());
    };

    "the provider's features: the flow layer, pitfalls, denied by profile safe"_test = [] {
        const flow::Rules rules;
        expect(rules.features().size() == 3);
        for (const auto& feature : rules.features()) {
            expect(feature.layer == "flow" && feature.category == mcxx::plugin::Category::pitfall && feature.needs == fact::Kinds::control_flow);
            expect(std::ranges::find(feature.profiles, std::pair<std::string, mcxx::plugin::Level> { "safe", mcxx::plugin::Level::deny }) != feature.profiles.end());
        }
    };

    return report();
}
