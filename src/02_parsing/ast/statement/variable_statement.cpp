//
// Created by lambda on 11/1/25.
//

#include "variable_statement.h"

#include "02_parsing/ast/expression/identifier_expression.h"
#include "02_parsing/ast/expression/type_expression.h"

VariableStatement::VariableStatement(const Token &token, const Node<IdentifierExpression> &name,
                                     const Node<TypeExpression> &type,
                                     const Node<Expression> &value) : m_token(token), m_name(name), m_type(type),
                                                                      m_value(value) {
}

std::partial_ordering VariableStatement::compare(const std::shared_ptr<ASTNode> &other) const {
    auto otherNode = other->as<VariableStatement>();
    if (otherNode == nullptr) {
        return std::partial_ordering::unordered;
    }

    if (m_name->compare(otherNode->m_name) != std::partial_ordering::equivalent) {
        return std::partial_ordering::unordered;
    }

    const bool isTypeNotNull = m_type != nullptr;
    const bool isOtherTypeNotNull = otherNode->m_type != nullptr;
    if (isTypeNotNull != isOtherTypeNotNull) {
        return std::partial_ordering::unordered;
    }
    if (isTypeNotNull && isOtherTypeNotNull) {
        if (m_type->compare(otherNode->m_type) != std::partial_ordering::equivalent) {
            return std::partial_ordering::unordered;
        }
    }
    const bool isValueNotNull = m_type != nullptr;
    const bool isOtherValueNotNull = otherNode->m_type != nullptr;
    if (isValueNotNull != isOtherValueNotNull) {
        return std::partial_ordering::unordered;
    }
    if (isValueNotNull && isOtherValueNotNull) {
        if (m_value->compare(otherNode->m_value) != std::partial_ordering::equivalent) {
            return std::partial_ordering::unordered;
        }
    }

    return std::partial_ordering::equivalent;
}

void VariableStatement::accept(ASTVisitor &visitor) const {
    m_name->accept(visitor);

    if (m_type != nullptr) {
        m_type->accept(visitor);
    }

    if (m_value != nullptr) {
        m_value->accept(visitor);
    }
}

Token VariableStatement::token() const {
    return m_token;
}

Node<IdentifierExpression> VariableStatement::name() const {
    return m_name;
}

Node<TypeExpression> VariableStatement::type() const {
    return m_type;
}

Node<Expression> VariableStatement::value() const {
    return m_value;
}
