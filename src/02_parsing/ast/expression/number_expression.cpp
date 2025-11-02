//
// Created by lambda on 11/1/25.
//

#include "number_expression.h"

#include "02_parsing/ast/util/visitor.h"

NumberExpression::NumberExpression(const Token &m_token) : m_token(m_token) {
}

std::partial_ordering NumberExpression::compare(const std::shared_ptr<ASTNode> &other) const {
    auto otherNode = other->as<NumberExpression>();
    if (otherNode == nullptr) {
        return std::partial_ordering::unordered;
    }

    return m_token.compare(otherNode->m_token);
}

void NumberExpression::accept(ASTVisitor &visitor) const {
    visitor.visit(std::static_pointer_cast<const NumberExpression>(self()));
}

Token NumberExpression::token() const {
    return m_token;
}

bool NumberExpression::isFloating() const {
    for (auto literal = m_token.literal(); const auto ch: literal) {
        if (ch == '.') {
            return true;
        }
    }
    return false;
}
