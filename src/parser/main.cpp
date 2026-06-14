#include "Grammar.hpp"
#include "SLRTable.hpp"
#include "Token.hpp"
#include "Parser.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

// main.cpp
//
// Driver: build the grammar -> construct the SLR tables -> read the lexer's
// token file -> run the PDA -> emit the ASCII parse tree + diagnostics to both
// stdout and an output file.
//
// Default output file is "parse_tree.txt".

using namespace parser;

int main(int argc, char** argv) {
    // Build grammar + tables once. This is the runtime SLR construction the
    // report describes (grammar = single source of truth).
    Grammar  grammar;
    SLRTable table(grammar);

    if (argc >= 2 && std::string(argv[1]) == "--grammar-info") {
        std::cout << table.dump();
        std::cout << "Terminals: "    << grammar.terminals().size()    << "\n";
        std::cout << "Non-terminals: " << grammar.nonTerminals().size() << "\n";
        std::cout << "Productions: "   << grammar.productions().size()  << "\n";
        return 0;
    }

    if (argc < 2) {
        std::cerr << "Usage: " << argv[0]
                  << " <lexer-tokens-file> [output-file]\n"
                  << "       " << argv[0] << " --grammar-info\n";
        return 1;
    }

    const std::string inPath  = argv[1];
    const std::string outPath = (argc >= 3) ? argv[2] : "parse_tree.txt";

    TokenStream stream;
    if (!stream.loadFromFile(inPath)) {
        std::cerr << "Error: could not open token file '" << inPath << "'\n";
        return 1;
    }
    if (stream.empty()) {
        std::cerr << "Error: no tokens parsed from '" << inPath << "'\n";
        return 1;
    }

    Parser parser(grammar, table);
    ParseResult result = parser.parse(stream);

    std::ofstream out(outPath);
    if (!out) {
        std::cerr << "Warning: could not open output file '" << outPath
                  << "'; writing to stdout only.\n";
    }

    // Header line describing the automaton, then the full report.
    std::ostringstream header;
    header << "Parser built from grammar: "
           << grammar.productions().size() << " productions, "
           << table.numStates() << " SLR states, "
           << table.conflicts().size() << " resolved conflict(s).\n\n";

    {
        std::ostringstream body;
        Parser::writeReport(body, result);

        // emit to stdout + file
        std::cout << header.str();
        std::cout << body.str();
        if (out) {
            out << header.str();
            out << body.str();
        }
    }

    return result.accepted ? 0 : 2;
}