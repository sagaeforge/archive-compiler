#include "ProgramNode.h"

namespace nugdev::compiler::ast::module {

ProgramNode::ProgramNode(std::vector<StatementPtr> statements) : m_statements(statements) {
}

const tokenize::Token &ProgramNode::get_token() const {
    return m_statements.front()->get_token();
}

std::vector<StatementPtr> &ProgramNode::get_statements() {
    return m_statements;
}

}  // namespace nugdev::compiler::ast::module
