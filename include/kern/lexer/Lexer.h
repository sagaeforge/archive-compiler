#pragma once
#include "kern/lexer/Token.h"
#include "kern/support/Diagnostic.h"
#include <string_view>

namespace kern {

class Lexer {
public:
    Lexer(std::string_view source, std::string_view filename, DiagnosticEngine& diag);

    Token nextToken();
    const std::string_view& source() const { return source_; }

    // Save/restore for parser lookahead
    struct Snapshot {
        const char* start;
        const char* current;
        uint32_t line;
        uint32_t col;
        uint32_t token_start_col;
    };
    Snapshot save() const {
        return {start_, current_, line_, col_, token_start_col_};
    }
    void restore(const Snapshot& s) {
        start_ = s.start;
        current_ = s.current;
        line_ = s.line;
        col_ = s.col;
        token_start_col_ = s.token_start_col;
    }

private:
    char peek() const;
    char peekNext() const;
    char advance();
    bool isAtEnd() const;
    bool match(char expected);

    void skipWhitespaceAndComments();
    void skipLineComment();
    void skipBlockComment();

    Token makeToken(TokenKind kind);
    Token errorToken(const char* message);

    Token scanNumber();
    Token scanString();
    Token scanCString();
    Token scanIdentifierOrKeyword();
    static TokenKind identifierKind(std::string_view text);

    std::string_view source_;
    std::string_view filename_;
    DiagnosticEngine& diag_;

    const char* start_   = nullptr;
    const char* current_ = nullptr;
    const char* end_     = nullptr;
    uint32_t line_ = 1;
    uint32_t col_  = 1;
    uint32_t token_start_col_ = 1;
};

} // namespace kern
