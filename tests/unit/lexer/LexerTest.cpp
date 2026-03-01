#include "kern/lexer/Lexer.h"
#include <gtest/gtest.h>
#include <vector>

using namespace kern;

// Holds source string to keep string_views in tokens valid
struct LexResult {
    std::string source;
    std::vector<Token> tokens;

    const Token& operator[](size_t i) const { return tokens[i]; }
    size_t size() const { return tokens.size(); }
};

static LexResult lex(std::string src, bool skip_newlines = true) {
    LexResult r;
    r.source = std::move(src);

    DiagnosticEngine diag;
    Lexer lexer(r.source, "test.kern", diag);
    while (true) {
        Token tok = lexer.nextToken();
        if (skip_newlines && tok.kind == TokenKind::Newline) continue;
        r.tokens.push_back(tok);
        if (tok.kind == TokenKind::Eof) break;
    }
    return r;
}

static LexResult lexWithErrors(std::string src, DiagnosticEngine& diag) {
    LexResult r;
    r.source = std::move(src);

    Lexer lexer(r.source, "test.kern", diag);
    while (true) {
        Token tok = lexer.nextToken();
        r.tokens.push_back(tok);
        if (tok.kind == TokenKind::Eof) break;
    }
    return r;
}

// ===== Existing tests =====

TEST(LexerTest, BasicTokens) {
    auto r = lex("fn fib(n: i64) -> i64 { }");
    ASSERT_GE(r.size(), 10u);
    EXPECT_EQ(r[0].kind, TokenKind::KwFn);
    EXPECT_EQ(r[1].kind, TokenKind::Ident);
    EXPECT_EQ(r[1].text, "fib");
    EXPECT_EQ(r[2].kind, TokenKind::LParen);
    EXPECT_EQ(r[3].kind, TokenKind::Ident);
    EXPECT_EQ(r[3].text, "n");
    EXPECT_EQ(r[4].kind, TokenKind::Colon);
    EXPECT_EQ(r[5].kind, TokenKind::Ident);
    EXPECT_EQ(r[5].text, "i64");
    EXPECT_EQ(r[6].kind, TokenKind::RParen);
    EXPECT_EQ(r[7].kind, TokenKind::Arrow);
    EXPECT_EQ(r[8].kind, TokenKind::Ident);
    EXPECT_EQ(r[9].kind, TokenKind::LBrace);
    EXPECT_EQ(r[10].kind, TokenKind::RBrace);
}

TEST(LexerTest, IntegerLiterals) {
    auto r = lex("42 0xFF 100");
    EXPECT_EQ(r[0].kind, TokenKind::IntLit);
    EXPECT_EQ(r[0].text, "42");
    EXPECT_EQ(r[1].kind, TokenKind::IntLit);
    EXPECT_EQ(r[1].text, "0xFF");
    EXPECT_EQ(r[2].kind, TokenKind::IntLit);
    EXPECT_EQ(r[2].text, "100");
}

TEST(LexerTest, Operators) {
    auto r = lex("+ - * / == != < <= > >= -> => |>");
    EXPECT_EQ(r[0].kind, TokenKind::Plus);
    EXPECT_EQ(r[1].kind, TokenKind::Minus);
    EXPECT_EQ(r[2].kind, TokenKind::Star);
    EXPECT_EQ(r[3].kind, TokenKind::Slash);
    EXPECT_EQ(r[4].kind, TokenKind::EqEq);
    EXPECT_EQ(r[5].kind, TokenKind::NotEq);
    EXPECT_EQ(r[6].kind, TokenKind::Lt);
    EXPECT_EQ(r[7].kind, TokenKind::LtEq);
    EXPECT_EQ(r[8].kind, TokenKind::Gt);
    EXPECT_EQ(r[9].kind, TokenKind::GtEq);
    EXPECT_EQ(r[10].kind, TokenKind::Arrow);
    EXPECT_EQ(r[11].kind, TokenKind::FatArrow);
    EXPECT_EQ(r[12].kind, TokenKind::Pipe);
}

