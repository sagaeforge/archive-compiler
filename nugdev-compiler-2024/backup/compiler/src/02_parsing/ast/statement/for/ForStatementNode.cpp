#include "ForStatementNode.h"

namespace nugdev::compiler::ast::statement {

ForStatementNode::ForStatementNode(const tokenize::Token &token, std::shared_ptr<Expression> label, std::shared_ptr<Expression> init,
                                   std::shared_ptr<Expression> condition, std::shared_ptr<Statement> post, std::shared_ptr<Statement> consequence)
    : m_token(token), m_label(label), m_init(init), m_condition(condition), m_post(post), m_consequence(consequence) {}

const tokenize::Token &ForStatementNode::get_token() const { return m_token; }

const std::shared_ptr<Expression> &ForStatementNode::get_label() const { return m_label; }

const std::shared_ptr<Expression> &ForStatementNode::get_init() const { return m_init; }

const std::shared_ptr<Expression> &ForStatementNode::get_condition() const { return m_condition; }

const std::shared_ptr<Statement> &ForStatementNode::get_post() const { return m_post; }

const std::shared_ptr<Statement> &ForStatementNode::get_consequence() const { return m_consequence; }

} // namespace nugdev::compiler::ast::statement
