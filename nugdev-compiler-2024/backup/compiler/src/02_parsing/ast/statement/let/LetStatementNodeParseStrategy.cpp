#include "LetStatementNodeParseStrategy.h"

#include "00_app/stream/StreamWorkbench.hpp"
#include "02_parsing/ast/expression/ExpressionParseStrategy.h"
#include "02_parsing/ast/expression/identifier/IdentifierLiteralNodeParseStrategy.h"
#include "02_parsing/ast/expression/type/TypeLiteralNode.h"
#include "02_parsing/ast/expression/type/TypeLiteralNodeParseStrategy.h"
#include "02_parsing/ast/statement/let/LetStatementNode.h"

namespace nugdev::compiler::ast::statement {

bool LetStatementNodeParseStrategy::can_parse(const parsing::Parser &parser, const tokenize::TokenStream &tokens) {
    return contains(tokens.current(), {tokenize::TokenType::Let});
}

/*
let identifier : type
let identifier : type = expression
let identifier = expression // expression은 타입 추론이 필요함.
*/

parsing::ParseStrategyResult LetStatementNodeParseStrategy::parse(parsing::Parser &parser, const tokenize::TokenStream &tokens) {
    static expression::IdentifierLiteralNodeParseStrategy identifierStrategy{};
    static expression::ExpressionParseStrategy expressionStrategy{};
    static expression::TypeLiteralNodeParseStrategy typeStrategy{};
    auto [node, itr] = stream::workbench(tokens, [this, &parser](tokenize::TokenStream &workbench) {
        auto letToken = workbench.current();
        workbench.move_next();

        auto [name, identifierItr] = identifierStrategy.parse(parser, workbench);
        workbench.move_at(identifierItr);

        std::shared_ptr<expression::TypeLiteralNode> type;
        if (contains(workbench.current(), {tokenize::TokenType::Colon})) {
            workbench.move_next();
            auto [type, expressionItr] = typeStrategy.parse(parser, workbench);
            workbench.move_at(expressionItr);
            type = type->as<expression::TypeLiteralNode>();
        }

        if (contains(workbench.current(), {tokenize::TokenType::Assign})) {
            workbench.move_next();
            auto [value, expressionItr] = expressionStrategy.parse(parser, workbench);
            workbench.move_at(expressionItr);

            if (type->get_meta() != value->as<Expression>()->get_type_info()) {
                throw std::runtime_error("type mismatch");
            }

            return std::make_shared<LetStatementNode>(letToken.value(), name->as<Expression>(), type, value->as<Expression>());
        }

        return std::make_shared<LetStatementNode>(letToken.value(), name->as<Expression>(), type, nullptr);
    });
    return {node, itr};
}

} // namespace nugdev::compiler::ast::statement