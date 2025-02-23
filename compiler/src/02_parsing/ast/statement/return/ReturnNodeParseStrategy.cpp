#include "ReturnNodeParseStrategy.h"

#include <stdexcept>

#include "02_parsing/ast/statement/return/ReturnNode.h"

namespace nugdev::compiler::ast::statement {

bool ReturnNodeParseStrategy::can_parse(const tokenize::TokenStream &tokens) { return tokens.current().value().get_type() == tokenize::TokenType::Return; }

parsing::ParseStrategyResult ReturnNodeParseStrategy::parse(const tokenize::TokenStream &tokens) {
    auto itr = tokens.current();
    auto token = itr.value();

    auto return_node = std::make_shared<ReturnNode>(token, nullptr, nullptr);

    if (itr.next().value().get_type() == tokenize::TokenType::At) {
        itr = itr.next().next();
        if (itr.valid() == false) {
            throw std::runtime_error("Invalid token stream");
        }

        // TODO: 레이블 파싱하는 전략을 부르고 해당 전략을 호출해야함.
        // auto labelExpression = Strategy
    }

    // TODO 여기는 return_value를 표현하기 위한

    return parsing::ParseStrategyResult(return_node, itr.next());
}
} // namespace nugdev::compiler::ast::statement
