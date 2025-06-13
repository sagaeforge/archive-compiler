#include "ast.hpp"

namespace ebnf {

ASTNode::ASTNode(NodeType type, const std::string &value)
    : type_(type), value_(value) {}

} // namespace ebnf