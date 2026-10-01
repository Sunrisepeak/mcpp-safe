// mcxx.plugins.flow: undefined-behaviour sources only a function's control flow shows (MC1's flow layer),
// decided over MC3 control-flow facts with the SDK's analysis passes. Each joins profile `safe`, whose
// features are undefined behaviour's sources; elsewhere they are allowed unless a configuration says.
//
//   uninitialized-read  a local of scalar type (or a class a trivial default constructor leaves so) read on
//                       some path before anything writes it: reading an indeterminate value
//   missing-return      a function returning a value whose closing brace a path reaches
//   noreturn-returns    a [[noreturn]] function that can return, by a return statement or its end
//
// A function whose flow is not known (a template's pattern) is not judged here.
export module mcxx.plugins.flow;

import std;
import mcxx.msa;
import mcxx.plugin;

export namespace mcxx::plugins::flow {

inline constexpr std::string_view UNINITIALIZED_READ { "uninitialized-read" };
inline constexpr std::string_view MISSING_RETURN { "missing-return" };
inline constexpr std::string_view NORETURN_RETURNS { "noreturn-returns" };

class Rules final : public plugin::Rule {
public:
    std::string_view name() const override { return "mcxx.plugins.flow"; }
    std::span<const plugin::Feature> features() const override { return features_; }
    void check(const plugin::Context& context, std::vector<plugin::Finding>& out) const override;

private:
    static plugin::Feature feature(std::string_view id, std::string standard, std::string summary, std::string fix) {
        return { .id = std::string { id }, .category = plugin::Category::pitfall, .standard = std::move(standard), .layer = "flow",
                 .summary = std::move(summary), .fix = std::move(fix), .profiles = { { "safe", plugin::Level::deny } },
                 .needs = msa::fact::Kinds::control_flow };
    }
    std::vector<plugin::Feature> features_ {
        feature(UNINITIALIZED_READ, "[basic.indet]", "a local read before anything writes it", "initialize it where it is declared"),
        feature(MISSING_RETURN, "[stmt.return]", "a function returning a value whose end can be reached",
                "return a value on every path, or end the path with a throw or a [[noreturn]] call"),
        feature(NORETURN_RETURNS, "[dcl.attr.noreturn]", "a [[noreturn]] function that can return",
                "end every path with a throw or a call that does not return, or drop [[noreturn]]"),
    };
};

// The locals that may still be indeterminate when each read of them happens, by a forward analysis:
// one finding per read (MC3 ranges), each with where the local was declared.
struct UninitializedRead {
    msa::Range read;
    std::string variable;
    msa::Range declared;
};
std::vector<UninitializedRead> uninitialized_reads(const msa::fact::Flow& f);

} // namespace mcxx::plugins::flow

namespace mcxx::plugins::flow {

namespace fact = msa::fact;

std::vector<UninitializedRead> uninitialized_reads(const fact::Flow& f) {
    using State = std::set<std::string>;   // the locals (by entity) that may be indeterminate
    std::map<std::string, msa::Range> declared;
    const auto step = [&](const fact::Event& e, State& state, std::vector<UninitializedRead>* out) {
        switch (e.kind) {
        case fact::EventKind::declare:
            declared[e.entity] = e.range;
            if (e.indeterminate) state.insert(e.entity);
            else state.erase(e.entity);
            break;
        case fact::EventKind::read:
            if (out != nullptr && state.contains(e.entity)) out->push_back({ e.range, e.name, declared[e.entity] });
            break;
        case fact::EventKind::write:
        case fact::EventKind::address:
        case fact::EventKind::scope_end:
            state.erase(e.entity);
            break;
        default:
            break;
        }
    };
    const auto transfer = [&](std::uint32_t b, const State& in) {
        State state { in };
        for (const auto& e : f.blocks[b].events) step(e, state, nullptr);
        return state;
    };
    const auto join = [](State& into, const State& from) {
        const std::size_t before { into.size() };
        into.insert(from.begin(), from.end());
        return into.size() != before;
    };
    const auto in = plugin::flow::forward<State>(f, State {}, transfer, join);
    const auto reachable = plugin::flow::reachable(f);
    std::vector<UninitializedRead> out;
    for (std::uint32_t b { 0 }; b < f.blocks.size(); ++b) {
        if (!reachable[b]) continue;
        State state { in[b] };
        for (const auto& e : f.blocks[b].events) step(e, state, &out);
    }
    std::ranges::sort(out, {}, [](const UninitializedRead& r) { return std::pair { r.read.begin.line, r.read.begin.column }; });
    const auto [first, last] = std::ranges::unique(out, {}, [](const UninitializedRead& r) { return std::pair { r.read.begin.line, r.read.begin.column }; });
    out.erase(first, last);
    return out;
}

void Rules::check(const plugin::Context& context, std::vector<plugin::Finding>& out) const {
    const bool reads { context.wants(UNINITIALIZED_READ) }, returns { context.wants(MISSING_RETURN) }, noreturn { context.wants(NORETURN_RETURNS) };
    for (const auto& f : context.facts.control_flow) {
        if (!f.known || f.blocks.empty()) continue;
        if (reads)
            for (const auto& r : uninitialized_reads(f))
                out.push_back({ std::string { UNINITIALIZED_READ }, r.read,
                                std::format("`{}` may be read here before anything writes it (declared at line {} without a value)", r.variable,
                                            r.declared.begin.line + 1),
                                f.container });
        if (!(returns && f.returns_value) && !(noreturn && f.noreturn)) continue;
        const auto reachable = plugin::flow::reachable(f);
        bool off_the_end { false };
        for (std::uint32_t b { 0 }; b < f.blocks.size(); ++b) {
            if (!reachable[b] || b == f.exit || std::ranges::find(f.blocks[b].successors, f.exit) == f.blocks[b].successors.end()) continue;
            const fact::Event* last { plugin::flow::last_event(f.blocks[b]) };
            if (noreturn && f.noreturn && last != nullptr && last->kind == fact::EventKind::return_)
                out.push_back({ std::string { NORETURN_RETURNS }, last->range, std::format("`{}` is [[noreturn]] but returns here", f.function), f.container });
            else if (!plugin::flow::ends_by_itself(f.blocks[b]))
                off_the_end = true;
        }
        if (!off_the_end) continue;
        if (noreturn && f.noreturn)
            out.push_back({ std::string { NORETURN_RETURNS }, f.end, std::format("`{}` is [[noreturn]] but can reach its end", f.function), f.container });
        else if (returns && f.returns_value)
            out.push_back({ std::string { MISSING_RETURN }, f.end, std::format("`{}` can reach its end without returning a value", f.function), f.container });
    }
}

plugin::Registration<Rules> registration;

} // namespace mcxx::plugins::flow
