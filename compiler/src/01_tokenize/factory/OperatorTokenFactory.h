#pragma once

#include <unordered_map>

#include "../TokenFactory.h"

namespace nugdev::compiler::tokenize {

class OperatorTokenFactory : public TokenFactory {
  public:
    OperatorTokenFactory();

  public:
    bool can_handle(const stream::StringStreamIterator &it) override;
    std::tuple<Token, stream::StringStreamIterator> create_token(const stream::StringStreamIterator &it) override;

  private:
    std::unordered_map<wchar_t, TokenType> operatorMap;
};

} // namespace nugdev::compiler::tokenize
