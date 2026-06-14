#ifndef SLRTABLE_HPP
#define SLRTABLE_HPP

#include "Grammar.hpp"

#include <map>
#include <vector>
#include <string>

// ---------------------------------------------------------------------------
// SLRTable.hpp
//
// Builds the SLR(1) ACTION and GOTO tables from the Grammar at construction
// time, exactly as described in the report's "Implementation Details": the
// grammar is the single source of truth and the 144-state automaton is derived
// here rather than hand-written.
//
// Strategy: canonical collection of LR(0) item sets, then SLR reduce entries
// keyed on FOLLOW(A). The one intentional shift/reduce conflict (dangling-else)
// is resolved in favour of SHIFT, matching standard C semantics.
// ---------------------------------------------------------------------------

namespace parser {

enum class ActionType { Error, Shift, Reduce, Accept };

struct Action {
    ActionType type = ActionType::Error;
    int        value = 0;   // Shift: target state ; Reduce: production index
};

// An LR(0) item: production `prod`, with the dot before rhs position `dot`.
struct Item {
    int prod;
    int dot;
    bool operator<(const Item& o) const {
        return prod < o.prod || (prod == o.prod && dot < o.dot);
    }
    bool operator==(const Item& o) const {
        return prod == o.prod && dot == o.dot;
    }
};

// Records a resolved conflict so the parser report can describe it.
struct ConflictNote {
    int         state;
    Symbol      onSymbol;
    std::string description;
};

class SLRTable {
public:
    explicit SLRTable(const Grammar& g);

    int numStates() const { return static_cast<int>(states_.size()); }

    // ACTION[state][terminal]
    Action action(int state, Symbol terminal) const;
    // GOTO[state][nonTerminal] -> state, or -1 if undefined
    int    go(int state, Symbol nonTerminal) const;

    const std::vector<ConflictNote>& conflicts() const { return conflicts_; }

    // Diagnostic dump of the canonical collection + tables (ASCII).
    std::string dump() const;

private:
    using ItemSet = std::vector<Item>;   // kept sorted + unique

    void closure(ItemSet& set) const;
    ItemSet gotoSet(const ItemSet& set, Symbol X) const;
    void build();

    static void normalise(ItemSet& set);          // sort + dedup
    std::string itemToString(const Item& it) const;

    const Grammar& g_;

    std::vector<ItemSet>                      states_;
    std::map<std::string, int>                stateIndex_;  // canonical key -> id

    // tables: action_[state] maps terminal -> Action ; goto_[state] maps NT -> state
    std::vector<std::map<Symbol, Action>>     action_;
    std::vector<std::map<Symbol, int>>        goto_;

    std::vector<ConflictNote>                 conflicts_;
};

} // namespace parser

#endif // SLRTABLE_HPP