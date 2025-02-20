#pragma once

#include <unicode/unistr.h>

namespace nugdev::compiler::tokenize {

enum class TokenType {
    Illegal,
    EoF,

    // Identifiers + literals
    Ident,  // add, foobar, x, y, ...
    Number, // 1343456
    String, // "foobar"

    // Operators
    ExclamationMark = '!', // 33
    Hash = '#',            // 35
    Dollar = '$',          // 36
    Percent = '%',         // 37
    Ampersand = '&',       // 38
    LParen = '(',          // 40
    RParen = ')',          // 41
    Asterisk = '*',        // 42
    Plus = '+',            // 43
    Comma = ',',           // 44
    Minus = '-',           // 45
    Period = '.',          // 46
    Slash = '/',           // 47

    // ASCII 48-57 : Numbers 0-9 (if needed)

    // ASCII 58-64
    Colon = ':',        // 58
    SemiColon = ';',    // 59
    LessThan = '<',     // 60
    Assign = '=',       // 61
    GreaterThan = '>',  // 62
    QuestionMark = '?', // 63
    At = '@',           // 64

    // ASCII 65-90 : Uppercase letters (if needed)

    // ASCII 91-96
    LBracket = '[', // 91
    RBracket = ']', // 93
    Caret = '^',    // 94
    // BackTick = '`', // 96

    // ASCII 97-122 : Lowercase letters (if needed)

    // ASCII 123-126
    LBrace = '{', // 123
    Pipe = '|',   // 124
    RBrace = '}', // 125
    Tilde = '~',  // 126

    // 2글자 이상 연산자.
    Equal = 129,
    NotEqual,
    Inc,
    Dec,
    PlusEqual,
    MinusEqual,
    LeftArrow,

    // Keywords
    Function,
    Let,
    True,
    False,
    If,
    Elif,
    Else,
    Return,
    For,
    Break,
    Continue,
    Struct,
    When,
    Override,
};

class Token {
  public:
    icu::UnicodeString to_str();

  public:
    static Token empty();
    static Token from(TokenType type, icu::UnicodeString literal);

  private:
    Token(TokenType type, icu::UnicodeString literal);

  public:
    TokenType get_type() const;
    icu::UnicodeString get_literal() const;

  private:
    TokenType type;
    icu::UnicodeString literal;
};

} // namespace nugdev::compiler::tokenize

namespace std {
std::string to_string(const nugdev::compiler::tokenize::TokenType &type);
} // namespace std
