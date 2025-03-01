#include "ReturnNodeParseStrategy.h"

#include "00_app/stream/StreamWorkbench.hpp"
#include "02_parsing/ast/expression/ExpressionParseStrategy.h"
#include "02_parsing/ast/expression/identifier/IdentifierLiteralNodeParseStrategy.h"
#include "02_parsing/ast/statement/return/ReturnNode.h"

namespace nugdev::compiler::ast::statement {

bool ReturnNodeParseStrategy::can_parse(const tokenize::TokenStream &tokens) { return tokens.current().value().get_type() == tokenize::TokenType::Return; }

parsing::ParseStrategyResult ReturnNodeParseStrategy::parse(const tokenize::TokenStream &tokens) {
    static expression::ExpressionParseStrategy expressionStrategy{};
    static expression::IdentifierLiteralNodeParseStrategy identifierStrategy{};

    auto [node, itr] = stream::workbench(tokens, [this, &tokens](tokenize::TokenStream &workbench) {
        workbench.next();

        auto return_node = std::make_shared<ReturnNode>(tokens.current().value(), static_cast<std::shared_ptr<Expression>>(nullptr),
                                                        static_cast<std::shared_ptr<Expression>>(nullptr));
        if (workbench.current().valid() && contains(workbench.current(), {tokenize::TokenType::At})) {
            workbench.next();
            if (workbench.current().valid() == false) {
                throw std::runtime_error("Invalid token stream");
            }

            auto [label, itr] = identifierStrategy.parse(workbench);
            workbench.move_at(itr);
            return_node->set_label(label->as<Expression>());
        }

        try {
            auto [value, itr] = expressionStrategy.parse(workbench);
            workbench.move_at(itr);
            return_node->set_value(value->as<Expression>());
        } catch (...) {
            return return_node;
        }

        return return_node;
    });

    return {node, itr};
}
} // namespace nugdev::compiler::ast::statement
