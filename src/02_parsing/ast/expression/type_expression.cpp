//
// Created by lambda on 11/1/25.
//

#include "type_expression.h"

TypeExpression::TypeExpression(const Token &m_token) : m_token(m_token) {
}

std::partial_ordering TypeExpression::compare(const std::shared_ptr<ASTNode> &other) const {
    auto otherNode = other->as<TypeExpression>();
    if (otherNode == nullptr) {
        return std::partial_ordering::unordered;
    }

    return m_token.compare(otherNode->m_token);
}

void TypeExpression::accept(ASTVisitor &visitor) const {
}

Token TypeExpression::token() const {
    return m_token;
}