TEST(LexerTest, Keywords) {
    auto r = lex("fn val var match return if else and or not true false");
    EXPECT_EQ(r[0].kind, TokenKind::KwFn);
    EXPECT_EQ(r[1].kind, TokenKind::KwVal);
    EXPECT_EQ(r[2].kind, TokenKind::KwVar);
    EXPECT_EQ(r[3].kind, TokenKind::KwMatch);
    EXPECT_EQ(r[4].kind, TokenKind::KwReturn);
    EXPECT_EQ(r[5].kind, TokenKind::KwIf);
    EXPECT_EQ(r[6].kind, TokenKind::KwElse);
    EXPECT_EQ(r[7].kind, TokenKind::KwAnd);
    EXPECT_EQ(r[8].kind, TokenKind::KwOr);
    EXPECT_EQ(r[9].kind, TokenKind::KwNot);
    EXPECT_EQ(r[10].kind, TokenKind::KwTrue);
    EXPECT_EQ(r[11].kind, TokenKind::KwFalse);
}

TEST(LexerTest, LineComment) {
    auto r = lex("42 // this is a comment\n55");
    EXPECT_EQ(r[0].kind, TokenKind::IntLit);
    EXPECT_EQ(r[0].text, "42");
    EXPECT_EQ(r[1].kind, TokenKind::IntLit);
    EXPECT_EQ(r[1].text, "55");
}

TEST(LexerTest, BlockComment) {
    auto r = lex("42 /* block */ 55");
    EXPECT_EQ(r[0].kind, TokenKind::IntLit);
    EXPECT_EQ(r[1].kind, TokenKind::IntLit);
}

TEST(LexerTest, SourceLocation) {
    auto r = lex("fn\nfib");
    EXPECT_EQ(r[0].loc.line, 1u);
    EXPECT_EQ(r[0].loc.col, 1u);
    EXPECT_EQ(r[1].loc.line, 2u);
    EXPECT_EQ(r[1].loc.col, 1u);
}

// ===== New TDD tests =====

// --- Edge case: empty source ---
TEST(LexerTest, EmptySource) {
    auto r = lex("");
    ASSERT_EQ(r.size(), 1u);
    EXPECT_EQ(r[0].kind, TokenKind::Eof);
}

// --- Edge case: whitespace only ---
TEST(LexerTest, WhitespaceOnly) {
    auto r = lex("   \t   ");
    ASSERT_EQ(r.size(), 1u);
    EXPECT_EQ(r[0].kind, TokenKind::Eof);
}

// --- Edge case: single character tokens ---
TEST(LexerTest, Delimiters) {
    auto r = lex("( ) { } [ ]");
    EXPECT_EQ(r[0].kind, TokenKind::LParen);
    EXPECT_EQ(r[1].kind, TokenKind::RParen);
    EXPECT_EQ(r[2].kind, TokenKind::LBrace);
    EXPECT_EQ(r[3].kind, TokenKind::RBrace);
    EXPECT_EQ(r[4].kind, TokenKind::LBracket);
    EXPECT_EQ(r[5].kind, TokenKind::RBracket);
}

TEST(LexerTest, Punctuation) {
    auto r = lex(": , ; . &");
    EXPECT_EQ(r[0].kind, TokenKind::Colon);
    EXPECT_EQ(r[1].kind, TokenKind::Comma);
    EXPECT_EQ(r[2].kind, TokenKind::Semicolon);
    EXPECT_EQ(r[3].kind, TokenKind::Dot);
    EXPECT_EQ(r[4].kind, TokenKind::Ampersand);
}

// --- Edge case: hex literals ---
TEST(LexerTest, HexLiterals) {
    auto r = lex("0x0 0xABCDEF 0X1a2B");
    ASSERT_GE(r.size(), 4u); // 3 tokens + Eof
    EXPECT_EQ(r[0].kind, TokenKind::IntLit);
    EXPECT_EQ(r[0].text, "0x0");
    EXPECT_EQ(r[1].kind, TokenKind::IntLit);
    EXPECT_EQ(r[1].text, "0xABCDEF");
    EXPECT_EQ(r[2].kind, TokenKind::IntLit);
    EXPECT_EQ(r[2].text, "0X1a2B");
}

// --- Edge case: zero ---
TEST(LexerTest, ZeroLiteral) {
    auto r = lex("0");
    EXPECT_EQ(r[0].kind, TokenKind::IntLit);
    EXPECT_EQ(r[0].text, "0");
}

