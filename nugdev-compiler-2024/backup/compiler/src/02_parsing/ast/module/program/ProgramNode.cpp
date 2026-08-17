#include "02_parsing/ast/module/program/ProgramNode.h"
#include <memory>

namespace nugdev::compiler::ast::module {

ProgramNode::ProgramNode(std::vector<StatementPtr> statements) : m_statements(statements) {}

const tokenize::Token &ProgramNode::get_token() const { return m_statements.front()->get_token(); }

} // namespace nugdev::compiler::ast::module
