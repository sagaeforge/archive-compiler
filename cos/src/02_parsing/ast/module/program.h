//
// Created by lambda on 11/1/25.
//

#pragma once

#include "02_parsing/ast/node.h"

class Program final : public Module {
public:
    explicit Program(std::vector<Node<Statement> > statements);

    ~Program() override = default;

public:
    std::partial_ordering compare(const std::shared_ptr<ASTNode> &other) const override;

    void accept(ASTVisitor &visitor) const override;

    [[nodiscard]] Token token() const override;

public:
    std::vector<Node<Statement> > statements() const;

private:
    std::vector<Node<Statement> > m_statements;
};
