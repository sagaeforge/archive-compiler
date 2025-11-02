//
// Created by lambda on 11/1/25.
//

#include "string_expression.h"

#include "02_parsing/ast/util/visitor.h"

StringExpression::StringExpression(const Token &m_token) : m_token(m_token) {
}

std::partial_ordering StringExpression::compare(const std::shared_ptr<ASTNode> &other) const {
    auto otherNode = other->as<StringExpression>();
    if (otherNode == nullptr) {
        return std::partial_ordering::equivalent;
    }

    return m_token.compare(otherNode->m_token);
}

void StringExpression::accept(ASTVisitor &visitor) const {
    visitor.visit(std::static_pointer_cast<const StringExpression>(self()));
}

Token StringExpression::token() const {
    return m_token;
}
