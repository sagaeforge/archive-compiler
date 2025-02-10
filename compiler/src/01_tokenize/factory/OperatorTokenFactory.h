#pragma once

#include <unordered_map>

#include "../TokenFactory.h"

namespace nugdev::compiler::tokenize {

class OperatorTokenFactory : public TokenFactory {
  public:
    OperatorTokenFactory();

  public:
    bool canHandle(wchar_t ch) override;
    Token createToken(std::wistream &stream) override;

  private:
    std::unordered_map<wchar_t, TokenType> operatorMap;
};

} // namespace nugdev::compiler::tokenize
