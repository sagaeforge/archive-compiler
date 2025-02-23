#pragma once

#include "02_parsing/ParseStrategy.h"

namespace nugdev::compiler::ast::expression {

class CallExpressionNodeParseStrategy : public parsing::ParseStrategy {
  public:
    virtual bool can_parse(const tokenize::TokenStream &tokens) override;
    virtual parsing::ParseStrategyResult parse(const tokenize::TokenStream &tokens) override;
    parsing::ParseStrategyResult parse(const tokenize::TokenStream &tokens, std::shared_ptr<Expression> callee);
};

} // namespace nugdev::compiler::ast::expression
