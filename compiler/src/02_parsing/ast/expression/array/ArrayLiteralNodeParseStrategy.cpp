#include "ArrayLiteralNodeParseStrategy.h"

#include "00_app/stream/StreamWorkbench.hpp"
#include "02_parsing/ParseStrategy.h"
#include "02_parsing/ast/expression/ExpressionParseStrategy.h"
#include "02_parsing/ast/expression/array/ArrayLiteralNode.h"

namespace nugdev::compiler::ast::expression {

bool ArrayLiteralNodeParseStrategy::can_parse(const tokenize::TokenStream &tokens) {
    return tokens.current().valid() && contains(tokens.current(), {tokenize::TokenType::LBracket});
}

parsing::ParseStrategyResult ArrayLiteralNodeParseStrategy::parse(const tokenize::TokenStream &tokens) {
    static ExpressionParseStrategy strategy{};

    auto [node, itr] = stream::workbench(tokens, [this, &tokens](tokenize::TokenStream &workbench) {
        std::vector<std::shared_ptr<ast::Expression>> list;

        // current: '['
        do {
            workbench.next();

            if (contains(workbench.current(), {tokenize::TokenType::RBracket})) {
                workbench.next();
                return std::make_shared<ArrayLiteralNode>(tokens.current().value(), list);
            }

            auto [element, moveItr] = strategy.parse(workbench, ExpressionParseStrategy::Precedence::Lowest);
            workbench.move_at(moveItr);
            list.push_back(element->as<ast::Expression>());
        } while (contains(workbench.current(), {tokenize::TokenType::Comma}));

        if (!contains(workbench.current(), {tokenize::TokenType::RBracket})) {
            throw std::runtime_error("Expected ']'");
        }
        workbench.next();

        return std::make_shared<ArrayLiteralNode>(tokens.current().value(), list);
    });

    return {node, itr};
}
} // namespace nugdev::compiler::ast::expression
