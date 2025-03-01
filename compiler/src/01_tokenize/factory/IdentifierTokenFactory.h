#pragma once

#include "../TokenFactory.h"

namespace nugdev::compiler::tokenize {

class IdentifierTokenFactory : public TokenFactory {
  public:
    bool can_handle(const stream::StringStream &stream) override;
    std::tuple<Token, stream::StringStreamIterator> create_token(const stream::StringStream &stream) override;
    bool isIdentifierChar(const stream::StringStream &stream);
};

} // namespace nugdev::compiler::tokenize
