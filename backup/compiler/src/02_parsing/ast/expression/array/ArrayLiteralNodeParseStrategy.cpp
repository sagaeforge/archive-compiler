#include "ArrayLiteralNodeParseStrategy.h"

#include "00_app/stream/StreamWorkbench.hpp"
#include "02_parsing/ParseStrategy.h"
#include "02_parsing/Parser.h"
#include "02_parsing/ast/expression/ExpressionParseStrategy.h"
#include "02_parsing/ast/expression/array/ArrayLiteralNode.h"

namespace nugdev::compiler::ast::expression {

bool ArrayLiteralNodeParseStrategy::can_parse(const parsing::Parser &parser, const tokenize::TokenStream &tokens) {
    return tokens.current().valid() && contains(tokens.current(), {tokenize::TokenType::LBracket});
}

parsing::ParseStrategyResult ArrayLiteralNodeParseStrategy::parse(parsing::Parser &parser, const tokenize::TokenStream &tokens) {
    static ExpressionParseStrategy strategy{};

    auto [node, itr] = stream::workbench(tokens, [this, &parser](tokenize::TokenStream &workbench) {
        auto arrayToken = workbench.current();
        std::vector<std::shared_ptr<ast::Expression>> list;

        // current: '['
        do {
            workbench.move_next();

            if (contains(workbench.current(), {tokenize::TokenType::RBracket})) {
                workbench.move_next();
                return std::make_shared<ArrayLiteralNode>(arrayToken.value(), list);
            }

            auto [element, moveItr] = strategy.parse(parser, workbench, parsing::Precedence::Lowest);
            workbench.move_at(moveItr);
            list.push_back(element->as<ast::Expression>());
        } while (contains(workbench.current(), {tokenize::TokenType::Comma}));

        if (!contains(workbench.current(), {tokenize::TokenType::RBracket})) {
            throw std::runtime_error("Expected ']'");
        }
        workbench.move_next();

        return std::make_shared<ArrayLiteralNode>(arrayToken.value(), list);
    });

    return {node, itr};
}
} // namespace nugdev::compiler::ast::expression
