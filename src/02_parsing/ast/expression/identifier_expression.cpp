//
// Created by lambda on 11/1/25.
//

#include "identifier_expression.h"

#include "02_parsing/ast/util/visitor.h"

IdentifierExpression::IdentifierExpression(Token token) : m_token(std::move(token)) {
}

std::partial_ordering IdentifierExpression::compare(const std::shared_ptr<ASTNode> &other) const {
    auto otherNode = other->as<IdentifierExpression>();
    if (otherNode == nullptr) {
        return std::partial_ordering::unordered;
    }

    return m_token.compare(otherNode->m_token);
}

void IdentifierExpression::accept(ASTVisitor &visitor) const {
    visitor.visit(std::static_pointer_cast<const IdentifierExpression>(self()));
}

Token IdentifierExpression::token() const {
    return m_token;
}
