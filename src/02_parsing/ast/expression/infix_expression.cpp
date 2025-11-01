//
// Created by lambda on 11/1/25.
//

#include "infix_expression.h"

InfixExpression::InfixExpression(const Token &token, const Node<Expression> &left,
                                 const Node<Expression> &right) : m_token(token),
                                                                  m_left(left),
                                                                  m_right(right) {
}

std::partial_ordering InfixExpression::compare(const std::shared_ptr<ASTNode> &other) const {
    auto otherNode = other->as<InfixExpression>();
    if (otherNode == nullptr) {
        return std::partial_ordering::unordered;
    }

    if (m_token.compare(otherNode->token()) != std::partial_ordering::equivalent) {
        return std::partial_ordering::unordered;
    }

    if (m_left->compare(otherNode->m_left) != std::partial_ordering::equivalent) {
        return std::partial_ordering::unordered;
    }

    return m_right->compare(otherNode->m_right);
}

void InfixExpression::accept(ASTVisitor &visitor) const {
    m_left->accept(visitor);
    m_right->accept(visitor);
}

Token InfixExpression::token() const {
    return m_token;
}

Node<Expression> InfixExpression::left() const {
    return m_left;
}

Node<Expression> InfixExpression::right() const {
    return m_right;
}
