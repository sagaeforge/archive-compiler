#pragma once

#include <memory>
#include <unicode/unistr.h>
#include <vector>

#include "00_app/stream/Stream.hpp"
#include "Token.h"

namespace nugdev::compiler::tokenize {

class TokenFactory;

class Tokenizer {
  public:
    Tokenizer();
    Tokenizer(std::vector<std::shared_ptr<TokenFactory>> factories);

  public:
    std::vector<Token> tokenize(const icu::UnicodeString &str);
    stream::StringStreamIterator find_first_of(const stream::StringStream &stream, const std::vector<char16_t> &chars);

  private:
    std::vector<std::shared_ptr<TokenFactory>> factories;
};
} // namespace nugdev::compiler::tokenize
