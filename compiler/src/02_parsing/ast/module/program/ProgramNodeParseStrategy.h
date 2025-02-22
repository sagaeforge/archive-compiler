#pragma once

#include "02_parsing/ParseStrategy.h"

namespace nugdev::compiler::ast::module {

class ProgramNodeParseStrategy : public parsing::ParseStrategy {
  public:
    parsing::ParseStrategyResult parse(const tokenize::TokenStream &tokens) override;
};

} // namespace nugdev::compiler::ast::module
