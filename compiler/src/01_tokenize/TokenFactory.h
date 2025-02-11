#pragma once

#include <tuple>

#include "00_app/stream/Stream.hpp"

#include "Token.h"

namespace nugdev::compiler::tokenize {

class TokenFactory {
  public:
    virtual bool canHandle(const stream::StringStreamIterator &it) = 0;
    virtual std::tuple<Token, stream::StringStreamIterator> createToken(const stream::StringStreamIterator &it) = 0;
};

} // namespace nugdev::compiler::tokenize
