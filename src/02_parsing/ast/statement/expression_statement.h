//
// Created by lambda on 11/1/25.
//

#pragma once

#include "02_parsing/ast/node.h"

class ExpressionStatement final : public Statement {
public:
    ExpressionStatement(const Node<Expression> &expression);

    ~ExpressionStatement() override = default;

public:
    std::partial_ordering compare(const std::shared_ptr<ASTNode> &other) const override;

    void accept(ASTVisitor &visitor) const override;

    [[nodiscard]] Token token() const override;

public:
    Node<Expression> expression();

private:
    Node<Expression> m_expression;
};
