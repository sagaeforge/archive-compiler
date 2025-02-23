#include "StringLiteralNodeParseStrategy.h"

#include "02_parsing/ast/expression/string/StringLiteralNode.h"

namespace nugdev::compiler::ast::expression {

bool StringLiteralNodeParseStrategy::can_parse(const tokenize::TokenStream &tokens) { return tokens.current()->get_type() == tokenize::TokenType::String; }

parsing::ParseStrategyResult StringLiteralNodeParseStrategy::parse(const tokenize::TokenStream &tokens) {
    auto itr = tokens.current();
    return parsing::ParseStrategyResult{std::make_shared<StringLiteralNode>(*itr, itr->get_literal()), itr.next()};
}
} // namespace nugdev::compiler::ast::expression