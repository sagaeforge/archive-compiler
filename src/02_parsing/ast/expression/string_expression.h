//
// Created by lambda on 11/1/25.
//

#pragma once

#include "02_parsing/ast/node.h"

class StringExpression final : public Expression {
public:
    explicit StringExpression(const Token &m_token);

    ~StringExpression() override = default;

public:
    std::partial_ordering compare(const std::shared_ptr<ASTNode> &other) const override;

    void accept(ASTVisitor &visitor) const override;

    [[nodiscard]] Token token() const override;

private:
    Token m_token;
};
