#include "NumberLiteralNodeParseStrategy.h"

#include "02_parsing/ast/expression/number/NumberLiteralNode.h"

namespace nugdev::compiler::ast::expression {

bool NumberLiteralNodeParseStrategy::can_parse(const tokenize::TokenStream &tokens) { return tokens.current()->get_type() == tokenize::TokenType::Number; }

parsing::ParseStrategyResult NumberLiteralNodeParseStrategy::parse(const tokenize::TokenStream &tokens) {
    auto itr = tokens.current();
    return parsing::ParseStrategyResult{std::make_shared<NumberLiteralNode>(*itr, itr->get_literal()), itr.next()};
}

} // namespace nugdev::compiler::ast::expression
