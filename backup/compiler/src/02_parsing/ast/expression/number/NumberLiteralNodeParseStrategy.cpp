#include "NumberLiteralNodeParseStrategy.h"

#include "00_app/stream/StreamWorkbench.hpp"
#include "02_parsing/ast/expression/number/NumberLiteralNode.h"

namespace nugdev::compiler::ast::expression {

bool NumberLiteralNodeParseStrategy::can_parse(const parsing::Parser &parser, const tokenize::TokenStream &tokens) {
    return contains(tokens.current(), {tokenize::TokenType::Number});
}

parsing::ParseStrategyResult NumberLiteralNodeParseStrategy::parse(parsing::Parser &parser, const tokenize::TokenStream &tokens) {
    auto [node, itr] = stream::workbench(tokens, [this, &parser](tokenize::TokenStream &workbench) {
        auto numberToken = workbench.current();
        workbench.move_next();
        return std::make_shared<NumberLiteralNode>(numberToken.value(), numberToken->get_literal());
    });
    return {node, itr};
}

} // namespace nugdev::compiler::ast::expression
