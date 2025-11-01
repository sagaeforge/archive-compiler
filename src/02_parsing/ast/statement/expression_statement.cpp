//
// Created by lambda on 11/1/25.
//

#include "expression_statement.h"

ExpressionStatement::ExpressionStatement(const Node<Expression> &expression) : m_expression(expression) {
}

std::partial_ordering ExpressionStatement::compare(const std::shared_ptr<ASTNode> &other) const {
    auto otherNode = other->as<ExpressionStatement>();
    if (otherNode == nullptr) {
        return std::partial_ordering::unordered;
    }

    return m_expression->compare(otherNode->m_expression);
}

void ExpressionStatement::accept(ASTVisitor &visitor) const {
    m_expression->accept(visitor);
}

Token ExpressionStatement::token() const {
    return m_expression->token();
}

Node<Expression> ExpressionStatement::expression() {
    return m_expression;
}
