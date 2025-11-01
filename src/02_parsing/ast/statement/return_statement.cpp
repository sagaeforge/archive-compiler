//
// Created by lambda on 11/1/25.
//

#include "return_statement.h"

ReturnStatement::ReturnStatement(const Token &token, const Node<Expression> &value) : m_token(token), m_value(value) {
}

std::partial_ordering ReturnStatement::compare(const std::shared_ptr<ASTNode> &other) const {
    auto otherNode = other->as<ReturnStatement>();
    if (otherNode == nullptr) {
        return std::partial_ordering::unordered;
    }

    const bool isNotNull = m_value != nullptr;
    const bool isNotNullOther = otherNode->m_value != nullptr;
    if (isNotNull != isNotNullOther) {
        return std::partial_ordering::unordered;
    }

    if (isNotNull && isNotNullOther) {
        if (m_value->compare(otherNode->m_value) != std::partial_ordering::equivalent) {
            return std::partial_ordering::unordered;
        }
    }

    return std::partial_ordering::equivalent;
}

void ReturnStatement::accept(ASTVisitor &visitor) const {
    if (m_value != nullptr) {
        m_value->accept(visitor);
    }
}

Token ReturnStatement::token() const {
    return m_token;
}

Node<Expression> ReturnStatement::value() const {
    return m_value;
}
