//
// Created by lambda on 11/1/25.
//

#pragma once

#include "02_parsing/ast/node.h"

class IdentifierExpression;
class TypeExpression;

class VariableStatement final : public Statement {
public:
    explicit VariableStatement(const Token &token, const Node<IdentifierExpression> &name,
                               const Node<TypeExpression> &type,
                               const Node<Expression> &value);

    ~VariableStatement() override = default;

public:
    std::partial_ordering compare(const std::shared_ptr<ASTNode> &other) const override;

    void accept(ASTVisitor &visitor) const override;

    [[nodiscard]] Token token() const override;

public:
    Node<IdentifierExpression> name() const;

    Node<TypeExpression> type() const;

    Node<Expression> value() const;

private:
    Token m_token;
    Node<IdentifierExpression> m_name;
    Node<TypeExpression> m_type;
    Node<Expression> m_value;
};
