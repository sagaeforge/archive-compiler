#include "BreakNodeParseStrategy.h"

#include <stdexcept>

#include "01_tokenize/Token.h"
#include "02_parsing/ast/statement/break/BreakNode.h"

namespace nugdev::compiler::ast::statement {

bool BreakNodeParseStrategy::can_parse(const tokenize::TokenStream &tokens) { return tokens.current().value().get_type() == tokenize::TokenType::Break; }

parsing::ParseStrategyResult BreakNodeParseStrategy::parse(const tokenize::TokenStream &tokens) {
    auto itr = tokens.current();
    auto token = itr.value();

    auto break_node = std::make_shared<BreakNode>(token, nullptr);
    if (itr.next().value().get_type() == tokenize::TokenType::At) {
        itr = itr.next().next();
        if (itr.valid() == false) {
            throw std::runtime_error("유효하지 않는 구문");
        }

        // TODO: 레이블 파싱하는 전략을 부르고 해당 전략을 호출해야함.
        // auto labelExpression = Strategy
    }

    return parsing::ParseStrategyResult(break_node, itr);
}

} // namespace nugdev::compiler::ast::statement
