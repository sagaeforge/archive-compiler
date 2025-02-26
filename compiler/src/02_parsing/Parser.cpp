#include "Parser.h"

#include <rapidjson/prettywriter.h>

#include "02_parsing/ast/module/program/ProgramNodeParseStrategy.h"

namespace nugdev::compiler::parsing {

std::shared_ptr<ast::Module> Parser::parse(const TokenStream &tokens) {
    static ast::module::ProgramNodeParseStrategy strategy{};

    auto [node, _] = strategy.parse(tokens);
    return node->as<ast::Module>();
}

json::JsonDocument Parser::to_json(const std::shared_ptr<ast::Module> &module) {
    json::JsonDocument document;
    auto json = module->to_json(document.GetAllocator());
    document.SetObject().AddMember("root", json, document.GetAllocator());
    return document;
}

std::string Parser::to_string(const std::shared_ptr<ast::Module> &module) {
    rapidjson::StringBuffer buffer;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
    to_json(module).Accept(writer);
    return buffer.GetString();
}

} // namespace nugdev::compiler::parsing
