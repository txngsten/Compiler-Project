#include "Token.hpp"

#include <fstream>
#include <sstream>

namespace parser {

// Trim leading/trailing ASCII whitespace.
static std::string trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

// Parse one line of the form:
//   Lexeme: <lexeme>, Token Type: <type>, Row: <r>, Column: <c>
// The lexeme itself may be a comma, so we locate the fixed field markers
// rather than naively splitting on commas.
bool TokenStream::parseLine(const std::string& line, Token& out) {
    const std::string kLex  = "Lexeme: ";
    const std::string kType = ", Token Type: ";
    const std::string kRow  = ", Row: ";
    const std::string kCol  = ", Column: ";

    size_t pLex = line.find(kLex);
    if (pLex == std::string::npos) return false;
    size_t pType = line.find(kType, pLex);
    if (pType == std::string::npos) return false;
    size_t pRow = line.find(kRow, pType);
    if (pRow == std::string::npos) return false;
    size_t pCol = line.find(kCol, pRow);
    if (pCol == std::string::npos) return false;

    size_t lexStart = pLex + kLex.size();
    out.lexeme = line.substr(lexStart, pType - lexStart);

    size_t typeStart = pType + kType.size();
    out.type = trim(line.substr(typeStart, pRow - typeStart));

    size_t rowStart = pRow + kRow.size();
    std::string rowStr = trim(line.substr(rowStart, pCol - rowStart));

    size_t colStart = pCol + kCol.size();
    std::string colStr = trim(line.substr(colStart));

    try {
        out.row    = std::stoi(rowStr);
        out.column = std::stoi(colStr);
    } catch (...) {
        out.row = out.column = -1;
    }
    out.endMarker = false;
    return true;
}

void TokenStream::appendEndMarker() {
    Token eof;
    eof.lexeme = "$";
    eof.type   = "$";
    eof.endMarker = true;
    // place at the last seen position if available
    if (!tokens_.empty()) {
        eof.row    = tokens_.back().row;
        eof.column = tokens_.back().column + 1;
    } else {
        eof.row = eof.column = 0;
    }
    tokens_.push_back(eof);
}

void TokenStream::loadFromText(const std::string& text) {
    tokens_.clear();
    std::istringstream in(text);
    std::string line;
    while (std::getline(in, line)) {
        std::string t = trim(line);
        if (t.empty()) continue;
        // stop at the summary statistics banner
        if (t.find("Summary Statistics") != std::string::npos) break;
        if (t.rfind("====", 0) == 0) break;
        Token tok;
        if (parseLine(line, tok)) tokens_.push_back(tok);
    }
    appendEndMarker();
}

bool TokenStream::loadFromFile(const std::string& path) {
    std::ifstream f(path);
    if (!f) return false;
    std::ostringstream ss;
    ss << f.rdbuf();
    loadFromText(ss.str());
    return true;
}

} // namespace parser