#include "BlockStatementNode.h"
#include <rapidjson/rapidjson.h>

namespace nugdev::compiler::ast::statement {

BlockStatementNode::BlockStatementNode(const tokenize::Token &token, std::vector<std::shared_ptr<Statement>> statements) : m_statements(statements) {}

json::JsonValue BlockStatementNode::to_json(json::JsonAllocator &allocator) const {
    json::JsonValue json(json::Type::kObjectType);
    json.AddMember("type", json::JsonValue("BlockStatement"), allocator);
    json.AddMember("statements", json::JsonValue(json::Type::kArrayType).SetArray().PushBack(m_statements.size(), allocator), allocator);
    return json;
}

icu::UnicodeString BlockStatementNode::to_str() const { return icu::UnicodeString("BlockStatement"); }

const tokenize::Token &BlockStatementNode::get_token() const { return m_statements.front()->get_token(); }

} // namespace nugdev::compiler::ast::statement
