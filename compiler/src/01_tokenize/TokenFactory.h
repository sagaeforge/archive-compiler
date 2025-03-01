#pragma once

#include <tuple>

#include "00_app/stream/Stream.hpp"

#include "Token.h"

namespace nugdev::compiler::tokenize {

class TokenFactory {
  public:
    virtual bool can_handle(const stream::StringStream &stream) = 0;
    virtual std::tuple<Token, stream::StringStreamIterator> create_token(const stream::StringStream &stream) = 0;
};

} // namespace nugdev::compiler::tokenize
