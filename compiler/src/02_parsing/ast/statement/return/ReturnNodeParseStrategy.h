#pragma once

#include "02_parsing/ParseStrategy.h"

namespace nugdev::compiler::ast::statement {

class ReturnNodeParseStrategy : public parsing::ParseStrategy {
  public:
    bool can_parse(const tokenize::TokenStream &tokens) override;
    parsing::ParseStrategyResult parse(const tokenize::TokenStream &tokens) override;
};

} // namespace nugdev::compiler::ast::statement