#include "ContinueNodeParseStrategy.h"

#include <stdexcept>

#include "01_tokenize/Token.h"
#include "02_parsing/ast/statement/continue/ContinueNode.h"

namespace nugdev::compiler::ast::statement {

bool ContinueNodeParseStrategy::can_parse(const tokenize::TokenStream &tokens) { return tokens.current().value().get_type() == tokenize::TokenType::Continue; }

parsing::ParseStrategyResult ContinueNodeParseStrategy::parse(const tokenize::TokenStream &tokens) {
    auto itr = tokens.current();
    auto token = itr.value();

    auto continue_node = std::make_shared<ContinueNode>(token, nullptr);
    if (itr.next().value().get_type() == tokenize::TokenType::At) {
        itr = itr.next().next();
        if (itr.valid() == false) {
            throw std::runtime_error("Invalid token stream");
        }

        // TODO: 레이블 파싱하는 전략을 부르고 해당 전략을 호출해야함.
        // auto labelExpression = Strategy
    }

    return parsing::ParseStrategyResult(continue_node, itr);
}

} // namespace nugdev::compiler::ast::statement
