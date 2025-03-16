#include "LetStatementNodeParseStrategy.h"

#include "00_app/stream/StreamWorkbench.hpp"
#include "02_parsing/ast/expression/ExpressionParseStrategy.h"
#include "02_parsing/ast/expression/identifier/IdentifierLiteralNodeParseStrategy.h"
#include "02_parsing/ast/expression/type/TypeLiteralNode.h"
#include "02_parsing/ast/expression/type/TypeLiteralNodeParseStrategy.h"
#include "02_parsing/ast/statement/let/LetStatementNode.h"

namespace nugdev::compiler::ast::statement {

bool LetStatementNodeParseStrategy::can_parse(const tokenize::TokenStream &tokens) { return contains(tokens.current(), {tokenize::TokenType::Let}); }

/*
let identifier : type
let identifier : type = expression
let identifier = expression // expression은 타입 추론이 필요함.
*/

parsing::ParseStrategyResult LetStatementNodeParseStrategy::parse(const tokenize::TokenStream &tokens) {
    static expression::IdentifierLiteralNodeParseStrategy identifierStrategy{};
    static expression::ExpressionParseStrategy expressionStrategy{};
    static expression::TypeLiteralNodeParseStrategy typeStrategy{};
    auto [node, itr] = stream::workbench(tokens, [this, &tokens](tokenize::TokenStream &workbench) {
        auto letToken = tokens.current();
        workbench.next();

        auto [name, identifierItr] = identifierStrategy.parse(workbench);
        workbench.move_at(identifierItr);

        // auto letNode = std::make_shared<LetStatementNode>(letToken.value(), name->as<Expression>(), nullptr, nullptr);
        if (contains(workbench.current(), {tokenize::TokenType::Colon})) {
            workbench.next();
            auto [type, expressionItr] = typeStrategy.parse(workbench);
            workbench.move_at(expressionItr);
            return std::make_shared<LetStatementNode>(letToken.value(), name->as<Expression>(), type->as<expression::TypeLiteralNode>(), nullptr);
        }

        if (contains(workbench.current(), {tokenize::TokenType::Assign})) {
            workbench.next();
            auto [value, expressionItr] = expressionStrategy.parse(workbench);
            workbench.move_at(expressionItr);

            return std::make_shared<LetStatementNode>(letToken.value(), name->as<Expression>(), nullptr, value->as<Expression>());
        }

        return std::make_shared<LetStatementNode>(letToken.value(), name->as<Expression>(), nullptr, nullptr);
    });
    return {node, itr};
}

} // namespace nugdev::compiler::ast::statement