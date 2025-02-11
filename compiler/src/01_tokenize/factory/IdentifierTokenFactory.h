#pragma once

#include "../TokenFactory.h"

namespace nugdev::compiler::tokenize {

class IdentifierTokenFactory : public TokenFactory {
  public:
    bool canHandle(const stream::StringStreamIterator &it) override;
    std::tuple<Token, stream::StringStreamIterator> createToken(const stream::StringStreamIterator &it) override;
    bool isIdentifierChar(const stream::StringStreamIterator &it);
};

} // namespace nugdev::compiler::tokenize
