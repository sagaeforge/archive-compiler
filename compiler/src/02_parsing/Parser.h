#pragma once

#include <vector>

#include "00_app/stream/Stream.hpp"
#include "01_tokenize/Token.h"

namespace nugdev::compiler::parsing {

enum class Precedence {
    Lowest = 1,
    Equals,      // ==
    LessGreater, // > or <
    Sum,         // +
    Product,     // *
    Prefix,      // -X or !X
    Call,        // myFunction(X)
    Index,       // array[index]
};

Precedence getPrecedence(tokenize::TokenType type);

class Parser {
    using TokenStream = stream::Stream<tokenize::Token>;
    using TokenStreamIterator = typename TokenStream::const_iterator<tokenize::Token>;

  public:
    Parser(const std::vector<tokenize::Token> &tokens);

  private:
    TokenStream stream;
};

} // namespace nugdev::compiler::parsing
