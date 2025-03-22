#pragma once

#include "02_parsing/ParseStrategy.h"

namespace nugdev::compiler::ast::module {

class ProgramNodeParseStrategy : public parsing::ParseStrategy {
  public:
    bool can_parse(const parsing::Parser &parser, const tokenize::TokenStream &tokens) override;
    parsing::ParseStrategyResult parse(parsing::Parser &parser, const tokenize::TokenStream &tokens) override;
};

} // namespace nugdev::compiler::ast::module
