#pragma once
#include "kern/support/SourceLocation.h"
#include <cstdint>
#include <string_view>

namespace kern {

enum class TokenKind : uint8_t {
    // Literals
    IntLit,
    FloatLit,
    StringLit,

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
    KwStruct,
    KwEnum,
    KwUnion,
    KwLoop,
    KwBreak,
    KwContinue,
    KwAs,
    KwAsm,
    KwVolatile,
    KwNoreturn,
    KwType,
    KwNewtype,
    KwModule,
    KwImport,
    KwSizeof,
    KwAlignof,
    KwTrait,
    KwImpl,
    KwConst,

    // Operators
    Plus,        // +
    Minus,       // -
    Star,        // *
    Slash,       // /
    Percent,     // %
    BitOr,       // | (bitwise OR, distinct from |> pipe)
    BitXor,      // ^
    Tilde,       // ~
    Shl,         // <<
    Shr,         // >>
    Exclaim,     // !
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
    ColonColon,  // ::
    Dot,         // .
    Pipe,        // |>
    Ampersand,   // &
    At,          // @
    Question,    // ?
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
