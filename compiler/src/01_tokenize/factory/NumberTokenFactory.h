#pragma once

#include "../TokenFactory.h"

namespace nugdev::compiler::tokenize {

class NumberTokenFactory : public TokenFactory {
  public:
    bool can_handle(const stream::StringStreamIterator &it) override;
    std::tuple<Token, stream::StringStreamIterator> create_token(const stream::StringStreamIterator &it) override;
};

} // namespace nugdev::compiler::tokenize