#pragma once

#include <unicode/unistr.h>

namespace nugdev::compiler::tokenize {

enum class TokenType {
    Illegal,
    EoF,

    // Identifiers + literals
    Ident,  // add, foobar, x, y, ...
    Int,    // 1343456
    String, // "foobar"

    // Operators
    Assign,
    Plus,
    Minus,
    Bang,
    Asterisk,
    Slash,

    LessThan,
    GreaterThan,

    Eq,
    NotEq,

    // Delimiters
    Comma,
    SemiColon,
    Colon,

    LParen,
    RParen,
    LBrace,
    RBrace,
    LBracket,
    RBracket,

    // Keywords
    Function,
    Let,
    True,
    False,
    If,
    Else,
    Return,
};

class Token {
  public:
    icu::UnicodeString to_str();

  public:
    static Token Empty();
    static Token from(TokenType type, icu::UnicodeString literal);

  private:
    Token(TokenType type, icu::UnicodeString literal);

  private:
    TokenType type;
    icu::UnicodeString literal;
};

} // namespace nugdev::compiler::tokenize
