#pragma once

#include "../TokenFactory.h"

namespace nugdev::compiler::tokenize {

class IdentifierTokenFactory : public TokenFactory {
  public:
    bool canHandle(wchar_t ch) override;
    std::shared_ptr<Token> createToken(std::wistream &stream) override;
    bool isIdentifierChar(wchar_t ch);
};

} // namespace nugdev::compiler::tokenize
