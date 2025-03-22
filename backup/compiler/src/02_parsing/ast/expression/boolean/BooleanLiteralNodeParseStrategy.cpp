#include "BooleanLiteralNodeParseStrategy.h"

#include "00_app/stream/StreamWorkbench.hpp"
#include "02_parsing/ast/expression/boolean/BooleanLiteralNode.h"

namespace nugdev::compiler::ast::expression {

bool BooleanLiteralNodeParseStrategy::can_parse(const parsing::Parser &parser, const tokenize::TokenStream &tokens) {
    return tokens.current().valid() && contains(tokens.current(), {tokenize::TokenType::True, tokenize::TokenType::False});
}

parsing::ParseStrategyResult BooleanLiteralNodeParseStrategy::parse(parsing::Parser &parser, const tokenize::TokenStream &tokens) {
    auto [node, itr] = stream::workbench(tokens, [this, &parser](tokenize::TokenStream &workbench) {
        auto booleanToken = workbench.current();
        auto value = booleanToken->get_literal() == "true";
        workbench.move_next();
        return std::make_shared<BooleanLiteralNode>(booleanToken.value(), value);
    });
    return {node, itr};
}

} // namespace nugdev::compiler::ast::expression
