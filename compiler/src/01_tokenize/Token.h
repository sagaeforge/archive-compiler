#pragma once

#include "00_app/lib/PointerHelper.hpp"

#include <unicode/unistr.h>

namespace nugdev::compiler::tokenize {

class Token : public lib::PointerHelper<Token> {
  public:
    virtual icu::UnicodeString to_str() = 0;
};

} // namespace nugdev::compiler::tokenize
