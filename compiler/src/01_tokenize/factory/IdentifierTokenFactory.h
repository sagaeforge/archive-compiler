#pragma once

#include "../TokenFactory.h"

namespace nugdev::compiler::tokenize {

class IdentifierTokenFactory : public TokenFactory {
  public:
    bool can_handle(const stream::StringStreamIterator &it) override;
    std::tuple<Token, stream::StringStreamIterator> create_token(const stream::StringStreamIterator &it) override;
    bool isIdentifierChar(const stream::StringStreamIterator &it);
};

} // namespace nugdev::compiler::tokenize
