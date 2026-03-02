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
    KwOffsetof,
    KwTrait,
    KwImpl,
    KwConst,
    KwDyn,
    KwWith,
    KwOwn,
    KwStatic,
    KwPub,
    KwExtern,
    KwNull,
    KwFor,
    KwIn,
    KwWhile,

    // Operators
    Plus,        // +
    PlusWrap,    // +%
    PlusSat,     // +|
    Minus,       // -
    MinusWrap,   // -%
    MinusSat,    // -|
    Star,        // *
    StarWrap,    // *%
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
    // Compound assignment
    PlusEq,      // +=
    MinusEq,     // -=
    StarEq,      // *=
    SlashEq,     // /=
    PercentEq,   // %=
    PipeEq,      // |=
    AmpEq,       // &=
    CaretEq,     // ^=
    ShlEq,       // <<=
    ShrEq,       // >>=
    Lt,          // <
    Gt,          // >
    LtEq,        // <=
    GtEq,        // >=
    Arrow,       // ->
    FatArrow,    // =>
    DotDot,      // ..
    DotDotEq,    // ..=
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

    // Labels
    Label,       // 'ident (loop label, e.g. 'outer)

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
