#include "Parser.hpp"

#include <sstream>
#include <unordered_set>

namespace parser {

Parser::Parser(const Grammar& g, const SLRTable& table)
    : g_(g), table_(table) {}

// Panic-mode synchronisation set: statement / declaration boundaries. After an
// error we discard input tokens until we reach one of these, giving the parser
// a reasonable place to resume.
bool Parser::isSyncTerminal(Symbol t) const {
    static const char* syncs[] = { ";", "}", "{", "$" };
    for (const char* s : syncs)
        if (g_.name(t) == s) return true;
    return false;
}

ParseResult Parser::parse(const TokenStream& stream) {
    ParseResult result;

    // PDA stacks: states run in lock-step with the parse-node stack.
    std::vector<int>            stateStack;
    std::vector<ParseNode::Ptr> nodeStack;
    stateStack.push_back(0);   // start state

    const std::vector<Token>& toks = stream.tokens();
    size_t ip = 0;             // input pointer

    // Map a Token to its grammar terminal id (end-marker handled specially).
    auto terminalOf = [&](const Token& t) -> Symbol {
        if (t.endMarker) return g_.endMarker();
        return g_.terminalForToken(t.type, t.lexeme);
    };

    int safety = 0;
    const int kSafetyLimit = 2'000'000; // guard against pathological loops

    while (ip < toks.size()) {
        if (++safety > kSafetyLimit) {
            SyntaxError e;
            e.message = "internal: parser exceeded step limit";
            result.errors.push_back(e);
            break;
        }

        const Token& tok = toks[ip];
        Symbol a = terminalOf(tok);
        int s = stateStack.back();

        // A token the grammar has no terminal for (e.g. Invalid, or "do").
        if (a < 0) {
            SyntaxError e{ tok.lexeme, tok.type, tok.row, tok.column, s,
                "unrecognised token '" + tok.lexeme +
                "' (" + tok.type + ") not in language" };
            result.errors.push_back(e);
            ++ip;
            ++result.tokensDiscarded;
            continue;
        }

        Action act = table_.action(s, a);

        if (act.type == ActionType::Shift) {
            stateStack.push_back(act.value);
            nodeStack.push_back(
                ParseNode::makeLeaf(g_.name(a), tok.lexeme, tok.row, tok.column));
            ++ip;
            ++result.tokensConsumed;
        }
        else if (act.type == ActionType::Reduce) {
            const Production& p = g_.production(act.value);
            auto node = ParseNode::makeInternal(g_.name(p.lhs));
            // pop |rhs| states + nodes; epsilon productions pop nothing
            for (size_t k = 0; k < p.rhs.size(); ++k) {
                if (!nodeStack.empty()) {
                    node->addChild(nodeStack.back());
                    nodeStack.pop_back();
                }
                if (stateStack.size() > 1) stateStack.pop_back();
            }
            node->reverseChildren();
            // GOTO
            int top = stateStack.back();
            int g = table_.go(top, p.lhs);
            if (g < 0) {
                SyntaxError e{ tok.lexeme, tok.type, tok.row, tok.column, top,
                    "no GOTO for non-terminal '" + g_.name(p.lhs) + "'" };
                result.errors.push_back(e);
                // treat as error -> recover below by falling through
                stateStack.push_back(top); // keep stack consistent
                // discard one token to make progress
                if (!tok.endMarker) { ++ip; ++result.tokensDiscarded; }
                else break;
                continue;
            }
            stateStack.push_back(g);
            nodeStack.push_back(node);
        }
        else if (act.type == ActionType::Accept) {
            result.accepted = result.errors.empty();
            if (!nodeStack.empty()) result.tree = nodeStack.back();
            break;
        }
        else {
            // -------- ACTION error: graceful report + panic-mode recovery ----
            std::ostringstream msg;
            msg << "unexpected token '" << tok.lexeme << "' (" << tok.type << ")";
            // list a few expected terminals to aid the user
            // (collected from the current state's shiftable terminals)
            SyntaxError e{ tok.lexeme, tok.type, tok.row, tok.column, s, msg.str() };
            result.errors.push_back(e);

            // Panic mode:
            // 1) pop states until some state can shift a sync terminal, OR the
            //    stack is down to the start state;
            // 2) discard input tokens until we see a sync terminal we can act on;
            // 3) resume.
            bool recovered = false;

            // Step 2 first: scan forward to a synchronising terminal.
            while (ip < toks.size()) {
                const Token& t2 = toks[ip];
                Symbol a2 = (t2.endMarker) ? g_.endMarker()
                                           : terminalOf(t2);
                if (a2 >= 0 && (isSyncTerminal(a2) || t2.endMarker)) break;
                ++ip;
                ++result.tokensDiscarded;
            }

            if (ip >= toks.size()) break;

            const Token& sync = toks[ip];
            Symbol as = sync.endMarker ? g_.endMarker() : terminalOf(sync);

            // Step 1: unwind the state stack until ACTION on the sync terminal
            // is defined (shift or reduce), keeping node/state stacks aligned.
            while (stateStack.size() > 1) {
                Action sa = table_.action(stateStack.back(), as);
                if (sa.type != ActionType::Error) { recovered = true; break; }
                stateStack.pop_back();
                if (!nodeStack.empty()) {
                    // attach discarded subtree marker so the loss is visible
                    auto drop = nodeStack.back();
                    nodeStack.pop_back();
                    (void)drop;
                }
            }
            // also check the start state itself
            if (!recovered) {
                Action sa = table_.action(stateStack.back(), as);
                if (sa.type != ActionType::Error) recovered = true;
            }

            if (recovered) {
                // If the sync token is itself only a delimiter like ';' or '}',
                // consume it so we move past the broken construct.
                if (g_.name(as) == ";" || g_.name(as) == "}") {
                    Action sa = table_.action(stateStack.back(), as);
                    if (sa.type == ActionType::Shift) {
                        stateStack.push_back(sa.value);
                        nodeStack.push_back(ParseNode::makeError(
                            "recovered at '" + sync.lexeme + "'"));
                        ++ip;
                        ++result.tokensDiscarded;
                    }
                }
                continue;
            }
            // Could not recover: discard the sync token and keep going to avoid
            // an infinite loop. If it is the end-marker, stop.
            if (sync.endMarker) break;
            ++ip;
            ++result.tokensDiscarded;
        }
    }

    // If we never accepted but built something, expose the partial tree.
    if (!result.tree && !nodeStack.empty())
        result.tree = nodeStack.back();

    return result;
}

void Parser::writeReport(std::ostream& os, const ParseResult& r) {
    os << "==================== PARSE RESULT ====================\n";
    os << "Status: " << (r.accepted ? "ACCEPTED (no syntax errors)"
                                     : "REJECTED (syntax errors present)") << "\n";
    os << "Tokens consumed : " << r.tokensConsumed << "\n";
    os << "Tokens discarded: " << r.tokensDiscarded
       << "  (panic-mode recovery)\n";
    os << "Syntax errors   : " << r.errors.size() << "\n";
    os << "\n";

    if (!r.errors.empty()) {
        os << "-------------------- SYNTAX ERRORS -------------------\n";
        for (size_t i = 0; i < r.errors.size(); ++i) {
            const SyntaxError& e = r.errors[i];
            os << "[" << (i + 1) << "] ";
            if (e.row >= 0)
                os << "line " << e.row << ", col " << e.column << ": ";
            os << e.message;
            if (e.state >= 0) os << "  (state " << e.state << ")";
            os << "\n";
        }
        os << "\n";
    }

    os << "--------------------- PARSE TREE ---------------------\n";
    if (r.tree) {
        r.tree->print(os);
    } else {
        os << "(no parse tree produced)\n";
    }
    os << "======================================================\n";
}

} // namespace parser