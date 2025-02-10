#pragma once

#include <istream>
#include <memory>
#include <unicode/unistr.h>
#include <vector>

#include "Token.h"

namespace nugdev::compiler::tokenize {

class TokenFactory;

class Tokenizer {
  public:
    Tokenizer();

  public:
    std::vector<Token> tokenize(std::wistream &stream);

  private:
    std::vector<std::shared_ptr<TokenFactory>> factories;
};
} // namespace nugdev::compiler::tokenize
