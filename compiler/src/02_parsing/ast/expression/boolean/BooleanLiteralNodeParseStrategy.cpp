#include "BooleanLiteralNodeParseStrategy.h"

#include "00_app/stream/StreamWorkbench.hpp"
#include "02_parsing/ast/expression/boolean/BooleanLiteralNode.h"

namespace nugdev::compiler::ast::expression {

bool BooleanLiteralNodeParseStrategy::can_parse(const tokenize::TokenStream &tokens) {
    return tokens.current().valid() && contains(tokens.current(), {tokenize::TokenType::True, tokenize::TokenType::False});
}

parsing::ParseStrategyResult BooleanLiteralNodeParseStrategy::parse(const tokenize::TokenStream &tokens) {
    auto [node, itr] = stream::workbench(tokens, [this, &tokens](tokenize::TokenStream &workbench) {
        auto itr = workbench.current();
        auto value = itr->get_literal() == "true";
        workbench.next();
        return std::make_shared<BooleanLiteralNode>(tokens.current().value(), value);
    });
    return {node, itr};
}

} // namespace nugdev::compiler::ast::expression
