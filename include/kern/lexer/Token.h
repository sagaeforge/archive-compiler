#pragma once
#include "kern/support/SourceLocation.h"
#include <cstdint>
#include <string_view>

namespace kern {

enum class TokenKind : uint8_t {
    // Literals
    IntLit,
    FloatLit,

    // Identifiers & Keywords
    Ident,
    KwFn,
    KwVal,
    KwVar,
    KwMatch,
    KwReturn,
    KwIf,
    KwElse,
    KwAnd,
    KwOr,
    KwNot,
    KwTrue,
    KwFalse,

    // Operators
    Plus,        // +
    Minus,       // -
    Star,        // *
    Slash,       // /
    Eq,          // =
    EqEq,        // ==
    NotEq,       // !=
    Lt,          // <
    Gt,          // >
    LtEq,        // <=
    GtEq,        // >=
    Arrow,       // ->
    FatArrow,    // =>
    Colon,       // :
    Dot,         // .
    Pipe,        // |>
    Ampersand,   // &
    Comma,       // ,
    Semicolon,   // ;

    // Delimiters
    LParen,      // (
    RParen,      // )
    LBrace,      // {
    RBrace,      // }
    LBracket,    // [
    RBracket,    // ]

    // Special
    Newline,
    Eof,
    Error,
};

const char* tokenKindName(TokenKind kind);

struct Token {
    TokenKind        kind;
    SourceLocation   loc;
    std::string_view text;
};

} // namespace kern