// --- Edge case: identifier with underscores ---
TEST(LexerTest, IdentifierWithUnderscores) {
    auto r = lex("_foo bar_baz _123 __init__");
    EXPECT_EQ(r[0].kind, TokenKind::Ident);
    EXPECT_EQ(r[0].text, "_foo");
    EXPECT_EQ(r[1].kind, TokenKind::Ident);
    EXPECT_EQ(r[1].text, "bar_baz");
    EXPECT_EQ(r[2].kind, TokenKind::Ident);
    EXPECT_EQ(r[2].text, "_123");
    EXPECT_EQ(r[3].kind, TokenKind::Ident);
    EXPECT_EQ(r[3].text, "__init__");
}

// --- Edge case: keyword-prefixed identifiers ---
TEST(LexerTest, KeywordPrefixedIdentifier) {
    auto r = lex("fns values iffy return_val matchup");
    EXPECT_EQ(r[0].kind, TokenKind::Ident);
    EXPECT_EQ(r[0].text, "fns");
    EXPECT_EQ(r[1].kind, TokenKind::Ident);
    EXPECT_EQ(r[1].text, "values");
    EXPECT_EQ(r[2].kind, TokenKind::Ident);
    EXPECT_EQ(r[2].text, "iffy");
    EXPECT_EQ(r[3].kind, TokenKind::Ident);
    EXPECT_EQ(r[3].text, "return_val");
    EXPECT_EQ(r[4].kind, TokenKind::Ident);
    EXPECT_EQ(r[4].text, "matchup");
}

// --- Edge case: nested block comments ---
TEST(LexerTest, NestedBlockComment) {
    auto r = lex("42 /* outer /* inner */ still comment */ 55");
    EXPECT_EQ(r[0].kind, TokenKind::IntLit);
    EXPECT_EQ(r[0].text, "42");
    EXPECT_EQ(r[1].kind, TokenKind::IntLit);
    EXPECT_EQ(r[1].text, "55");
}

// --- Edge case: unterminated block comment ---
TEST(LexerTest, UnterminatedBlockComment) {
    DiagnosticEngine diag;
    auto r = lexWithErrors("42 /* unterminated", diag);
    EXPECT_TRUE(diag.hasErrors());
}

// --- Bare ! is now Exclaim token (bitwise NOT prefix) ---
TEST(LexerTest, BareExclamation) {
    auto r = lex("!");
    ASSERT_GE(r.size(), 2u);
    EXPECT_EQ(r[0].kind, TokenKind::Exclaim);
    EXPECT_EQ(r[1].kind, TokenKind::Eof);
}

// --- Bare | is now BitOr token ---
TEST(LexerTest, BarePipe) {
    auto r = lex("|");
    ASSERT_GE(r.size(), 2u);
    EXPECT_EQ(r[0].kind, TokenKind::BitOr);
    EXPECT_EQ(r[1].kind, TokenKind::Eof);
}

// --- Newlines are tokens (not skipped) ---
TEST(LexerTest, NewlinesAreTokens) {
    auto r = lex("42\n55", /*skip_newlines=*/false);
    ASSERT_GE(r.size(), 4u);
    EXPECT_EQ(r[0].kind, TokenKind::IntLit);
    EXPECT_EQ(r[1].kind, TokenKind::Newline);
    EXPECT_EQ(r[2].kind, TokenKind::IntLit);
    EXPECT_EQ(r[3].kind, TokenKind::Eof);
}

// --- Consecutive newlines collapse ---
TEST(LexerTest, ConsecutiveNewlines) {
    auto r = lex("42\n\n\n55", /*skip_newlines=*/false);
    ASSERT_GE(r.size(), 4u);
    EXPECT_EQ(r[0].kind, TokenKind::IntLit);
    EXPECT_EQ(r[1].kind, TokenKind::Newline);
    EXPECT_EQ(r[2].kind, TokenKind::IntLit);
    EXPECT_EQ(r[3].kind, TokenKind::Eof);
}

// --- Assignment operator = ---
TEST(LexerTest, AssignmentOperator) {
    auto r = lex("x = 42");
    EXPECT_EQ(r[0].kind, TokenKind::Ident);
    EXPECT_EQ(r[1].kind, TokenKind::Eq);
    EXPECT_EQ(r[2].kind, TokenKind::IntLit);
}

// --- Multi-digit numbers ---
TEST(LexerTest, LargeNumber) {
    auto r = lex("999999999");
    EXPECT_EQ(r[0].kind, TokenKind::IntLit);
    EXPECT_EQ(r[0].text, "999999999");
}

