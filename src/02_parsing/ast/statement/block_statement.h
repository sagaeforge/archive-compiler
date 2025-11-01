//
// Created by lambda on 11/1/25.
//

#pragma once

#include "02_parsing/ast/node.h"

class BlockStatement final : public Statement {
public:
    explicit BlockStatement(std::vector<Node<Statement> > statements);

    ~BlockStatement() override = default;

public:
    std::partial_ordering compare(const std::shared_ptr<ASTNode> &other) const override;

    void accept(ASTVisitor &visitor) const override;

    [[nodiscard]] Token token() const override;

public:
    std::vector<Node<Statement> > statements();

private:
    std::vector<Node<Statement> > m_statements;
};
