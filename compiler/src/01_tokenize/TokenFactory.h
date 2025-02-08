#pragma once

#include <istream>
#include <memory>

namespace nugdev::compiler::tokenize {

class Token;

class TokenFactory {
  public:
    virtual bool canHandle(wchar_t ch) = 0;
    virtual std::shared_ptr<Token> createToken(std::wistream &stream) = 0;
};

} // namespace nugdev::compiler::tokenize
