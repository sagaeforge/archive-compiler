//
// Created by lambda on 11/1/25.
//

#pragma once
#include "02_parsing/ast/node.h"


class BlockStatement;

class IfExpression final : public Expression {
public:
    explicit IfExpression(const Token &token, const Node<Expression> &condition,
                          const Node<BlockStatement> &consequence,
                          const Node<IfExpression> &then, const Node<BlockStatement> &alternative);

    ~IfExpression() override = default;

public:
    std::partial_ordering compare(const std::shared_ptr<ASTNode> &other) const override;

    void accept(ASTVisitor &visitor) const override;

    [[nodiscard]] Token token() const override;

public:
    Node<Expression> condition() const;

    Node<BlockStatement> consequence() const;

    Node<IfExpression> then() const;

    Node<BlockStatement> alternative() const;

private:
    Token m_token;
    Node<Expression> m_condition;
    Node<BlockStatement> m_consequence;
    Node<IfExpression> m_then;
    Node<BlockStatement> m_alternative;
};
