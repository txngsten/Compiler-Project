#include "SLRTable.hpp"

#include <algorithm>
#include <sstream>

namespace parser {

SLRTable::SLRTable(const Grammar& g) : g_(g) {
    build();
}

void SLRTable::normalise(ItemSet& set) {
    std::sort(set.begin(), set.end());
    set.erase(std::unique(set.begin(), set.end()), set.end());
}

// CLOSURE(I): for every item A -> alpha . B beta, add all B -> . gamma items.
void SLRTable::closure(ItemSet& set) const {
    normalise(set);
    bool changed = true;
    while (changed) {
        changed = false;
        ItemSet additions;
        for (const Item& it : set) {
            const Production& p = g_.production(it.prod);
            if (it.dot >= static_cast<int>(p.rhs.size())) continue;
            Symbol B = p.rhs[it.dot];
            if (!g_.isNonTerminal(B)) continue;
            for (int rule : g_.productionsFor(B)) {
                Item ni{ rule, 0 };
                if (!std::binary_search(set.begin(), set.end(), ni))
                    additions.push_back(ni);
            }
        }
        if (!additions.empty()) {
            set.insert(set.end(), additions.begin(), additions.end());
            size_t before = set.size();
            normalise(set);
            if (set.size() != before || !additions.empty()) changed = true;
            // recompute changed precisely: if nothing new survived dedup, stop
            changed = false;
            for (const Item& a : additions)
                if (std::binary_search(set.begin(), set.end(), a)) { changed = true; break; }
        }
    }
}

// GOTO(I, X): advance the dot over X for every matching item, then close.
SLRTable::ItemSet SLRTable::gotoSet(const ItemSet& set, Symbol X) const {
    ItemSet moved;
    for (const Item& it : set) {
        const Production& p = g_.production(it.prod);
        if (it.dot < static_cast<int>(p.rhs.size()) && p.rhs[it.dot] == X)
            moved.push_back(Item{ it.prod, it.dot + 1 });
    }
    if (!moved.empty()) closure(moved);
    return moved;
}

static std::string keyOf(const std::vector<Item>& set) {
    std::string k;
    k.reserve(set.size() * 6);
    for (const Item& it : set) {
        k += std::to_string(it.prod);
        k += ':';
        k += std::to_string(it.dot);
        k += '|';
    }
    return k;
}

void SLRTable::build() {
    // initial state: closure of  S' -> . Program
    int augRule = -1;
    for (const auto& p : g_.productions())
        if (p.lhs == g_.augmentedStart()) { augRule = p.index; break; }

    ItemSet start{ Item{ augRule, 0 } };
    closure(start);

    states_.push_back(start);
    stateIndex_[keyOf(start)] = 0;

    // breadth-first construction of the canonical collection
    for (size_t i = 0; i < states_.size(); ++i) {
        // gather symbols appearing immediately after a dot
        std::vector<Symbol> symbols;
        {
            std::vector<char> seen(g_.symbolCount(), 0);
            for (const Item& it : states_[i]) {
                const Production& p = g_.production(it.prod);
                if (it.dot < static_cast<int>(p.rhs.size())) {
                    Symbol X = p.rhs[it.dot];
                    if (!seen[X]) { seen[X] = 1; symbols.push_back(X); }
                }
            }
        }
        for (Symbol X : symbols) {
            ItemSet g = gotoSet(states_[i], X);
            if (g.empty()) continue;
            std::string key = keyOf(g);
            int target;
            auto it = stateIndex_.find(key);
            if (it == stateIndex_.end()) {
                target = static_cast<int>(states_.size());
                states_.push_back(g);
                stateIndex_[key] = target;
            } else {
                target = it->second;
            }
            if (g_.isTerminal(X)) {
                // shift will be recorded in the ACTION pass below
            }
        }
    }

    // size the tables
    action_.assign(states_.size(), {});
    goto_.assign(states_.size(), {});

    // fill ACTION / GOTO
    for (size_t i = 0; i < states_.size(); ++i) {
        // transitions
        std::vector<char> seen(g_.symbolCount(), 0);
        for (const Item& it : states_[i]) {
            const Production& p = g_.production(it.prod);
            if (it.dot < static_cast<int>(p.rhs.size())) {
                Symbol X = p.rhs[it.dot];
                if (seen[X]) continue;
                seen[X] = 1;
                ItemSet g = gotoSet(states_[i], X);
                if (g.empty()) continue;
                int target = stateIndex_[keyOf(g)];
                if (g_.isTerminal(X)) {
                    action_[i][X] = Action{ ActionType::Shift, target };
                } else {
                    goto_[i][X] = target;
                }
            }
        }
        // reductions and accept
        for (const Item& it : states_[i]) {
            const Production& p = g_.production(it.prod);
            if (it.dot != static_cast<int>(p.rhs.size())) continue;  // not complete
            if (p.lhs == g_.augmentedStart()) {
                // S' -> Program .   => accept on $
                action_[i][g_.endMarker()] = Action{ ActionType::Accept, 0 };
                continue;
            }
            // SLR reduce on every terminal in FOLLOW(lhs)
            for (Symbol a : g_.follow(p.lhs)) {
                auto existing = action_[i].find(a);
                if (existing != action_[i].end() &&
                    existing->second.type == ActionType::Shift) {
                    // shift/reduce conflict -> resolve in favour of SHIFT
                    // (this is the intentional dangling-else resolution)
                    conflicts_.push_back(ConflictNote{
                        static_cast<int>(i), a,
                        "shift/reduce on '" + g_.name(a) +
                        "' resolved as SHIFT (rule r" + std::to_string(p.index) +
                        " discarded); dangling-else / nearest-match" });
                    continue; // keep the shift
                }
                if (existing != action_[i].end() &&
                    existing->second.type == ActionType::Reduce &&
                    existing->second.value != p.index) {
                    // reduce/reduce conflict -> keep lower-numbered rule
                    int keep = std::min(existing->second.value, p.index);
                    conflicts_.push_back(ConflictNote{
                        static_cast<int>(i), a,
                        "reduce/reduce on '" + g_.name(a) +
                        "' resolved to lower rule r" + std::to_string(keep) });
                    action_[i][a] = Action{ ActionType::Reduce, keep };
                    continue;
                }
                action_[i][a] = Action{ ActionType::Reduce, p.index };
            }
        }
    }
}

Action SLRTable::action(int state, Symbol terminal) const {
    auto& row = action_[state];
    auto it = row.find(terminal);
    if (it == row.end()) return Action{};   // Error
    return it->second;
}

int SLRTable::go(int state, Symbol nonTerminal) const {
    auto& row = goto_[state];
    auto it = row.find(nonTerminal);
    if (it == row.end()) return -1;
    return it->second;
}

std::string SLRTable::itemToString(const Item& it) const {
    const Production& p = g_.production(it.prod);
    std::ostringstream os;
    os << g_.name(p.lhs) << " -> ";
    for (int i = 0; i <= static_cast<int>(p.rhs.size()); ++i) {
        if (i == it.dot) os << ". ";
        if (i < static_cast<int>(p.rhs.size())) os << g_.name(p.rhs[i]) << ' ';
    }
    if (p.rhs.empty()) os << "(epsilon) ";
    return os.str();
}

std::string SLRTable::dump() const {
    std::ostringstream os;
    os << "SLR automaton: " << states_.size() << " states, "
       << g_.productions().size() << " productions.\n";
    os << "Resolved conflicts: " << conflicts_.size() << "\n";
    for (const auto& c : conflicts_)
        os << "  state " << c.state << ": " << c.description << "\n";
    return os.str();
}

} // namespace parser