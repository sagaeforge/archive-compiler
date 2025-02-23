#pragma once

#include "02_parsing/ParseStrategy.h"

namespace nugdev::compiler::ast::statement {

class BreakNodeParseStrategy;
class ContinueNodeParseStrategy;
class ReturnNodeParseStrategy;

class StatementParseStrategy : public parsing::ParseStrategy {
  public:
    virtual bool can_parse(const tokenize::TokenStream &tokens) = 0;
    virtual parsing::ParseStrategyResult parse(const tokenize::TokenStream &tokens) = 0;
};

} // namespace nugdev::compiler::ast::statement
