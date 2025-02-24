#include "ContinueNodeParseStrategy.h"

#include <stdexcept>

#include "01_tokenize/Token.h"
#include "02_parsing/ast/expression/identifier/IdentifierLiteralNodeParseStrategy.h"
#include "02_parsing/ast/statement/continue/ContinueNode.h"

namespace nugdev::compiler::ast::statement {

bool ContinueNodeParseStrategy::can_parse(const tokenize::TokenStream &tokens) { return contains(tokens.current(), {tokenize::TokenType::Continue}); }

parsing::ParseStrategyResult ContinueNodeParseStrategy::parse(const tokenize::TokenStream &tokens) {
    static expression::IdentifierLiteralNodeParseStrategy strategy{};

    auto workbench = tokens.clone(); // current : continue

    auto continue_node = std::make_shared<ContinueNode>(workbench.current().value(), static_cast<std::shared_ptr<Expression>>(nullptr));
    if (contains(workbench.next().current(), {tokenize::TokenType::At})) {
        workbench.next();
        if (workbench.current().valid() == false) {
            throw std::runtime_error("invalid continue statement");
        }

        auto [label, itr] = strategy.parse(workbench);
        workbench.move_at(itr);
        continue_node->set_label(label->as<Expression>());
    }

    return {continue_node, tokens.begin() + workbench.current().distance()};
}

} // namespace nugdev::compiler::ast::statement
