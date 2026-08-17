//
// Created by lambda on 11/1/25.
//

#pragma once

#include <utility>

#include "01_tokenize/token/token.h"
#include "02_parsing/ast/node.h"

class TypeExpression;
class BlockStatement;
class IdentifierExpression;

class FunctionExpression final : public Expression {
public:
    FunctionExpression(const Token &token, const Node<IdentifierExpression> &name,
                       const std::vector<Node<Statement> > &parameters, const Node<BlockStatement> &body,
                       const Node<TypeExpression> &returnType);

    ~FunctionExpression() override = default;

public:
    std::partial_ordering compare(const std::shared_ptr<ASTNode> &other) const override;

    void accept(ASTVisitor &visitor) const override;

    [[nodiscard]] Token token() const override;

public:
    Node<IdentifierExpression> name() const;

    std::vector<Node<Statement> > parameters() const;

    Node<BlockStatement> body() const;

    Node<TypeExpression> returnType() const;

private:
    Token m_token;
    Node<IdentifierExpression> m_name;
    std::vector<Node<Statement> > m_parameters;
    Node<BlockStatement> m_body;
    Node<TypeExpression> m_returnType;
};
