#pragma once

#include "../TokenFactory.h"

namespace nugdev::compiler::tokenize {

class StringTokenFactory : public TokenFactory {
  public:
    bool canHandle(wchar_t ch) override;
    Token createToken(std::wistream &stream) override;
};

} // namespace nugdev::compiler::tokenize