// --- Location tracking across multiple lines ---
TEST(LexerTest, MultilineLocationTracking) {
    auto r = lex("fn main() -> i64 {\n    42\n}");
    // 'fn' at line 1 col 1
    EXPECT_EQ(r[0].loc.line, 1u);
    EXPECT_EQ(r[0].loc.col, 1u);
    // '42' at line 2 col 5
    size_t int_idx = 0;
    for (size_t i = 0; i < r.size(); ++i) {
        if (r[i].kind == TokenKind::IntLit) { int_idx = i; break; }
    }
    EXPECT_EQ(r[int_idx].loc.line, 2u);
    EXPECT_EQ(r[int_idx].loc.col, 5u);
}

// --- Comment at end of file ---
TEST(LexerTest, CommentAtEndOfFile) {
    auto r = lex("42 // end");
    ASSERT_GE(r.size(), 2u);
    EXPECT_EQ(r[0].kind, TokenKind::IntLit);
    EXPECT_EQ(r[1].kind, TokenKind::Eof);
}

// --- Tokens without spaces ---
TEST(LexerTest, TokensWithoutSpaces) {
    auto r = lex("fn(a:i64)->i64{a}");
    EXPECT_EQ(r[0].kind, TokenKind::KwFn);
    EXPECT_EQ(r[1].kind, TokenKind::LParen);
    EXPECT_EQ(r[2].kind, TokenKind::Ident);
    EXPECT_EQ(r[3].kind, TokenKind::Colon);
    EXPECT_EQ(r[4].kind, TokenKind::Ident);
    EXPECT_EQ(r[5].kind, TokenKind::RParen);
    EXPECT_EQ(r[6].kind, TokenKind::Arrow);
    EXPECT_EQ(r[7].kind, TokenKind::Ident);
    EXPECT_EQ(r[8].kind, TokenKind::LBrace);
    EXPECT_EQ(r[9].kind, TokenKind::Ident);
    EXPECT_EQ(r[10].kind, TokenKind::RBrace);
}

// ===== Coverage improvement tests =====

// --- source() accessor ---
TEST(LexerTest, SourceAccessor) {
    std::string src = "fn main() -> i64 { 42 }";
    DiagnosticEngine diag;
    Lexer lexer(src, "test.kern", diag);
    EXPECT_EQ(lexer.source(), src);
}

// --- tokenKindName covers all enum values ---
TEST(LexerTest, TokenKindNameCoverage) {
    TokenKind kinds[] = {
        TokenKind::IntLit, TokenKind::FloatLit, TokenKind::StringLit,
        TokenKind::Ident,
        TokenKind::KwFn, TokenKind::KwVal, TokenKind::KwVar,
        TokenKind::KwMatch, TokenKind::KwReturn,
        TokenKind::KwIf, TokenKind::KwElse,
        TokenKind::KwAnd, TokenKind::KwOr, TokenKind::KwNot,
        TokenKind::KwTrue, TokenKind::KwFalse,
        TokenKind::KwStruct, TokenKind::KwEnum, TokenKind::KwUnion,
        TokenKind::Plus, TokenKind::Minus, TokenKind::Star, TokenKind::Slash,
        TokenKind::Eq, TokenKind::EqEq, TokenKind::NotEq,
        TokenKind::Lt, TokenKind::Gt, TokenKind::LtEq, TokenKind::GtEq,
        TokenKind::Arrow, TokenKind::FatArrow,
        TokenKind::Colon, TokenKind::ColonColon,
        TokenKind::Dot, TokenKind::Pipe, TokenKind::Ampersand,
        TokenKind::Comma, TokenKind::Semicolon,
        TokenKind::LParen, TokenKind::RParen,
        TokenKind::LBrace, TokenKind::RBrace,
        TokenKind::LBracket, TokenKind::RBracket,
        TokenKind::Newline, TokenKind::Eof, TokenKind::Error,
    };
    for (auto k : kinds) {
        const char* name = tokenKindName(k);
        ASSERT_NE(name, nullptr);
        EXPECT_STRNE(name, "?");
    }
}

// --- Tab column tracking ---
TEST(LexerTest, TabColumnTracking) {
    auto r = lex("\ta");
    EXPECT_EQ(r[0].kind, TokenKind::Ident);
    EXPECT_EQ(r[0].loc.col, 2u); // tab advances col
}

