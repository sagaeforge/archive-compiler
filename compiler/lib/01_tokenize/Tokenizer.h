#pragma once

#include "00_lib/lib/String.h"
#include "01_tokenize/token/Token.h"

namespace nugdev::compiler::tokenize {

class TokenizeStrategy;
class Tokenizer {
  public:
    Tokenizer();
    Tokenizer(const std::vector<std::shared_ptr<TokenizeStrategy>> &strategies);

  public:
    std::vector<Token> tokenize(const lib::String &resource);

  private:
    std::vector<std::shared_ptr<TokenizeStrategy>> m_strategies;
};

} // namespace nugdev::compiler::tokenize