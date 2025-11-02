//
// Created by lambda on 11/1/25.
//

#include "function_expression.h"

#include "identifier_expression.h"
#include "type_expression.h"
#include "02_parsing/ast/statement/block_statement.h"
#include "02_parsing/ast/util/visitor.h"

FunctionExpression::FunctionExpression(const Token &token, const Node<IdentifierExpression> &name,
                                       const std::vector<Node<Statement> > &parameters,
                                       const Node<BlockStatement> &body,
                                       const Node<TypeExpression> &returnType) : m_token(token),
    m_name(name),
    m_parameters(parameters),
    m_body(body),
    m_returnType(returnType) {
}

std::partial_ordering FunctionExpression::compare(const std::shared_ptr<ASTNode> &other) const {
    auto otherNode = other->as<FunctionExpression>();
    if (otherNode == nullptr) {
        return std::partial_ordering::unordered;
    }

    if (m_name->compare(otherNode->m_name) != std::partial_ordering::equivalent) {
        return std::partial_ordering::unordered;
    }

    if (m_parameters.size() != otherNode->m_parameters.size()) {
        return std::partial_ordering::unordered;
    }

    for (std::size_t i = 0; i < m_parameters.size(); ++i) {
        if (m_parameters[i]->compare(otherNode->m_parameters[i]) != std::partial_ordering::equivalent) {
            return std::partial_ordering::unordered;
        }
    }

    if (m_body->compare(otherNode->m_body) != std::partial_ordering::equivalent) {
        return std::partial_ordering::unordered;
    }

    if (m_returnType->compare(otherNode->m_returnType) != std::partial_ordering::equivalent) {
        return std::partial_ordering::unordered;
    }

    return std::partial_ordering::equivalent;
}

void FunctionExpression::accept(ASTVisitor &visitor) const {
    visitor.visit(std::static_pointer_cast<const FunctionExpression>(self()));
}

Token FunctionExpression::token() const {
    return m_token;
}

Node<IdentifierExpression> FunctionExpression::name() const {
    return m_name;
}

std::vector<Node<Statement> > FunctionExpression::parameters() const {
    return m_parameters;
}

Node<BlockStatement> FunctionExpression::body() const {
    return m_body;
}

Node<TypeExpression> FunctionExpression::returnType() const {
    return m_returnType;
}
