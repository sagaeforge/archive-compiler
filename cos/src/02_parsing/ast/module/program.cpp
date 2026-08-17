//
// Created by lambda on 11/1/25.
//

#include "program.h"

#include <utility>

#include "02_parsing/ast/util/visitor.h"

Program::Program(std::vector<Node<Statement> > statements) : m_statements(std::move(statements)) {
}

std::partial_ordering Program::compare(const Node<ASTNode> &other) const {
    const auto otherProgram = other->as<Program>();
    if (otherProgram == nullptr) {
        return std::partial_ordering::unordered;
    }

    if (otherProgram->m_statements.size() != m_statements.size()) {
        return std::partial_ordering::unordered;
    }

    for (auto i = 0; i < m_statements.size(); ++i) {
        if (otherProgram->m_statements[i] != m_statements[i]) {
            return std::partial_ordering::unordered;
        }
    }

    return std::partial_ordering::equivalent;
}

void Program::accept(ASTVisitor &visitor) const {
    visitor.visit(std::static_pointer_cast<const Program>(self()));
}

Token Program::token() const {
    if (m_statements.empty()) {
        return Token::illegal();
    }
    return m_statements.front()->token();
}

std::vector<Node<Statement> > Program::statements() const {
    return m_statements;
}
