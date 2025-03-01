#include "ContinueNodeParseStrategy.h"

#include "00_app/stream/StreamWorkbench.hpp"
#include "02_parsing/ast/expression/identifier/IdentifierLiteralNodeParseStrategy.h"
#include "02_parsing/ast/statement/continue/ContinueNode.h"

namespace nugdev::compiler::ast::statement {

bool ContinueNodeParseStrategy::can_parse(const tokenize::TokenStream &tokens) { return contains(tokens.current(), {tokenize::TokenType::Continue}); }

parsing::ParseStrategyResult ContinueNodeParseStrategy::parse(const tokenize::TokenStream &tokens) {
    static expression::IdentifierLiteralNodeParseStrategy strategy{};

    auto [node, itr] = stream::workbench(tokens, [this, &tokens](tokenize::TokenStream &workbench) {
        workbench.next();

        auto continueNode = std::make_shared<ContinueNode>(tokens.current().value(), static_cast<std::shared_ptr<Expression>>(nullptr));
        if (workbench.current().valid() && contains(workbench.current(), {tokenize::TokenType::At})) {
            workbench.next();
            if (workbench.current().valid() == false) {
                throw std::runtime_error("invalid continue statement");
            }

            auto [label, itr] = strategy.parse(workbench);
            workbench.move_at(itr);
            continueNode->set_label(label->as<Expression>());
        }

        return continueNode;
    });

    return {node, itr};
}

} // namespace nugdev::compiler::ast::statement
