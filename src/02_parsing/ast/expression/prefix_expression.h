//
// Created by lambda on 11/1/25.
//

#pragma once

#include "02_parsing/ast/node.h"

class PrefixExpression final : public Expression {
public:
    explicit PrefixExpression(const Token &m_token, const Node<Expression> &m_right);

    ~PrefixExpression() override = default;

public:
    std::partial_ordering compare(const std::shared_ptr<ASTNode> &other) const override;

    void accept(ASTVisitor &visitor) const override;

    [[nodiscard]] Token token() const override;

public:
    Node<Expression> right() const;

private:
    Token m_token;
    Node<Expression> m_right;
};
