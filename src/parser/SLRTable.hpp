#ifndef SLRTABLE_HPP
#define SLRTABLE_HPP

#include "Grammar.hpp"

#include <map>
#include <vector>
#include <string>

// SLRTable.hpp
//
// Builds the SLR ACTION and GOTO tables from the Grammar at construction time.

namespace parser {

    enum class ActionType { Error, Shift, Reduce, Accept };

    struct Action {
        ActionType type = ActionType::Error;
        int value = 0;
    };

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
        int state;
        Symbol onSymbol;
        std::string description;
    };

    class SLRTable {
    public:
        explicit SLRTable(const Grammar& g);

        int numStates() const { return static_cast<int>(states_.size()); }

        // ACTION[state][terminal]
        Action action(int state, Symbol terminal) const;
        // GOTO[state][nonTerminal] -> state, or -1 if undefined
        int go(int state, Symbol nonTerminal) const;

        const std::vector<ConflictNote>& conflicts() const { return conflicts_; }

        // Diagnostic dump of the canonical collection + tables.
        std::string dump() const;

    private:
        using ItemSet = std::vector<Item>;  // kept sorted + unique

        void closure(ItemSet& set) const;
        ItemSet gotoSet(const ItemSet& set, Symbol X) const;
        void build();

        static void normalise(ItemSet& set);    // sort + dedup
        std::string itemToString(const Item& it) const;

        const Grammar& g_;

        std::vector<ItemSet> states_;
        std::map<std::string, int> stateIndex_;

        std::vector<std::map<Symbol, Action>> action_;
        std::vector<std::map<Symbol, int>> goto_;

        std::vector<ConflictNote> conflicts_;
    };

} // namespace parser

#endif // SLRTABLE_HPP