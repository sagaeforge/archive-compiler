#pragma once

#include <istream>
#include <memory>
#include <unicode/unistr.h>
#include <vector>

namespace nugdev::compiler::tokenize {

class TokenFactory;
class Token;

class Tokenizer {
  public:
    Tokenizer();

  public:
    std::vector<std::shared_ptr<Token>> tokenize(std::wistream &stream);

  private:
    std::vector<std::shared_ptr<TokenFactory>> factories;
};
} // namespace nugdev::compiler::tokenize
