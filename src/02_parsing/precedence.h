//
// Created by nugde on 25. 10. 10..
//

#pragma once

#include "01_tokenize/token/token_type.h"

enum class precedence {
    Lowest = 1,
    Assignment, // =, +=, -=, *=, /=, %=
    Disjunction, // ||
    Conjunction, // &&
    Equality, // ==, !=
    Comparison, // <, >, <=, >=
    InIs, // in, !in, is, !is
    Elvis, // ?:
    Infix, // 사용자 정의 infix 함수
    Range, // ..
    Additive, // +, -
    Multiplicative, // *, /, %
    AsIs, // as, as?, :
    Prefix, // -, +, ++, --, !, label@
    Postfix, // ++, --, ., ?., ?
    Call, // myFunction(X)
    Index, // array[index]
    Unknown,
};

precedence getPrecedence(TokenType type);
