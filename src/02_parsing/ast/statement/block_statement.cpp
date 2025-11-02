//
// Created by lambda on 11/1/25.
//

#include "block_statement.h"

#include "02_parsing/ast/util/visitor.h"

BlockStatement::BlockStatement(std::vector<Node<Statement> > statements) : m_statements(std::move(statements)) {
}

std::partial_ordering BlockStatement::compare(const std::shared_ptr<ASTNode> &other) const {
    const auto otherProgram = other->as<BlockStatement>();
    if (otherProgram == nullptr) {
        return std::partial_ordering::unordered;
    }

    if (otherProgram->m_statements.size() != m_statements.size()) {
        return std::partial_ordering::unordered;
    }

    for (auto i = 0; i < m_statements.size(); ++i) {
        if (otherProgram->m_statements[i]->compare(m_statements[i]) != std::partial_ordering::equivalent) {
            return std::partial_ordering::unordered;
        }
    }

    return std::partial_ordering::equivalent;
}

void BlockStatement::accept(ASTVisitor &visitor) const {
    visitor.visit(std::static_pointer_cast<const BlockStatement>(self()));
}

Token BlockStatement::token() const {
    if (m_statements.empty()) {
        return Token::illegal();
    }
    return m_statements.front()->token();
}

std::vector<Node<Statement> > BlockStatement::statements() const {
    return m_statements;
}
