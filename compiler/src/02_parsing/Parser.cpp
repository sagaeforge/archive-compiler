#include "Parser.h"

#include "02_parsing/ast/module/program/ProgramNodeParseStrategy.h"

namespace nugdev::compiler::parsing {

std::shared_ptr<ast::Module> Parser::parse(const TokenStream &tokens) {
    static ast::module::ProgramNodeParseStrategy strategy{};

    auto [node, _] = strategy.parse(tokens);
    return node->as<ast::Module>();
}

} // namespace nugdev::compiler::parsing
