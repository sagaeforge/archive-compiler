#pragma once

#include <map>

#include "../TokenFactory.h"

namespace nugdev::compiler::tokenize {

class KeywordTokenFactory : public TokenFactory {
  public:
    KeywordTokenFactory();

  public:
    bool canHandle(wchar_t ch) override;
    Token createToken(std::wistream &stream) override;

  private:
    std::map<icu::UnicodeString, TokenType> keywordMap;
};

} // namespace nugdev::compiler::tokenize