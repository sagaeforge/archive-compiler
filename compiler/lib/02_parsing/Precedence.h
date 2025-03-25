#pragma once

#include "01_tokenize/token/TokenType.h"

namespace nugdev::compiler::ast {

enum class Precedence {
    Lowest = 1,
    In,           // in
    Equals,       // ==
    LessGreater,  // > or <
    Sum,          // +
    Product,      // *
    Postfix,      // x++, x--
    Prefix,       // -X, !X ++x, --x
    Call,         // myFunction(X)
    Index,        // array[index]
    Unknown,
};

Precedence get_precedence(tokenize::TokenType type);

}  // namespace nugdev::compiler::ast
