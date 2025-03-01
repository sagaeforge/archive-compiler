#include "StringLiteralNodeParseStrategy.h"

#include "02_parsing/ast/expression/string/StringLiteralNode.h"

namespace nugdev::compiler::ast::expression {

bool StringLiteralNodeParseStrategy::can_parse(const tokenize::TokenStream &tokens) { return contains(tokens.current(), {tokenize::TokenType::String}); }

parsing::ParseStrategyResult StringLiteralNodeParseStrategy::parse(const tokenize::TokenStream &tokens) {
    return parsing::ParseStrategyResult{std::make_shared<StringLiteralNode>(tokens.current().value(), tokens.current()->get_literal()),
                                        tokens.current().next()};
}

} // namespace nugdev::compiler::ast::expression