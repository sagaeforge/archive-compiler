//
// Created by lambda on 11/1/25.
//

#pragma once

#include "02_parsing/ast/node.h"

class NumberExpression final : public Expression {
public:
    explicit NumberExpression(const Token &m_token);

    ~NumberExpression() override = default;

public:
    std::partial_ordering compare(const std::shared_ptr<ASTNode> &other) const override;

    void accept(ASTVisitor &visitor) const override;

    [[nodiscard]] Token token() const override;

public:
    bool isFloating() const;
    std::int64_t asInt() const;
    std::uint64_t asUInt() const;

private:
    Token m_token;
};
