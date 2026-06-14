#ifndef PARSETREE_HPP
#define PARSETREE_HPP

#include <string>
#include <vector>
#include <memory>
#include <ostream>
#include <algorithm>

// ---------------------------------------------------------------------------
// ParseTree.hpp
//
// A node in the parse / syntax tree produced by the shift-reduce parser.
// Leaf nodes carry an actual token (terminal); internal nodes carry the
// non-terminal name and the children gathered during a reduce.
//
// The tree renders as ASCII to any std::ostream (file and stdout both use this).
// ---------------------------------------------------------------------------

namespace parser {

class ParseNode {
public:
    using Ptr = std::shared_ptr<ParseNode>;

    // Terminal (leaf) node.
    static Ptr makeLeaf(const std::string& symbol,
                        const std::string& lexeme,
                        int row, int col) {
        auto n = std::make_shared<ParseNode>();
        n->symbol_ = symbol;
        n->lexeme_ = lexeme;
        n->row_ = row;
        n->col_ = col;
        n->leaf_ = true;
        return n;
    }

    // Non-terminal (internal) node.
    static Ptr makeInternal(const std::string& symbol) {
        auto n = std::make_shared<ParseNode>();
        n->symbol_ = symbol;
        n->leaf_ = false;
        return n;
    }

    // Marker for tokens dropped by panic-mode recovery, kept in the tree so the
    // report can show where data was discarded.
    static Ptr makeError(const std::string& detail) {
        auto n = std::make_shared<ParseNode>();
        n->symbol_ = "<error>";
        n->lexeme_ = detail;
        n->leaf_ = true;
        n->error_ = true;
        return n;
    }

    void addChild(const Ptr& c) { children_.push_back(c); }
    // Children are gathered in reverse (popped off the stack); fix order.
    void reverseChildren() {
        std::reverse(children_.begin(), children_.end());
    }

    const std::string& symbol() const { return symbol_; }
    bool isLeaf()  const { return leaf_; }
    bool isError() const { return error_; }
    const std::vector<Ptr>& children() const { return children_; }

    // ASCII tree rendering (box-drawing kept to plain ASCII characters).
    void print(std::ostream& os) const { print(os, "", true); }

private:
    void print(std::ostream& os, const std::string& prefix, bool last) const;

    std::string         symbol_;
    std::string         lexeme_;
    int                 row_ = -1;
    int                 col_ = -1;
    bool                leaf_  = false;
    bool                error_ = false;
    std::vector<Ptr>    children_;
};

} // namespace parser

#endif // PARSETREE_HPP