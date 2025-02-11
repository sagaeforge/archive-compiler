#pragma once

#include "../TokenFactory.h"

namespace nugdev::compiler::tokenize {

class StringTokenFactory : public TokenFactory {
  public:
    bool canHandle(const stream::StringStreamIterator &it) override;
    std::tuple<Token, stream::StringStreamIterator> createToken(const stream::StringStreamIterator &it) override;
};

} // namespace nugdev::compiler::tokenize