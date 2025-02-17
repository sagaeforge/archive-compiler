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
    Assign = '=',
    Plus = '+',
    Minus = '-',
    Bang = '!',
    Asterisk = '*',
    Slash = '/',

    LessThan = '<',
    GreaterThan = '>',

    // Delimiters
    Comma = ',',
    SemiColon = ';',
    Colon = ':',

    LParen = '(',
    RParen = ')',
    LBrace = '{',
    RBrace = '}',
    LBracket = '[',
    RBracket = ']',

    Eq = 129,
    NotEq,
    Inc,
    Dec,

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