// --- Carriage return handling ---
TEST(LexerTest, CarriageReturnHandling) {
    auto r = lex("a\r\nb");
    EXPECT_EQ(r[0].kind, TokenKind::Ident);
    EXPECT_EQ(r[0].text, "a");
    EXPECT_EQ(r[1].kind, TokenKind::Ident);
    EXPECT_EQ(r[1].text, "b");
    EXPECT_EQ(r[1].loc.line, 2u);
}

// --- Block comment with newlines tracks line ---
TEST(LexerTest, BlockCommentWithNewlines) {
    auto r = lex("42 /* line1\nline2 */ 55");
    EXPECT_EQ(r[0].kind, TokenKind::IntLit);
    EXPECT_EQ(r[0].text, "42");
    EXPECT_EQ(r[1].kind, TokenKind::IntLit);
    EXPECT_EQ(r[1].text, "55");
    EXPECT_EQ(r[1].loc.line, 2u);
}

// --- Line comment at EOF without newline (with newline tokens) ---
TEST(LexerTest, LineCommentAtEOFNoNewline) {
    auto r = lex("42 // end", /*skip_newlines=*/false);
    ASSERT_GE(r.size(), 2u);
    EXPECT_EQ(r[0].kind, TokenKind::IntLit);
    EXPECT_EQ(r[1].kind, TokenKind::Eof);
}

// --- Leading zero decimal ---
TEST(LexerTest, LeadingZeroDecimal) {
    auto r = lex("007");
    EXPECT_EQ(r[0].kind, TokenKind::IntLit);
    EXPECT_EQ(r[0].text, "007");
}

// --- Minus vs Arrow distinction ---
TEST(LexerTest, MinusVsArrow) {
    auto r = lex("a - b -> c");
    EXPECT_EQ(r[0].kind, TokenKind::Ident);
    EXPECT_EQ(r[1].kind, TokenKind::Minus);
    EXPECT_EQ(r[2].kind, TokenKind::Ident);
    EXPECT_EQ(r[3].kind, TokenKind::Arrow);
    EXPECT_EQ(r[4].kind, TokenKind::Ident);
}

// --- Equal variants: =, ==, => ---
TEST(LexerTest, EqualVariants) {
    auto r = lex("= == =>");
    EXPECT_EQ(r[0].kind, TokenKind::Eq);
    EXPECT_EQ(r[1].kind, TokenKind::EqEq);
    EXPECT_EQ(r[2].kind, TokenKind::FatArrow);
}

// --- Unknown character error ---
TEST(LexerTest, UnknownCharacterError) {
    DiagnosticEngine diag;
    auto r = lexWithErrors("$", diag);
    EXPECT_TRUE(diag.hasErrors());
    bool found_error = false;
    for (auto& t : r.tokens) {
        if (t.kind == TokenKind::Error) found_error = true;
    }
    EXPECT_TRUE(found_error);
}

// --- Number followed by identifier ---
TEST(LexerTest, NumberFollowedByIdent) {
    auto r = lex("42abc");
    EXPECT_EQ(r[0].kind, TokenKind::IntLit);
    EXPECT_EQ(r[0].text, "42");
    EXPECT_EQ(r[1].kind, TokenKind::Ident);
    EXPECT_EQ(r[1].text, "abc");
}

// --- Block comment then newline then token ---
TEST(LexerTest, BlockCommentNewlineThenToken) {
    auto r = lex("/* comment */\n42", /*skip_newlines=*/false);
    EXPECT_EQ(r[0].kind, TokenKind::Newline);
    EXPECT_EQ(r[1].kind, TokenKind::IntLit);
    EXPECT_EQ(r[1].text, "42");
}

// --- Long identifier ---
TEST(LexerTest, LongIdentifier) {
    std::string long_id(256, 'a');
    auto r = lex(long_id);
    EXPECT_EQ(r[0].kind, TokenKind::Ident);
    EXPECT_EQ(r[0].text, long_id);
}

// ===== M5a: struct keyword =====

TEST(LexerTest, StructKeyword) {
    auto r = lex("struct Point { x: i64, y: i64 }");
    EXPECT_EQ(r[0].kind, TokenKind::KwStruct);
    EXPECT_EQ(r[0].text, "struct");
    EXPECT_EQ(r[1].kind, TokenKind::Ident);
    EXPECT_EQ(r[1].text, "Point");
}

TEST(LexerTest, StructIdentifierPrefix) {
    auto r = lex("structure");
    EXPECT_EQ(r[0].kind, TokenKind::Ident);
    EXPECT_EQ(r[0].text, "structure");
}

