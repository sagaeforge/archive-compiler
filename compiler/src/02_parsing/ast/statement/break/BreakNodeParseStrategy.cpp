#include "BreakNodeParseStrategy.h"

#include <stdexcept>

#include "01_tokenize/Token.h"
#include "02_parsing/ast/expression/identifier/IdentifierLiteralNodeParseStrategy.h"
#include "02_parsing/ast/statement/break/BreakNode.h"

namespace nugdev::compiler::ast::statement {

bool BreakNodeParseStrategy::can_parse(const tokenize::TokenStream &tokens) { return contains(tokens.current(), {tokenize::TokenType::Break}); }

parsing::ParseStrategyResult BreakNodeParseStrategy::parse(const tokenize::TokenStream &tokens) {
    static expression::IdentifierLiteralNodeParseStrategy strategy{};

    auto workbench = tokens.clone(); // current : break

    auto breakNode = std::make_shared<BreakNode>(workbench.current().value(), static_cast<std::shared_ptr<Expression>>(nullptr));
    if (contains(workbench.next().current(), {tokenize::TokenType::At})) {
        workbench.next();
        if (workbench.current().valid() == false) {
            throw std::runtime_error("invalid break statement");
        }

        auto [label, itr] = strategy.parse(workbench);
        workbench.move_at(itr);
        breakNode->set_label(label->as<Expression>());
    }

    return {breakNode, tokens.begin() + workbench.current().distance()};
}

} // namespace nugdev::compiler::ast::statement
