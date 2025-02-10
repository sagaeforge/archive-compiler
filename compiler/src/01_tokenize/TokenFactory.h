#pragma once

#include <istream>
#include <unicode/unistr.h>

#include "Token.h"

namespace nugdev::compiler::tokenize {

class TokenFactory {
  public:
    virtual bool canHandle(wchar_t ch) = 0;
    virtual Token createToken(std::wistream &stream) = 0;
};

} // namespace nugdev::compiler::tokenize
