#ifndef TOKEN_HPP
#define TOKEN_HPP

#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// Token.hpp
//
// Token management: a Token mirrors one line of the lexer's output, and
// TokenStream parses the lexer's text file into a vector of Tokens, appending
// the synthetic end-marker ($).
//
// Expected lexer line format (Assessment 1 output):
//   Lexeme: <lexeme>, Token Type: <type>, Row: <r>, Column: <c>
// Everything after a line of "===..." (the summary block) is ignored.
// ---------------------------------------------------------------------------

namespace parser {

    struct Token {
        std::string lexeme;
        std::string type;     // Keyword | Identifier | Numeric Literal | Operator | Delimiter | Invalid
        int         row = -1;
        int         column = -1;
        bool        endMarker = false;
    };

    class TokenStream {
    public:
        // Load tokens from a lexer output file. Returns false on I/O failure.
        bool loadFromFile(const std::string& path);

        // Load tokens from already-read text (used for testing).
        void loadFromText(const std::string& text);

        const std::vector<Token>& tokens() const { return tokens_; }
        bool empty() const { return tokens_.empty(); }

    private:
        static bool parseLine(const std::string& line, Token& out);
        void appendEndMarker();

        std::vector<Token> tokens_;
    };

} // namespace parser

#endif // TOKEN_HPP