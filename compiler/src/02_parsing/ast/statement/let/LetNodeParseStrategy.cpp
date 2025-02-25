#include "LetNodeParseStrategy.h"

#include "02_parsing/ast/expression/ExpressionParseStrategy.h"
#include "02_parsing/ast/expression/identifier/IdentifierLiteralNodeParseStrategy.h"
#include "02_parsing/ast/statement/let/LetNode.h"

namespace nugdev::compiler::ast::statement {

bool LetNodeParseStrategy::can_parse(const tokenize::TokenStream &tokens) { return contains(tokens.current(), {tokenize::TokenType::Let}); }

parsing::ParseStrategyResult LetNodeParseStrategy::parse(const tokenize::TokenStream &tokens) {
    static expression::IdentifierLiteralNodeParseStrategy identifierStrategy{};
    static expression::ExpressionParseStrategy expressionStrategy{};

    auto letToken = tokens.current();
    auto workbench = tokens.clone(); // current : let
    workbench.next();

    auto [name, identifierItr] = identifierStrategy.parse(workbench);
    workbench.move_at(identifierItr);

    auto letNode = std::make_shared<LetNode>(letToken.value(), name->as<Expression>(), nullptr, nullptr);
    if (contains(workbench.current(), {tokenize::TokenType::Colon})) {
        workbench.next();
        auto [type, expressionItr] = expressionStrategy.parse(workbench);
        workbench.move_at(expressionItr);
        letNode->set_type(type->as<Expression>());
    }

    if (contains(workbench.current(), {tokenize::TokenType::Assign})) {
        workbench.next();
        auto [value, expressionItr] = expressionStrategy.parse(workbench);
        workbench.move_at(expressionItr);
        letNode->set_value(value->as<Expression>());
    }

    return {letNode, tokens.begin() + workbench.current().distance()};
}

} // namespace nugdev::compiler::ast::statement