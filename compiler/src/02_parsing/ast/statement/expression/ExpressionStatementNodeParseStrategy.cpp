#include "ExpressionStatementNodeParseStrategy.h"

namespace nugdev::compiler::ast::statement {

bool ExpressionStatementNodeParseStrategy::can_parse(const tokenize::TokenStream &tokens) { return true; }

parsing::ParseStrategyResult ExpressionStatementNodeParseStrategy::parse(const tokenize::TokenStream &tokens) {
    return parsing::ParseStrategyResult(nullptr, tokens.current());
}

} // namespace nugdev::compiler::ast::statement
