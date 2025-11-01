//
// Created by lambda on 11/1/25.
//

#pragma once

#include "02_parsing/ast/node.h"

class ReturnStatement final : public Statement {
public:
    explicit ReturnStatement(const Token &token, const Node<Expression> &value);

    ~ReturnStatement() override = default;

public:
    std::partial_ordering compare(const std::shared_ptr<ASTNode> &other) const override;

    void accept(ASTVisitor &visitor) const override;

    [[nodiscard]] Token token() const override;

public:
    Node<Expression> value() const;

private:
    Token m_token;
    Node<Expression> m_value;
};
