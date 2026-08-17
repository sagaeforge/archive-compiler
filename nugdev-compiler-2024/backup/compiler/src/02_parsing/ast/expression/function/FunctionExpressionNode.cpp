#include "FunctionExpressionNode.h"

namespace nugdev::compiler::ast::expression {

FunctionExpressionNode::FunctionExpressionNode(
    const tokenize::Token &token,
    const std::vector<std::tuple<std::shared_ptr<Expression>, std::shared_ptr<Expression>, std::shared_ptr<Expression>>> &parameters,
    std::shared_ptr<Statement> body)
    : m_token(token), m_parameters(parameters), m_body(body) {}

const tokenize::Token &FunctionExpressionNode::get_token() const { return m_token; }

const std::vector<std::tuple<std::shared_ptr<Expression>, std::shared_ptr<Expression>, std::shared_ptr<Expression>>> &
FunctionExpressionNode::get_parameters() const {
    return m_parameters;
}

std::shared_ptr<Statement> FunctionExpressionNode::get_body() const { return m_body; }

} // namespace nugdev::compiler::ast::expression
