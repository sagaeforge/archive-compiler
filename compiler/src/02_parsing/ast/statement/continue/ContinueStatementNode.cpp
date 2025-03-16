#include "ContinueStatementNode.h"

namespace nugdev::compiler::ast::statement {

ContinueStatementNode::ContinueStatementNode(const tokenize::Token &token, std::shared_ptr<Expression> label) : m_token(token), m_label(label) {}

const tokenize::Token &ContinueStatementNode::get_token() const { return m_token; }

ContinueStatementNode::self_t ContinueStatementNode::set_label(std::shared_ptr<Expression> label) {
    m_label = label;
    return *this;
}

const std::shared_ptr<Expression> &ContinueStatementNode::get_label() const { return m_label; }

} // namespace nugdev::compiler::ast::statement
