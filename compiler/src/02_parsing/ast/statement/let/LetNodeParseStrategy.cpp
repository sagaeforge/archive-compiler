#include "LetNodeParseStrategy.h"
#include "01_tokenize/Token.h"

namespace nugdev::compiler::ast::statement {

parsing::ParseStrategyResult LetNodeParseStrategy::parse(const tokenize::TokenStream &tokens) {
    // TODO 추후 구현
    return {nullptr, tokens.current()};
}

bool LetNodeParseStrategy::can_parse(const tokenize::TokenStream &tokens) { return tokens.current().value().get_type() == tokenize::TokenType::Let; }

} // namespace nugdev::compiler::ast::statement