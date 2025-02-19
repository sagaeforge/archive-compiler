#pragma once

#include <exception>
#include <stdexcept>
#include <unicode/unistr.h>

namespace nugdev::compiler::exception {

class CompilerException : public std::exception {
  public:
    CompilerException(const icu::UnicodeString &message) : message(message) {}

  private:
    icu::UnicodeString message;
};

} // namespace nugdev::compiler::exception
