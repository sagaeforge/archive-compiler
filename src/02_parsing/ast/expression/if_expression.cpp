//
// Created by lambda on 11/1/25.
//

#include "if_expression.h"

#include "02_parsing/ast/statement/block_statement.h"

IfExpression::IfExpression(const Token &token, const Node<Expression> &condition,
                           const Node<BlockStatement> &consequence, const Node<IfExpression> &then,
                           const Node<BlockStatement> &alternative) : m_token(token),
                                                                      m_condition(condition),
                                                                      m_consequence(consequence),
                                                                      m_then(then),
                                                                      m_alternative(alternative) {
}

std::partial_ordering IfExpression::compare(const std::shared_ptr<ASTNode> &other) const {
    auto otherNode = other->as<IfExpression>();
    if (otherNode == nullptr) {
        return std::partial_ordering::unordered;
    }

    if (m_condition->compare(otherNode->m_condition) != std::partial_ordering::equivalent) {
        return std::partial_ordering::unordered;
    }

    if (m_consequence->compare(otherNode->m_consequence) != std::partial_ordering::equivalent) {
        return std::partial_ordering::unordered;
    }

    bool isThenNotNull = m_then != nullptr;
    bool isOtherThenNotNull = otherNode->m_then != nullptr;
    if (isThenNotNull != isOtherThenNotNull) {
        return std::partial_ordering::unordered;
    }

    if (isThenNotNull && isOtherThenNotNull) {
        if (m_then->compare(otherNode->m_then) != std::partial_ordering::equivalent) {
            return std::partial_ordering::unordered;
        }
    }

    bool isAlternativeNotNull = m_alternative != nullptr;
    bool isOtherAlternativeNotNull = otherNode->m_alternative != nullptr;
    if (isAlternativeNotNull != isOtherAlternativeNotNull) {
        return std::partial_ordering::unordered;
    }

    if (isAlternativeNotNull && isOtherAlternativeNotNull) {
        if (m_alternative->compare(otherNode->m_alternative) != std::partial_ordering::equivalent) {
            return std::partial_ordering::unordered;
        }
    }

    return std::partial_ordering::equivalent;
}

void IfExpression::accept(ASTVisitor &visitor) const {
    m_condition->accept(visitor);
    m_consequence->accept(visitor);
    if (m_then != nullptr) {
        m_then->accept(visitor);
    }

    if (m_alternative != nullptr) {
        m_alternative->accept(visitor);
    }
}

Token IfExpression::token() const {
    return m_token;
}

Node<Expression> IfExpression::condition() const {
    return m_condition;
}

Node<BlockStatement> IfExpression::consequence() const {
    return m_consequence;
}

Node<IfExpression> IfExpression::then() const {
    return m_then;
}

Node<BlockStatement> IfExpression::alternative() const {
    return m_alternative;
}