// ===== M5b: enum, union keywords + :: token =====

TEST(LexerTest, EnumKeyword) {
    auto r = lex("enum Color { Red, Green, Blue }");
    EXPECT_EQ(r[0].kind, TokenKind::KwEnum);
    EXPECT_EQ(r[0].text, "enum");
    EXPECT_EQ(r[1].kind, TokenKind::Ident);
    EXPECT_EQ(r[1].text, "Color");
}

TEST(LexerTest, UnionKeyword) {
    auto r = lex("union Shape { Circle(Circle), Empty }");
    EXPECT_EQ(r[0].kind, TokenKind::KwUnion);
    EXPECT_EQ(r[0].text, "union");
    EXPECT_EQ(r[1].kind, TokenKind::Ident);
    EXPECT_EQ(r[1].text, "Shape");
}

TEST(LexerTest, ColonColonToken) {
    auto r = lex("Shape::Circle");
    EXPECT_EQ(r[0].kind, TokenKind::Ident);
    EXPECT_EQ(r[0].text, "Shape");
    EXPECT_EQ(r[1].kind, TokenKind::ColonColon);
    EXPECT_EQ(r[1].text, "::");
    EXPECT_EQ(r[2].kind, TokenKind::Ident);
    EXPECT_EQ(r[2].text, "Circle");
}

TEST(LexerTest, ColonVsColonColon) {
    auto r = lex(": :: :");
    EXPECT_EQ(r[0].kind, TokenKind::Colon);
    EXPECT_EQ(r[1].kind, TokenKind::ColonColon);
    EXPECT_EQ(r[2].kind, TokenKind::Colon);
}

TEST(LexerTest, EnumIdentifierPrefix) {
    auto r = lex("enumerate");
    EXPECT_EQ(r[0].kind, TokenKind::Ident);
    EXPECT_EQ(r[0].text, "enumerate");
}

TEST(LexerTest, UnionIdentifierPrefix) {
    auto r = lex("unionize");
    EXPECT_EQ(r[0].kind, TokenKind::Ident);
    EXPECT_EQ(r[0].text, "unionize");
}

// ===== String literal tests =====

TEST(LexerTest, StringLitBasic) {
    auto r = lex("\"hello\"");
    EXPECT_EQ(r[0].kind, TokenKind::StringLit);
    EXPECT_EQ(r[0].text, "\"hello\"");
}

TEST(LexerTest, StringLitEmpty) {
    auto r = lex("\"\"");
    EXPECT_EQ(r[0].kind, TokenKind::StringLit);
    EXPECT_EQ(r[0].text, "\"\"");
}

TEST(LexerTest, StringLitEscapeSequences) {
    auto r = lex("\"a\\nb\\tc\\\\d\\\"e\"");
    EXPECT_EQ(r[0].kind, TokenKind::StringLit);
    EXPECT_EQ(r[0].text, "\"a\\nb\\tc\\\\d\\\"e\"");
}

TEST(LexerTest, StringLitWithSpaces) {
    auto r = lex("\"hello world\"");
    EXPECT_EQ(r[0].kind, TokenKind::StringLit);
    EXPECT_EQ(r[0].text, "\"hello world\"");
}

TEST(LexerTest, StringLitInContext) {
    auto r = lex("val s: String = \"hi\"");
    EXPECT_EQ(r[0].kind, TokenKind::KwVal);
    EXPECT_EQ(r[1].kind, TokenKind::Ident);
    EXPECT_EQ(r[2].kind, TokenKind::Colon);
    EXPECT_EQ(r[3].kind, TokenKind::Ident);
    EXPECT_EQ(r[4].kind, TokenKind::Eq);
    EXPECT_EQ(r[5].kind, TokenKind::StringLit);
    EXPECT_EQ(r[5].text, "\"hi\"");
}

TEST(LexerTest, StringLitUnterminated) {
    auto r = lex("\"hello");
    EXPECT_EQ(r[0].kind, TokenKind::Error);
}

TEST(LexerTest, StringLitUnterminatedNewline) {
    auto r = lex("\"hello\nworld\"", false);
    EXPECT_EQ(r[0].kind, TokenKind::Error);
}

TEST(LexerTest, StringLitInvalidEscape) {
    auto r = lex("\"hello\\xworld\"");
    EXPECT_EQ(r[0].kind, TokenKind::Error);
}
