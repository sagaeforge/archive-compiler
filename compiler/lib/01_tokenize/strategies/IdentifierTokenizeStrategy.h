#pragma once

#include "01_tokenize/strategies/TokenizeStrategy.h"

namespace nugdev::compiler::tokenize {

class IdentifierTokenizeStrategy : public TokenizeStrategy {
  public:
    IdentifierTokenizeStrategy();

  public:
    bool can_handle(const lib::iterator::Workbench<lib::Char>::command_t &command) override;
    std::optional<Token> handle(const lib::iterator::Workbench<lib::Char>::command_t &command) override;

  private:
    std::unordered_map<lib::String, TokenType> m_keywordMap;
};

} // namespace nugdev::compiler::tokenize
