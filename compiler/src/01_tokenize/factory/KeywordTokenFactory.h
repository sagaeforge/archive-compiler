#pragma once

#include <map>

#include "../TokenFactory.h"

namespace nugdev::compiler::tokenize {

class KeywordTokenFactory : public TokenFactory {
  public:
    KeywordTokenFactory();

  public:
    bool can_handle(const stream::StringStreamIterator &it) override;
    std::tuple<Token, stream::StringStreamIterator> create_token(const stream::StringStreamIterator &it) override;

  private:
    std::map<icu::UnicodeString, TokenType> keywordMap;
};

} // namespace nugdev::compiler::tokenize