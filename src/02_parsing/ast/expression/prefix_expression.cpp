//
// Created by lambda on 11/1/25.
//

#include "prefix_expression.h"

PrefixExpression::PrefixExpression(const Token &m_token, const Node<Expression> &m_right) : m_token(m_token),
    m_right(m_right) {
}

std::partial_ordering PrefixExpression::compare(const std::shared_ptr<ASTNode> &other) const {
    auto otherNode = other->as<PrefixExpression>();
    if (otherNode == nullptr) {
        return std::partial_ordering::unordered;
    }

    if (m_token.compare(otherNode->m_token) != std::partial_ordering::equivalent) {
        return std::partial_ordering::unordered;
    }

    return m_right->compare(otherNode->m_right);
}

void PrefixExpression::accept(ASTVisitor &visitor) const {
    m_right->accept(visitor);
}

Token PrefixExpression::token() const {
    return m_token;
}

Node<Expression> PrefixExpression::right() const {
    return m_right;
}
