#pragma once

#include "02_parsing/ParseStrategy.h"

namespace nugdev::compiler::ast::expression {

/*
    proxy 목적이고, 실제론 expression parse strategy를 사용한다.
*/

class GroupExpressionParseStrategy : public parsing::ParseStrategy {
  public:
    virtual bool can_parse(const tokenize::TokenStream &tokens) override;
    virtual parsing::ParseStrategyResult parse(const tokenize::TokenStream &tokens) override;
};

} // namespace nugdev::compiler::ast::expression