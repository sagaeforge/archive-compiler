//
// Created by lambda on 11/1/25.
//

#pragma once

#include "02_parsing/ast/node.h"

class InfixExpression final : public Expression {
public:
    InfixExpression(const Token &token, const Node<Expression> &left, const Node<Expression> &right);

    ~InfixExpression() override = default;

public:
    std::partial_ordering compare(const std::shared_ptr<ASTNode> &other) const override;

    void accept(ASTVisitor &visitor) const override;

    [[nodiscard]] Token token() const override;

public:
    Node<Expression> left() const;

    Node<Expression> right() const;

private:
    Token m_token;
    Node<Expression> m_left;
    Node<Expression> m_right;
};
