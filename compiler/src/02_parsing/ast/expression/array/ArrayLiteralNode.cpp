#include "ArrayLiteralNode.h"

namespace nugdev::compiler::ast::expression {

ArrayLiteralNode::ArrayLiteralNode(const tokenize::Token &token, const std::vector<std::shared_ptr<Expression>> &elements)
    : m_token(token), m_elements(elements) {}

const tokenize::Token &ArrayLiteralNode::get_token() const { return m_token; }

const std::vector<std::shared_ptr<Expression>> &ArrayLiteralNode::get_elements() const { return m_elements; }

} // namespace nugdev::compiler::ast::expression
