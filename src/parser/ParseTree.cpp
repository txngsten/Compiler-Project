#include "ParseTree.hpp"

namespace parser {

    void ParseNode::print(std::ostream& os, const std::string& prefix, bool last) const {
        os << prefix;
        os << (last ? "`-- " : "|-- ");

        if (error_) {
            os << "<error: " << lexeme_ << ">\n";
            return;
        }
        if (leaf_) {
            // terminal: show the symbol and the concrete lexeme + position
            os << symbol_;
            if (!lexeme_.empty()) {
                os << " \"" << lexeme_ << "\"";
                if (row_ >= 0) os << "  (" << row_ << ":" << col_ << ")";
            }
            os << "\n";
            return;
        }

        os << symbol_ << "\n";
        std::string childPrefix = prefix + (last ? "    " : "|   ");
        for (size_t i = 0; i < children_.size(); ++i) {
            children_[i]->print(os, childPrefix, i + 1 == children_.size());
        }
    }

} // namespace parser