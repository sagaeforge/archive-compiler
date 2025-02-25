#pragma once

#include "02_parsing/ParseStrategy.h"

namespace nugdev::compiler::ast::expression {

class NumberLiteralNodeParseStrategy : public parsing::ParseStrategy {
  public:
    virtual bool can_parse(const tokenize::TokenStream &tokens) override;
    virtual parsing::ParseStrategyResult parse(const tokenize::TokenStream &tokens) override;
};

} // namespace nugdev::compiler::ast::expression