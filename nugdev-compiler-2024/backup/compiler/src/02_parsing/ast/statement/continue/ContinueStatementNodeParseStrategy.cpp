#include "ContinueStatementNodeParseStrategy.h"

#include "00_app/stream/StreamWorkbench.hpp"
#include "02_parsing/ast/expression/identifier/IdentifierLiteralNodeParseStrategy.h"
#include "02_parsing/ast/statement/continue/ContinueStatementNode.h"

namespace nugdev::compiler::ast::statement {

bool ContinueStatementNodeParseStrategy::can_parse(const parsing::Parser &parser, const tokenize::TokenStream &tokens) {
    return contains(tokens.current(), {tokenize::TokenType::Continue});
}

parsing::ParseStrategyResult ContinueStatementNodeParseStrategy::parse(parsing::Parser &parser, const tokenize::TokenStream &tokens) {
    static expression::IdentifierLiteralNodeParseStrategy strategy{};

    auto [node, itr] = stream::workbench(tokens, [this, &parser](tokenize::TokenStream &workbench) {
        auto continueToken = workbench.current();
        workbench.move_next();

        auto continueNode = std::make_shared<ContinueStatementNode>(continueToken.value(), nullptr);
        if (workbench.current().valid() && contains(workbench.current(), {tokenize::TokenType::At})) {
            workbench.move_next();
            if (workbench.current().valid() == false) {
                throw std::runtime_error("invalid continue statement");
            }

            auto [label, itr] = strategy.parse(parser, workbench);
            workbench.move_at(itr);
            continueNode->set_label(label->as<Expression>());
        }

        return continueNode;
    });

    return {node, itr};
}

} // namespace nugdev::compiler::ast::statement
