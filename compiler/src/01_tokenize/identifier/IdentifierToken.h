#pragma once

#include "../Token.h"

namespace nugdev::compiler::tokenize {

class IdentifierToken : public Token {
  public:
    IdentifierToken(const icu::UnicodeString &value);

  public:
    icu::UnicodeString to_str() override;

  private:
    icu::UnicodeString value;
};

} // namespace nugdev::compiler::tokenize
