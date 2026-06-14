#ifndef GRAMMAR_HPP
#define GRAMMAR_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

// ---------------------------------------------------------------------------
// Grammar.hpp
//
// Encapsulates the Context-Free Grammar (the 4-tuple G = (V, Sigma, R, S)) for
// the extended C-subset object-oriented language.
//
// Design intent (see report): the parser is NOT hand-fed a 144-state ACTION /
// GOTO table. Instead it holds onto the 84 productions only - the single source
// of truth - and the SLR table is constructed from this grammar at start-up.
// This class owns the symbol vocabulary, the productions, and the derived
// NULLABLE / FIRST / FOLLOW sets that the SLR construction depends on.
// ---------------------------------------------------------------------------

namespace parser {

// A grammar symbol is identified by an integer id. Terminals and non-terminals
// share one id space; isTerminal() distinguishes them. The end-marker ($) and
// the augmented start symbol (S') are synthesised internally.
using Symbol = int;

// A single production  A -> X1 X2 ... Xn  (an epsilon production has an empty
// rhs vector).
struct Production {
    Symbol             lhs;      // left-hand side non-terminal
    std::vector<Symbol> rhs;     // right-hand side symbols (empty => epsilon)
    int                index;    // stable rule number, used for "rN" reduce ids
};

class Grammar {
public:
    Grammar();

    // ---- symbol queries -------------------------------------------------
    bool        isTerminal(Symbol s)    const;
    bool        isNonTerminal(Symbol s) const;
    const std::string& name(Symbol s)   const;   // printable name of a symbol

    Symbol      startSymbol()      const { return start_; }       // Program
    Symbol      augmentedStart()   const { return augStart_; }    // S'
    Symbol      endMarker()        const { return endMarker_; }   // $
    Symbol      epsilon()          const { return epsilon_; }     // (marker only)

    // Map a lexer token (type + lexeme) onto a grammar terminal id. Returns
    // -1 if the token does not correspond to any terminal in the language.
    Symbol terminalForToken(const std::string& tokenType,
                            const std::string& lexeme) const;

    // ---- production access ---------------------------------------------
    const std::vector<Production>& productions() const { return productions_; }
    const Production&              production(int i) const { return productions_[i]; }

    // All productions whose lhs == A (used when building item closures).
    const std::vector<int>& productionsFor(Symbol A) const;

    const std::vector<Symbol>& terminals()    const { return terminals_; }
    const std::vector<Symbol>& nonTerminals() const { return nonTerminals_; }
    int  symbolCount() const { return symbolCount_; }

    // ---- derived sets (computed once in the constructor) ----------------
    bool isNullable(Symbol s) const;
    const std::unordered_set<Symbol>& first(Symbol s)  const;
    const std::unordered_set<Symbol>& follow(Symbol s) const;

    // FIRST of a string of symbols (sequence). Adds epsilon-presence via the
    // returned bool (true => the whole sequence is nullable).
    std::unordered_set<Symbol> firstOfSequence(const std::vector<Symbol>& seq,
                                               size_t from) const;

private:
    // construction helpers
    Symbol intern(const std::string& nm, bool terminal);
    void   addProduction(Symbol lhs, std::vector<Symbol> rhs);
    void   buildVocabulary();
    void   buildProductions();
    void   computeNullable();
    void   computeFirst();
    void   computeFollow();

    // symbol tables
    std::vector<std::string>                 names_;
    std::vector<bool>                        terminalFlag_;
    std::unordered_map<std::string, Symbol>  interned_;
    std::vector<Symbol>                      terminals_;
    std::vector<Symbol>                      nonTerminals_;
    int                                      symbolCount_ = 0;

    // distinguished symbols
    Symbol start_      = -1;
    Symbol augStart_   = -1;
    Symbol endMarker_  = -1;
    Symbol epsilon_    = -1;

    // convenience handles for terminal lookup
    std::unordered_map<std::string, Symbol> keywordTerminals_;
    std::unordered_map<std::string, Symbol> punctTerminals_;
    Symbol tIdentifier_ = -1;
    Symbol tNumeric_    = -1;

    // productions
    std::vector<Production>            productions_;
    std::unordered_map<Symbol, std::vector<int>> byLhs_;
    std::vector<int>                   emptyRuleList_;

    // derived sets, indexed by symbol id
    std::vector<char>                              nullable_;
    std::vector<std::unordered_set<Symbol>>        first_;
    std::vector<std::unordered_set<Symbol>>        follow_;
};

} // namespace parser

#endif // GRAMMAR_HPP