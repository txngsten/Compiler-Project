#ifndef PARSER_HPP
#define PARSER_HPP

#include "Grammar.hpp"
#include "SLRTable.hpp"
#include "Token.hpp"
#include "ParseTree.hpp"

#include <string>
#include <vector>
#include <ostream>

// ---------------------------------------------------------------------------
// Parser.hpp
//
// The pushdown automaton. Encapsulates the parser state: a state stack and a
// parallel symbol/parse-node stack. Driven by the SLR ACTION/GOTO tables, it
// performs shift / reduce / accept steps and builds the parse tree.
//
// Error handling is graceful: on an ACTION error it reports the offending
// token (lexeme, line, column) and enters panic-mode recovery rather than
// aborting, so a single syntax error does not lose the rest of the parse.
// ---------------------------------------------------------------------------

namespace parser {

    struct SyntaxError {
        std::string lexeme;
        std::string tokenType;
        int         row = -1;
        int         column = -1;
        int         state = -1;
        std::string message;
    };

    struct ParseResult {
        bool                     accepted = false;
        ParseNode::Ptr           tree;          // best-effort tree (may be partial)
        std::vector<SyntaxError> errors;
        int                      tokensConsumed = 0;
        int                      tokensDiscarded = 0;
    };

    class Parser {
    public:
        Parser(const Grammar& g, const SLRTable& table);

        // Run the PDA over a token stream and build the parse tree.
        ParseResult parse(const TokenStream& stream);

        // Convenience: write a full ASCII report (tree, errors, stats) to `os`.
        static void writeReport(std::ostream& os, const ParseResult& r);

    private:
        // Recovery terminals the parser will try to synchronise on (panic mode).
        bool isSyncTerminal(Symbol t) const;

        const Grammar&  g_;
        const SLRTable& table_;
    };

} // namespace parser

#endif // PARSER_HPP