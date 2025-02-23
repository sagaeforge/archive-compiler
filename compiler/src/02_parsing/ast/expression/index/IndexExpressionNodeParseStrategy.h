#pragma once

#include "02_parsing/ParseStrategy.h"

namespace nugdev::compiler::ast::expression {

class IndexExpressionNodeParseStrategy : public parsing::ParseStrategy {
  public:
    virtual bool can_parse(const tokenize::TokenStream &tokens) override;
    virtual parsing::ParseStrategyResult parse(const tokenize::TokenStream &tokens) override;
    parsing::ParseStrategyResult parse(const tokenize::TokenStream &tokens, std::shared_ptr<Expression> left);
};

} // namespace nugdev::compiler::ast::expression