#include "ReturnStatementNode.h"

namespace nugdev::compiler::ast::statement {

ReturnStatementNode::ReturnStatementNode(const tokenize::Token &token, std::shared_ptr<Expression> label, std::shared_ptr<Expression> value)
    : m_token(token), m_label(label), m_value(value) {}

const tokenize::Token &ReturnStatementNode::get_token() const { return m_token; }

ReturnStatementNode::self_t ReturnStatementNode::set_label(std::shared_ptr<Expression> label) {
    m_label = label;
    return this;
}

ReturnStatementNode::self_t ReturnStatementNode::set_value(std::shared_ptr<Expression> return_expression) {
    m_value = return_expression;
    return this;
}

const std::shared_ptr<Expression> &ReturnStatementNode::get_label() const { return m_label; }

const std::shared_ptr<Expression> &ReturnStatementNode::get_value() const { return m_value; }

} // namespace nugdev::compiler::ast::statement
