#pragma once

#include <memory>
#include <unicode/unistr.h>
#include <vector>

#include "Token.h"

namespace nugdev::compiler::tokenize {

class TokenFactory;

class Tokenizer {
  public:
    Tokenizer();
    Tokenizer(std::vector<std::shared_ptr<TokenFactory>> factories);

  public:
    std::vector<Token> tokenize(const icu::UnicodeString &str);

  private:
    std::vector<std::shared_ptr<TokenFactory>> factories;
};
} // namespace nugdev::compiler::tokenize
