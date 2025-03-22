#include "BlockStatementNode.h"
#include <rapidjson/rapidjson.h>

namespace nugdev::compiler::ast::statement {

BlockStatementNode::BlockStatementNode(std::vector<std::shared_ptr<Statement>> statements) : m_statements(statements) {}

const tokenize::Token &BlockStatementNode::get_token() const { return m_statements.front()->get_token(); }

const std::vector<std::shared_ptr<Statement>> &BlockStatementNode::get_statements() const { return m_statements; }

} // namespace nugdev::compiler::ast::statement
