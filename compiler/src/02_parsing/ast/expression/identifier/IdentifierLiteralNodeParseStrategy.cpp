#include "IdentifierLiteralNodeParseStrategy.h"

#include "02_parsing/ast/expression/identifier/IdentifierLiteralNode.h"

namespace nugdev::compiler::ast::expression {

bool IdentifierLiteralNodeParseStrategy::can_parse(const tokenize::TokenStream &tokens) { return tokens.current()->get_type() == tokenize::TokenType::Ident; }

parsing::ParseStrategyResult IdentifierLiteralNodeParseStrategy::parse(const tokenize::TokenStream &tokens) {
    auto itr = tokens.current();
    return parsing::ParseStrategyResult{std::make_shared<IdentifierLiteralNode>(*itr, itr->get_literal()), itr.next()};
}

} // namespace nugdev::compiler::ast::expression
