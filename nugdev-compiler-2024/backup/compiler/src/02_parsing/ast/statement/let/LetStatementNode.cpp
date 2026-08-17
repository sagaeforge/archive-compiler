#include "LetStatementNode.h"

#include "02_parsing/ast/expression/type/TypeLiteralNode.h"

namespace nugdev::compiler::ast::statement {

LetStatementNode::LetStatementNode(const tokenize::Token &token, const std::shared_ptr<Expression> &name,
                                   const std::shared_ptr<expression::TypeLiteralNode> &type, const std::shared_ptr<Expression> &value)
    : m_token(token), m_name(name), m_type(type), m_value(value) {}

const tokenize::Token &LetStatementNode::get_token() const { return m_token; }

const std::shared_ptr<Expression> &LetStatementNode::get_name() const { return m_name; }

const std::shared_ptr<expression::TypeLiteralNode> &LetStatementNode::get_type() const { return m_type; }

const std::shared_ptr<Expression> &LetStatementNode::get_value() const { return m_value; }

} // namespace nugdev::compiler::ast::statement