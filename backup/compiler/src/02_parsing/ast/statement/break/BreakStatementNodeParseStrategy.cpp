#include "BreakStatementNodeParseStrategy.h"

#include "00_app/stream/StreamWorkbench.hpp"
#include "02_parsing/ast/expression/identifier/IdentifierLiteralNodeParseStrategy.h"
#include "02_parsing/ast/statement/break/BreakStatementNode.h"

namespace nugdev::compiler::ast::statement {

bool BreakStatementNodeParseStrategy::can_parse(const parsing::Parser &parser, const tokenize::TokenStream &tokens) {
    return contains(tokens.current(), {tokenize::TokenType::Break});
}

parsing::ParseStrategyResult BreakStatementNodeParseStrategy::parse(parsing::Parser &parser, const tokenize::TokenStream &tokens) {
    static expression::IdentifierLiteralNodeParseStrategy strategy{};

    auto [node, itr] = stream::workbench(tokens, [this, &parser](tokenize::TokenStream &workbench) {
        // current: 'break'
        auto breakToken = workbench.current();
        workbench.move_next();

        auto breakNode = std::make_shared<BreakStatementNode>(breakToken.value(), nullptr);
        if (workbench.current().valid() && contains(workbench.current(), {tokenize::TokenType::At})) {
            workbench.move_next();
            if (workbench.current().valid() == false) {
                throw std::runtime_error("invalid break statement");
            }

            auto [label, itr] = strategy.parse(parser, workbench);
            workbench.move_at(itr);
            breakNode->set_label(label->as<Expression>());
        }

        return breakNode;
    });

    return {node, itr};
}

} // namespace nugdev::compiler::ast::statement
