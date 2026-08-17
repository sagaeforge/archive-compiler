//
// Created by lambda on 11/1/25.
//

#pragma once

#include "02_parsing/ast/node.h"

class CallExpression final : public Expression {
public:
    explicit CallExpression(const Token &token, const Node<Expression> &callee,
                            const std::vector<Node<Expression> > &args);

    ~CallExpression() override = default;

public:
    std::partial_ordering compare(const std::shared_ptr<ASTNode> &other) const override;

    void accept(ASTVisitor &visitor) const override;

    [[nodiscard]] Token token() const override;

public:
    Node<Expression> callee() const;

    std::vector<Node<Expression> > args() const;

private:
    Token m_token;
    Node<Expression> m_callee;
    std::vector<Node<Expression> > m_args;
};
