#include "Grammar.hpp"

#include <algorithm>

namespace parser {

// ===========================================================================
// Construction
// ===========================================================================
Grammar::Grammar() {
    buildVocabulary();
    buildProductions();

    // size the derived-set tables now that all symbols are interned
    nullable_.assign(symbolCount_, 0);
    first_.assign(symbolCount_, {});
    follow_.assign(symbolCount_, {});

    computeNullable();
    computeFirst();
    computeFollow();
}

Symbol Grammar::intern(const std::string& nm, bool terminal) {
    auto it = interned_.find(nm);
    if (it != interned_.end()) return it->second;
    Symbol id = symbolCount_++;
    names_.push_back(nm);
    terminalFlag_.push_back(terminal);
    interned_[nm] = id;
    if (terminal) terminals_.push_back(id);
    else          nonTerminals_.push_back(id);
    return id;
}

bool Grammar::isTerminal(Symbol s)    const { return terminalFlag_[s]; }
bool Grammar::isNonTerminal(Symbol s) const { return !terminalFlag_[s]; }
const std::string& Grammar::name(Symbol s) const { return names_[s]; }

// ---------------------------------------------------------------------------
// Vocabulary: terminals (Sigma) and the non-terminals (V) we will reference.
// ---------------------------------------------------------------------------
void Grammar::buildVocabulary() {
    // --- terminals -------------------------------------------------------
    tIdentifier_ = intern("IDENTIFIER", true);
    tNumeric_    = intern("NUMERIC",    true);

    const char* keywords[] = {
        "class", "public", "private", "protected", "new", "delete",
        "int", "char", "if", "else", "while", "for", "return"
    };
    for (const char* k : keywords)
        keywordTerminals_[k] = intern(k, true);

    // punctuation / operator terminals
    const char* puncts[] = { "=", "+", "-", "*", "/", "<", ">",
                             ";", ",", ":", "(", ")", "{", "}", "[", "]" };
    for (const char* p : puncts)
        punctTerminals_[p] = intern(p, true);

    // end-marker
    endMarker_ = intern("$", true);
    // epsilon is only ever a marker in FIRST sets; give it an id but no role
    epsilon_   = intern("(epsilon)", true);

    // --- non-terminals (declared lazily by productions, but intern the
    //     start symbol up front so its id is stable) ----------------------
    start_ = intern("Program", false);
}

// Map a token coming out of the lexer onto a grammar terminal.
Symbol Grammar::terminalForToken(const std::string& tokenType,
                                 const std::string& lexeme) const {
    if (tokenType == "Identifier")       return tIdentifier_;
    if (tokenType == "Numeric Literal")  return tNumeric_;

    if (tokenType == "Keyword") {
        auto it = keywordTerminals_.find(lexeme);
        if (it != keywordTerminals_.end()) return it->second;
        return -1; // keyword not part of the parsing grammar (e.g. "do")
    }
    // Operators and Delimiters are matched by their literal lexeme.
    if (tokenType == "Operator" || tokenType == "Delimiter") {
        auto it = punctTerminals_.find(lexeme);
        if (it != punctTerminals_.end()) return it->second;
        return -1;
    }
    return -1; // Invalid, or anything unrecognised
}

// ---------------------------------------------------------------------------
// Productions (R). These mirror the 84 rules from the design report exactly.
// addProduction interns any new non-terminal names on first use.
// ---------------------------------------------------------------------------
void Grammar::addProduction(Symbol lhs, std::vector<Symbol> rhs) {
    int idx = static_cast<int>(productions_.size());
    productions_.push_back(Production{ lhs, std::move(rhs), idx });
    byLhs_[lhs].push_back(idx);
}

void Grammar::buildProductions() {
    // helper lambdas to fetch / create symbols by name
    auto N = [&](const std::string& nm) { return intern(nm, false); };  // non-term
    auto T = [&](const std::string& nm) { return interned_.at(nm); };   // terminal

    // Augmented start  S' -> Program
    augStart_ = intern("S'", false);

    // --- Program structure ----------------------------------------------
    addProduction(N("Program"), { N("TopLevelList") });

    addProduction(N("TopLevelList"), { N("TopLevelItem") });
    addProduction(N("TopLevelList"), { N("TopLevelItem"), N("TopLevelList") });

    addProduction(N("TopLevelItem"), { N("ClassDeclaration") });
    addProduction(N("TopLevelItem"), { N("FunctionDeclaration") });
    addProduction(N("TopLevelItem"), { N("Statement") });

    // --- Class declarations ---------------------------------------------
    addProduction(N("ClassDeclaration"),
                  { T("class"), T("IDENTIFIER"), T("{"),
                    N("AccessSpecifierSection"), T("}"), T(";") });

    addProduction(N("AccessSpecifierSection"),
                  { N("AccessSpecifier"), T(":"), N("MemberList") });

    addProduction(N("AccessSpecifier"), { T("public") });
    addProduction(N("AccessSpecifier"), { T("private") });
    addProduction(N("AccessSpecifier"), { T("protected") });

    addProduction(N("MemberList"), { N("MemberDeclaration") });
    addProduction(N("MemberList"), { N("MemberDeclaration"), N("MemberList") });

    addProduction(N("MemberDeclaration"), { N("VarDeclaration") });
    addProduction(N("MemberDeclaration"), { N("FunctionDeclaration") });
    addProduction(N("MemberDeclaration"), { N("ClassDeclaration") });

    // --- Types -----------------------------------------------------------
    addProduction(N("Type"), { T("int") });
    addProduction(N("Type"), { T("char") });
    addProduction(N("Type"), { T("IDENTIFIER") });

    // --- Function and variable declarations -----------------------------
    addProduction(N("VarDeclaration"),
                  { N("Type"), T("IDENTIFIER"), T(";") });
    addProduction(N("VarDeclaration"),
                  { N("Type"), T("IDENTIFIER"), T("="), N("Expression"), T(";") });
    addProduction(N("VarDeclaration"),
                  { N("Type"), T("IDENTIFIER"), T("["), T("NUMERIC"), T("]"), T(";") });

    addProduction(N("FunctionDeclaration"),
                  { N("Type"), T("IDENTIFIER"), T("("), N("ParamList"), T(")"), T(";") });
    addProduction(N("FunctionDeclaration"),
                  { N("Type"), T("IDENTIFIER"), T("("), N("ParamList"), T(")"), N("Block") });

    addProduction(N("ParamList"), {});                       // epsilon
    addProduction(N("ParamList"), { N("ParamSeq") });

    addProduction(N("ParamSeq"), { N("Param") });
    addProduction(N("ParamSeq"), { N("Param"), T(","), N("ParamSeq") });

    addProduction(N("Param"), { N("Type"), T("IDENTIFIER") });

    // --- Statements ------------------------------------------------------
    addProduction(N("Block"), { T("{"), N("StatementList"), T("}") });

    addProduction(N("StatementList"), {});                   // epsilon
    addProduction(N("StatementList"), { N("Statement"), N("StatementList") });

    addProduction(N("Statement"), { N("VarDeclaration") });
    addProduction(N("Statement"), { N("ExpressionStatement") });
    addProduction(N("Statement"), { N("IfStatement") });
    addProduction(N("Statement"), { N("WhileStatement") });
    addProduction(N("Statement"), { N("ForStatement") });
    addProduction(N("Statement"), { N("ReturnStatement") });
    addProduction(N("Statement"), { N("DeleteStatement") });
    addProduction(N("Statement"), { N("Block") });

    addProduction(N("ExpressionStatement"), { N("Expression"), T(";") });

    addProduction(N("ReturnStatement"), { T("return"), T(";") });
    addProduction(N("ReturnStatement"), { T("return"), N("Expression"), T(";") });

    addProduction(N("DeleteStatement"), { T("delete"), T("IDENTIFIER"), T(";") });

    // --- Control flow ----------------------------------------------------
    addProduction(N("IfStatement"),
                  { T("if"), T("("), N("Expression"), T(")"), N("Statement") });
    addProduction(N("IfStatement"),
                  { T("if"), T("("), N("Expression"), T(")"), N("Statement"),
                    T("else"), N("Statement") });

    addProduction(N("WhileStatement"),
                  { T("while"), T("("), N("Expression"), T(")"), N("Statement") });

    addProduction(N("ForStatement"),
                  { T("for"), T("("), N("ForInit"), T(";"), N("ForCond"), T(";"),
                    N("ForUpdate"), T(")"), N("Statement") });

    addProduction(N("ForInit"), {});                         // epsilon
    addProduction(N("ForInit"), { N("VarDeclarationNoSemi") });
    addProduction(N("ForInit"), { N("Expression") });

    addProduction(N("ForCond"), {});                         // epsilon
    addProduction(N("ForCond"), { N("Expression") });

    addProduction(N("ForUpdate"), {});                       // epsilon
    addProduction(N("ForUpdate"), { N("Expression") });

    addProduction(N("VarDeclarationNoSemi"), { N("Type"), T("IDENTIFIER") });
    addProduction(N("VarDeclarationNoSemi"),
                  { N("Type"), T("IDENTIFIER"), T("="), N("Expression") });

    // --- Expressions (precedence layered) -------------------------------
    addProduction(N("Expression"), { N("Assignment") });

    addProduction(N("Assignment"),
                  { T("IDENTIFIER"), T("="), N("Assignment") });
    addProduction(N("Assignment"), { N("Comparison") });

    addProduction(N("Comparison"),
                  { N("Comparison"), T("<"), N("Additive") });
    addProduction(N("Comparison"),
                  { N("Comparison"), T(">"), N("Additive") });
    addProduction(N("Comparison"), { N("Additive") });

    addProduction(N("Additive"),
                  { N("Additive"), T("+"), N("Multiplicative") });
    addProduction(N("Additive"),
                  { N("Additive"), T("-"), N("Multiplicative") });
    addProduction(N("Additive"), { N("Multiplicative") });

    addProduction(N("Multiplicative"),
                  { N("Multiplicative"), T("*"), N("Unary") });
    addProduction(N("Multiplicative"),
                  { N("Multiplicative"), T("/"), N("Unary") });
    addProduction(N("Multiplicative"), { N("Unary") });

    addProduction(N("Unary"), { N("NewExpression") });
    addProduction(N("Unary"), { N("Postfix") });

    addProduction(N("NewExpression"), { T("new"), T("IDENTIFIER") });
    addProduction(N("NewExpression"),
                  { T("new"), T("IDENTIFIER"), T("("), N("ArgList"), T(")") });

    addProduction(N("Postfix"),
                  { N("Postfix"), T("("), N("ArgList"), T(")") });
    addProduction(N("Postfix"), { N("Primary") });

    addProduction(N("Primary"), { T("IDENTIFIER") });
    addProduction(N("Primary"), { T("NUMERIC") });
    addProduction(N("Primary"), { T("("), N("Expression"), T(")") });

    addProduction(N("ArgList"), {});                         // epsilon
    addProduction(N("ArgList"), { N("ArgSeq") });

    addProduction(N("ArgSeq"), { N("Expression") });
    addProduction(N("ArgSeq"), { N("Expression"), T(","), N("ArgSeq") });

    // Augmented production added last so its rule index is highest; the SLR
    // builder treats it specially for the accept action.
    addProduction(augStart_, { start_ });
}

const std::vector<int>& Grammar::productionsFor(Symbol A) const {
    auto it = byLhs_.find(A);
    if (it == byLhs_.end()) return emptyRuleList_;
    return it->second;
}

// ===========================================================================
// NULLABLE
// ===========================================================================
void Grammar::computeNullable() {
    bool changed = true;
    while (changed) {
        changed = false;
        for (const auto& p : productions_) {
            if (nullable_[p.lhs]) continue;
            bool allNullable = true;
            for (Symbol s : p.rhs) {
                if (isTerminal(s) || !nullable_[s]) { allNullable = false; break; }
            }
            if (allNullable) { nullable_[p.lhs] = 1; changed = true; }
        }
    }
}

bool Grammar::isNullable(Symbol s) const {
    if (isTerminal(s)) return false;
    return nullable_[s] != 0;
}

// ===========================================================================
// FIRST
// ===========================================================================
void Grammar::computeFirst() {
    // FIRST(terminal) = { terminal }
    for (Symbol t : terminals_) first_[t].insert(t);

    bool changed = true;
    while (changed) {
        changed = false;
        for (const auto& p : productions_) {
            auto& F = first_[p.lhs];
            size_t before = F.size();
            // add FIRST of the rhs prefix, stopping at first non-nullable symbol
            bool prefixNullable = true;
            for (Symbol s : p.rhs) {
                for (Symbol f : first_[s]) {
                    if (f != epsilon_) F.insert(f);
                }
                if (!isNullable(s)) { prefixNullable = false; break; }
            }
            (void)prefixNullable;
            if (F.size() != before) changed = true;
        }
    }
}

const std::unordered_set<Symbol>& Grammar::first(Symbol s) const {
    return first_[s];
}

std::unordered_set<Symbol>
Grammar::firstOfSequence(const std::vector<Symbol>& seq, size_t from) const {
    std::unordered_set<Symbol> result;
    bool allNullable = true;
    for (size_t i = from; i < seq.size(); ++i) {
        Symbol s = seq[i];
        for (Symbol f : first_[s])
            if (f != epsilon_) result.insert(f);
        if (!isNullable(s)) { allNullable = false; break; }
    }
    if (allNullable) result.insert(epsilon_);  // signals nullable suffix
    return result;
}

// ===========================================================================
// FOLLOW
// ===========================================================================
void Grammar::computeFollow() {
    follow_[start_].insert(endMarker_);
    follow_[augStart_].insert(endMarker_);

    bool changed = true;
    while (changed) {
        changed = false;
        for (const auto& p : productions_) {
            for (size_t i = 0; i < p.rhs.size(); ++i) {
                Symbol B = p.rhs[i];
                if (!isNonTerminal(B)) continue;
                size_t before = follow_[B].size();

                // FIRST of the part after B
                bool betaNullable = true;
                for (size_t j = i + 1; j < p.rhs.size(); ++j) {
                    Symbol s = p.rhs[j];
                    for (Symbol f : first_[s])
                        if (f != epsilon_) follow_[B].insert(f);
                    if (!isNullable(s)) { betaNullable = false; break; }
                }
                // if everything after B is nullable, add FOLLOW(lhs)
                if (betaNullable) {
                    for (Symbol f : follow_[p.lhs]) follow_[B].insert(f);
                }
                if (follow_[B].size() != before) changed = true;
            }
        }
    }
}

const std::unordered_set<Symbol>& Grammar::follow(Symbol s) const {
    return follow_[s];
}

} // namespace parser