// mcxx.plugin:flow -- analysis passes over MC3 control flow (MC4 0.5.0, the analysis-pass extension point):
// what a rule on MC1's flow layer needs besides the facts (msa::fact::Flow, Kinds::control_flow) --
// which blocks run, each block's predecessors, the event a path ends with, and a forward data-flow
// solver that iterates to a fixed point. Written against MSA only: the same pass runs on the flow
// Clang's CFG gives today and on the one MC++'s own front end gives later.
//
//   const auto in = plugin::flow::forward<State>(f, State {}, transfer, join);   // the state at each block's entry
export module mcxx.plugin:flow;

import std;
import mcxx.msa;

export namespace mcxx::plugin::flow {

namespace fact = msa::fact;

// Whether each block can run: reachable from the entry along the flow's edges.
std::vector<bool> reachable(const fact::Flow& f) {
    std::vector<bool> seen(f.blocks.size(), false);
    if (f.blocks.empty()) return seen;
    std::vector<std::uint32_t> stack { f.entry };
    seen[f.entry] = true;
    while (!stack.empty()) {
        const std::uint32_t b { stack.back() };
        stack.pop_back();
        for (const std::uint32_t next : f.blocks[b].successors)
            if (!seen[next]) {
                seen[next] = true;
                stack.push_back(next);
            }
    }
    return seen;
}

// Each block's predecessors.
std::vector<std::vector<std::uint32_t>> predecessors(const fact::Flow& f) {
    std::vector<std::vector<std::uint32_t>> out(f.blocks.size());
    for (std::uint32_t b { 0 }; b < f.blocks.size(); ++b)
        for (const std::uint32_t next : f.blocks[b].successors) out[next].push_back(b);
    return out;
}

// The event a path through the block ends with, lifetimes' ends aside (they follow a return); null
// for a block with none.
const fact::Event* last_event(const fact::Block& block) {
    for (auto it = block.events.rbegin(); it != block.events.rend(); ++it)
        if (it->kind != fact::EventKind::scope_end) return &*it;
    return nullptr;
}

// Whether a path that leaves the block for the exit ends there by itself: a return, a throw, or a call
// that does not return. Otherwise it flows off the function's end.
bool ends_by_itself(const fact::Block& block) {
    const fact::Event* last { last_event(block) };
    return last != nullptr && (last->kind == fact::EventKind::return_ || last->kind == fact::EventKind::throw_ ||
                               (last->kind == fact::EventKind::call && last->noreturn));
}

// Forward data flow: the state at each reachable block's entry. `entry` is the entry block's; a block's
// is the join of its predecessors' states after them. transfer(block, state) gives the state after a
// block from the one before it; join(into, from) merges `from` into `into` and says whether it changed.
// Iterates to a fixed point (a join that only grows ends: the lattice is finite). Unreachable blocks
// keep a default State.
template <class State, class Transfer, class Join>
std::vector<State> forward(const fact::Flow& f, State entry, Transfer transfer, Join join) {
    std::vector<State> in(f.blocks.size());
    if (f.blocks.empty()) return in;
    std::vector<bool> has(f.blocks.size(), false);
    in[f.entry] = std::move(entry);
    has[f.entry] = true;
    std::deque<std::uint32_t> work { f.entry };
    std::vector<bool> queued(f.blocks.size(), false);
    queued[f.entry] = true;
    while (!work.empty()) {
        const std::uint32_t b { work.front() };
        work.pop_front();
        queued[b] = false;
        const State out { transfer(b, std::as_const(in[b])) };
        for (const std::uint32_t next : f.blocks[b].successors) {
            bool changed { false };
            if (!has[next]) {
                in[next] = out;
                has[next] = true;
                changed = true;
            } else
                changed = join(in[next], out);
            if (changed && !queued[next]) {
                queued[next] = true;
                work.push_back(next);
            }
        }
    }
    return in;
}

} // namespace mcxx::plugin::flow
