//
// Created by nugde on 25. 10. 10..
//

#include "precedence.h"

precedence getPrecedence(const TokenType type) {
    switch (type) {
        // Assignment (가장 낮음)
        case TokenType::Assign:
            return precedence::Assignment;


        // Equality (==, !=)
        case TokenType::Equal:
            return precedence::Equality;

        // Comparison (<, >, <=, >=)
        case TokenType::LessEqual:
        case TokenType::GreaterEqual:
            return precedence::Comparison;

        // // Elvis (?:)
        // case TokenType::Elvis:
        //     return Precedence::Elvis;

        // Infix (사용자 정의 infix 함수 - 필요시)
        // case TokenType::Infix:
        //   return Precedence::Infix;

        // // Range (..)
        // case TokenType::Range:
        //   return Precedence::Range;

        // Additive (+, -)
        case TokenType::Plus:
        case TokenType::Minus:
            return precedence::Additive;

        // Multiplicative (*, /, %)
        case TokenType::Multiply:
        case TokenType::Divide:
            return precedence::Multiplicative;

        // // Type casting (as, as?, :)
        // case TokenType::As:
        // case TokenType::AsNullable:
        // case TokenType::Colon:
        //   return Precedence::AsIs;


        // Call/Index
        case TokenType::LeftParenthesis:
            return precedence::Call;

        default:
            return precedence::Lowest;
    }
}
