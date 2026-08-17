//
// Created by lambda on 10/25/25.
//

#pragma once

#include "common.h"

/**
 * @brief 토큰 타입
 */
enum class TokenType : uint8_t {
    /**
     * @brief 잘못된 토큰
     * @version 0.0.1a, Milestone 1
     */
    Illegal = 0,

    /**
     * @brief 식별자 토큰
     * @version 0.0.1a, Milestone 1
     */
    Identifier,
    /**
     * @brief 더하기.
     * @details <expression> + <expression>
     * @version 0.0.1a, Milestone 1
     */
    Plus,
    /**
     * @brief 빼기.
     * @details <expression> - <expression>
     * @version 0.0.1a, Milestone 1
     */
    Minus,
    /**
     * @brief 곱하기.
     * @details <expression> * <expression>
     * @version 0.0.1a, Milestone 1
     */
    Multiply,
    /**
     * @brief 나누기.
     * @details <expression> / <expression>
     * @version 0.0.1a, Milestone 1
     */
    Divide,
    /**
     * @brief 콜론.
     * @version 0.0.1a, Milestone 1
     */
    Colon,
    /**
     * @brief 왼쪽 소괄호.
     * @version 0.0.1a, Milestone 1
     */
    LeftParenthesis,
    /**
     * @brief 오른쪽 소괄호.
     * @version 0.0.1a, Milestone 1
     */
    RightParenthesis,
    /**
     * @brief 왼쪽 중괄호.
     * @version 0.0.1a, Milestone 1
     */
    LeftBrace,
    /**
     * @brief 오른쪽 중괄호.
     * @version 0.0.1a, Milestone 1
     */
    RightBrace,
    /**
     * @brief 크다(>)
     * @version 0.0.1a, Milestone 1
     */
    Greater,
    /**
     * @brief 작다(<)
     * @version 0.0.1a, Milestone 1
     */
    Less,
    /**
     * @brief 같다(==)
     * @version 0.0.1a, Milestone 1
     */
    Equal,
    /**
     * @brief 크거나 같다(<=)
     * @version 0.0.1a, Milestone 1
     */
    GreaterEqual,
    /**
     * @brief 크거나 같다(<=)
     * @version 0.0.1a, Milestone 1
     */
    LessEqual,

    /**
     * @brief 변수 토큰
     * @version 0.0.1a, Milestone 1
     */
    Variable,

    /**
     * @brief 함수
     * @version 0.0.1a, Milestone 1
     */
    Function,
    /**
     * @brief 조건문
     * @version 0.0.1a, Milestone 1
     */
    If,
    /**
     * @brief return
     * @version 0.0.1a, Milestone 1
     */
    Return,

    /**
     * @brief 문자열
     * @version 0.0.1a, Milestone 1
     */
    String,
    /**
     * @brief 숫자
     * @version 0.0.1a, Milestone 1
     */
    Number,
    /**
     * @brief 할당 연산자
     * @version 0.0.1a, Milestone 1
     */
    Assign,
    /**
     * @brief else-if
     * @version 0.0.1a, Milestone 1
     */
    Elif,
    /**
     * @brief else
     * @version 0.0.1a, Milestone 1
     */
    Else,
    /**
     * @brief Comma
     * @version 0.0.1a, Milestone 1
     */
    Comma,
    /**
     * @brief Null
     * @version 0.0.1a, Milestone 1
     */
    Null,
};
