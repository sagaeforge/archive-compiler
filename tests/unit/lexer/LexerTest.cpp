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

// --- Error: bare ! character ---
TEST(LexerTest, BareExclamation) {
    DiagnosticEngine diag;
    auto r = lexWithErrors("!", diag);
    EXPECT_TRUE(diag.hasErrors());
    bool found_error = false;
    for (auto& t : r.tokens) {
        if (t.kind == TokenKind::Error) found_error = true;
    }
    EXPECT_TRUE(found_error);
}

// --- Error: bare | character ---
TEST(LexerTest, BarePipe) {
    DiagnosticEngine diag;
    auto r = lexWithErrors("|", diag);
    EXPECT_TRUE(diag.hasErrors());
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
