#pragma once

#include "01_tokenize/strategies/TokenizeStrategy.h"

namespace nugdev::compiler::tokenize {

class StringTokenizeStrategy : public TokenizeStrategy {
  public:
    bool can_handle(const lib::iterator::Workbench<lib::Char>::command_t &command) override;
    std::optional<Token> handle(const lib::iterator::Workbench<lib::Char>::command_t &command) override;
};

} // namespace nugdev::compiler::tokenize