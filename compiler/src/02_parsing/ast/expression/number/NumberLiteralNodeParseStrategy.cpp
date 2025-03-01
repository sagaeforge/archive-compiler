#include "NumberLiteralNodeParseStrategy.h"

#include "00_app/stream/StreamWorkbench.hpp"
#include "02_parsing/ast/expression/number/NumberLiteralNode.h"

namespace nugdev::compiler::ast::expression {

bool NumberLiteralNodeParseStrategy::can_parse(const tokenize::TokenStream &tokens) { return contains(tokens.current(), {tokenize::TokenType::Number}); }

parsing::ParseStrategyResult NumberLiteralNodeParseStrategy::parse(const tokenize::TokenStream &tokens) {
    auto [node, itr] = stream::workbench(tokens, [this, &tokens](tokenize::TokenStream &workbench) {
        auto itr = workbench.current();
        workbench.next();
        return std::make_shared<NumberLiteralNode>(tokens.current().value(), itr->get_literal());
    });
    return {node, itr};
}

} // namespace nugdev::compiler::ast::expression
