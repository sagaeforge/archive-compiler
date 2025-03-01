#pragma once

#include "../TokenFactory.h"
#include "00_app/stream/Stream.hpp"

namespace nugdev::compiler::tokenize {

class StringTokenFactory : public TokenFactory {
  public:
    bool can_handle(const stream::StringStream &stream) override;
    std::tuple<Token, stream::StringStreamIterator> create_token(const stream::StringStream &stream) override;
};

} // namespace nugdev::compiler::tokenize