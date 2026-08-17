#pragma once

#include "02_parsing/ParseStrategy.h"

namespace nugdev::compiler::ast::expression {

class FunctionExpressionNodeParseStrategy : public parsing::ParseStrategy {
  public:
    virtual bool can_parse(const parsing::Parser &parser, const tokenize::TokenStream &tokens) override;
    virtual parsing::ParseStrategyResult parse(parsing::Parser &parser, const tokenize::TokenStream &tokens) override;
};

} // namespace nugdev::compiler::ast::expression
