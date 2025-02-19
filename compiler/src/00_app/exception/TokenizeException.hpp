#pragma once

#include "CompilerException.hpp"

namespace nugdev::compiler::exception {

class TokenizeException : public CompilerException {
  public:
    TokenizeException(const icu::UnicodeString &message) : CompilerException(message) {}
};

class InvalidKeywordException : public TokenizeException {
  public:
    InvalidKeywordException(const icu::UnicodeString &message) : TokenizeException(message) {}
};

class InvalidOperatorException : public TokenizeException {
  public:
    InvalidOperatorException(const icu::UnicodeString &message) : TokenizeException(message) {}
};

} // namespace nugdev::compiler::exception