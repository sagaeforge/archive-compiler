#pragma once

namespace nugdev::compiler::tokenize {

/**
 * @brief 토큰 타입
 */
enum class TokenType {
    Illegal = 1,
    Comment,

    // 특수 문자.
    ExclamationMark = '!',  // !
    Dollar = '$',           // $
    Percent = '%',          // %
    Ampersand = '&',        // &
    LeftParen = '(',        // (
    RightParen = ')',       // )
    Asterisk = '*',         // *
    Plus = '+',             // +
    Comma = ',',            // ,
    Minus = '-',            // -
    Period = '.',           // .
    Slash = '/',            // /
    Colon = ':',            // :
    SemiColon = ';',        // ;
    LeftAngle = '<',        // <
    Assign = '=',           // =
    RightAngle = '>',       // >
    QuestionMark = '?',     // ?
    At = '@',               // @
    LeftBracket = '[',      // [
    RightBracket = ']',     // ]
    Caret = '^',            // ^
    LeftBrace = '{',        // {
    RightBrace = '}',       // }

    // 원시 타입.
    Identifier = 128,  // 식별자 = {_d-zA-Z1-9}+.
    Number,            // 숫자. = {0-9}+.
    String,            // 문자열. = "..." | '...'.
    Boolean,           // 불리언[true, false].

    // 연산자.
    In,
    And,
    Or,
    Equal,
    NotEqual,
    GreaterEqual,
    LessEqual,
    PlusEqual,
    MinusEqual,
    Increment,
    Decrement,

    // 키워드.
    Let,
    If,
    Elif,
    Else,
    When,
    For,
    Break,
    Continue,
    Function,
    Return,
    Struct,
};

}  // namespace nugdev::compiler::tokenize
