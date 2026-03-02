#include "kern/lexer/Lexer.h"
#include <cctype>
#include <cstring>

namespace kern {

const char* tokenKindName(TokenKind kind) {
    switch (kind) {
        case TokenKind::IntLit:     return "IntLit";
        case TokenKind::FloatLit:   return "FloatLit";
        case TokenKind::StringLit:  return "StringLit";
        case TokenKind::Ident:      return "Ident";
        case TokenKind::KwFn:       return "fn";
        case TokenKind::KwVal:      return "val";
        case TokenKind::KwVar:      return "var";
        case TokenKind::KwMatch:    return "match";
        case TokenKind::KwReturn:   return "return";
        case TokenKind::KwIf:       return "if";
        case TokenKind::KwElse:     return "else";
        case TokenKind::KwAnd:      return "and";
        case TokenKind::KwOr:       return "or";
        case TokenKind::KwNot:      return "not";
        case TokenKind::KwTrue:     return "true";
        case TokenKind::KwFalse:    return "false";
        case TokenKind::KwStruct:   return "struct";
        case TokenKind::KwEnum:     return "enum";
        case TokenKind::KwUnion:    return "union";
        case TokenKind::KwLoop:     return "loop";
        case TokenKind::KwBreak:    return "break";
        case TokenKind::KwContinue: return "continue";
        case TokenKind::KwAs:       return "as";
        case TokenKind::KwAsm:      return "asm";
        case TokenKind::KwVolatile: return "volatile";
        case TokenKind::KwNoreturn: return "noreturn";
        case TokenKind::KwType:     return "type";
        case TokenKind::KwNewtype:  return "newtype";
        case TokenKind::KwModule:   return "module";
        case TokenKind::KwImport:   return "import";
        case TokenKind::KwSizeof:   return "sizeof";
        case TokenKind::KwAlignof:  return "alignof";
        case TokenKind::KwOffsetof: return "offsetof";
        case TokenKind::KwTrait:    return "trait";
        case TokenKind::KwImpl:     return "impl";
        case TokenKind::KwConst:    return "const";
        case TokenKind::KwDyn:      return "dyn";
        case TokenKind::KwWith:     return "with";
        case TokenKind::KwOwn:      return "own";
        case TokenKind::KwStatic:   return "static";
        case TokenKind::KwPub:      return "pub";
        case TokenKind::KwPriv:     return "priv";
        case TokenKind::KwExtern:   return "extern";
        case TokenKind::KwNull:     return "null";
        case TokenKind::KwFor:      return "for";
        case TokenKind::KwIn:       return "in";
        case TokenKind::KwWhile:    return "while";
        case TokenKind::Plus:       return "+";
        case TokenKind::PlusWrap:   return "+%";
        case TokenKind::PlusSat:    return "+|";
        case TokenKind::Minus:      return "-";
        case TokenKind::MinusWrap:  return "-%";
        case TokenKind::MinusSat:   return "-|";
        case TokenKind::Star:       return "*";
        case TokenKind::StarWrap:   return "*%";
        case TokenKind::Slash:      return "/";
        case TokenKind::Percent:    return "%";
        case TokenKind::BitOr:      return "|";
        case TokenKind::BitXor:     return "^";
        case TokenKind::Tilde:      return "~";
        case TokenKind::Shl:        return "<<";
        case TokenKind::Shr:        return ">>";
        case TokenKind::Exclaim:    return "!";
        case TokenKind::Eq:         return "=";
        case TokenKind::EqEq:       return "==";
        case TokenKind::NotEq:      return "!=";
        case TokenKind::PlusEq:     return "+=";
        case TokenKind::MinusEq:    return "-=";
        case TokenKind::StarEq:     return "*=";
        case TokenKind::SlashEq:    return "/=";
        case TokenKind::PercentEq:  return "%=";
        case TokenKind::PipeEq:     return "|=";
        case TokenKind::AmpEq:      return "&=";
        case TokenKind::CaretEq:    return "^=";
        case TokenKind::ShlEq:      return "<<=";
        case TokenKind::ShrEq:      return ">>=";
        case TokenKind::Lt:         return "<";
        case TokenKind::Gt:         return ">";
        case TokenKind::LtEq:       return "<=";
        case TokenKind::GtEq:       return ">=";
        case TokenKind::Arrow:      return "->";
        case TokenKind::FatArrow:   return "=>";
        case TokenKind::Colon:      return ":";
        case TokenKind::ColonColon: return "::";
        case TokenKind::DotDot:     return "..";
        case TokenKind::DotDotEq:   return "..=";
        case TokenKind::Dot:        return ".";
        case TokenKind::Pipe:       return "|>";
        case TokenKind::Ampersand:  return "&";
        case TokenKind::At:         return "@";
        case TokenKind::Question:   return "?";
        case TokenKind::Comma:      return ",";
        case TokenKind::Semicolon:  return ";";
        case TokenKind::LParen:     return "(";
        case TokenKind::RParen:     return ")";
        case TokenKind::LBrace:     return "{";
        case TokenKind::RBrace:     return "}";
        case TokenKind::LBracket:   return "[";
        case TokenKind::RBracket:   return "]";
        case TokenKind::Label:      return "Label";
        case TokenKind::Newline:    return "Newline";
        case TokenKind::Eof:        return "Eof";
        case TokenKind::Error:      return "Error";
    }
    return "?";
}

Lexer::Lexer(std::string_view source, std::string_view filename, DiagnosticEngine& diag)
    : source_(source), filename_(filename), diag_(diag),
      start_(source.data()), current_(source.data()), end_(source.data() + source.size()) {}

char Lexer::peek() const {
    if (isAtEnd()) return '\0';
    return *current_;
}

char Lexer::peekNext() const {
    if (current_ + 1 >= end_) return '\0';
    return *(current_ + 1);
}

char Lexer::advance() {
    char c = *current_++;
    if (c == '\n') {
        line_++;
        col_ = 1;
    } else {
        col_++;
    }
    return c;
}

bool Lexer::isAtEnd() const {
    return current_ >= end_;
}

bool Lexer::match(char expected) {
    if (isAtEnd() || *current_ != expected) return false;
    advance();
    return true;
}

void Lexer::skipWhitespaceAndComments() {
    while (!isAtEnd()) {
        char c = peek();
        if (c == ' ' || c == '\t' || c == '\r') {
            advance();
        } else if (c == '\n') {
            break; // newlines are significant for statement separation
        } else if (c == '/' && peekNext() == '/') {
            skipLineComment();
        } else if (c == '/' && peekNext() == '*') {
            skipBlockComment();
        } else {
            break;
        }
    }
}

void Lexer::skipLineComment() {
    advance(); // /
    advance(); // /
    while (!isAtEnd() && peek() != '\n') {
        advance();
    }
}

void Lexer::skipBlockComment() {
    advance(); // /
    advance(); // *
    int depth = 1;
    while (!isAtEnd() && depth > 0) {
        if (peek() == '/' && peekNext() == '*') {
            advance(); advance();
            depth++;
        } else if (peek() == '*' && peekNext() == '/') {
            advance(); advance();
            depth--;
        } else {
            advance();
        }
    }
    if (depth != 0) {
        diag_.error({line_, col_, filename_}, "unterminated block comment");
    }
}

Token Lexer::makeToken(TokenKind kind) {
    std::string_view text(start_, static_cast<size_t>(current_ - start_));
    return {kind, {line_, token_start_col_, filename_}, text};
}

Token Lexer::errorToken(const char* message) {
    SourceLocation loc{line_, token_start_col_, filename_};
    diag_.error(loc, message);
    std::string_view text(start_, static_cast<size_t>(current_ - start_));
    return {TokenKind::Error, loc, text};
}

Token Lexer::scanNumber() {
    // Note: the first digit was already consumed by nextToken()
    if (*start_ == '0' && (peek() == 'x' || peek() == 'X')) {
        advance(); // consume 'x'/'X'
        if (isAtEnd() || !std::isxdigit(static_cast<unsigned char>(peek()))) {
            return errorToken("hex literal requires at least one digit after '0x'");
        }
        while (!isAtEnd() && (std::isxdigit(static_cast<unsigned char>(peek())) || peek() == '_')) {
            advance();
        }
        goto scan_int_suffix;
    }

    if (*start_ == '0' && (peek() == 'b' || peek() == 'B')) {
        advance(); // consume 'b'/'B'
        if (isAtEnd() || (peek() != '0' && peek() != '1')) {
            return errorToken("binary literal requires at least one digit after '0b'");
        }
        while (!isAtEnd() && (peek() == '0' || peek() == '1' || peek() == '_')) {
            advance();
        }
        goto scan_int_suffix;
    }

    if (*start_ == '0' && (peek() == 'o' || peek() == 'O')) {
        advance(); // consume 'o'/'O'
        if (isAtEnd() || peek() < '0' || peek() > '7') {
            return errorToken("octal literal requires at least one digit after '0o'");
        }
        while (!isAtEnd() && ((peek() >= '0' && peek() <= '7') || peek() == '_')) {
            advance();
        }
        goto scan_int_suffix;
    }

    while (!isAtEnd() && (std::isdigit(static_cast<unsigned char>(peek())) || peek() == '_')) {
        advance();
    }

    // Check for float: decimal point followed by digit
    if (!isAtEnd() && peek() == '.' && (current_ + 1 < end_) &&
        std::isdigit(static_cast<unsigned char>(peekNext()))) {
        advance(); // consume '.'
        while (!isAtEnd() && std::isdigit(static_cast<unsigned char>(peek()))) {
            advance();
        }
        // Optional 'f' suffix for f32
        if (!isAtEnd() && peek() == 'f') {
            advance();
        }
        return makeToken(TokenKind::FloatLit);
    }

scan_int_suffix:
    // Check for integer type suffix: u8, u16, u32, u64, i8, i16, i32, i64
    if (!isAtEnd() && (peek() == 'u' || peek() == 'i')) {
        const char* suffix_start = current_;
        advance(); // consume 'u' or 'i'
        // Must be followed by 8, 16, 32, or 64
        if (!isAtEnd() && std::isdigit(static_cast<unsigned char>(peek()))) {
            while (!isAtEnd() && std::isdigit(static_cast<unsigned char>(peek()))) {
                advance();
            }
            // Verify it's a valid suffix width
            std::string_view suffix(suffix_start, static_cast<size_t>(current_ - suffix_start));
            if (suffix != "u8" && suffix != "u16" && suffix != "u32" && suffix != "u64" &&
                suffix != "i8" && suffix != "i16" && suffix != "i32" && suffix != "i64") {
                // Not a valid suffix — rewind
                current_ = suffix_start;
            }
        } else {
            // No digit after u/i — rewind
            current_ = suffix_start;
        }
    }

    return makeToken(TokenKind::IntLit);
}

TokenKind Lexer::identifierKind(std::string_view text) {
    if (text == "fn")     return TokenKind::KwFn;
    if (text == "val")    return TokenKind::KwVal;
    if (text == "var")    return TokenKind::KwVar;
    if (text == "match")  return TokenKind::KwMatch;
    if (text == "return") return TokenKind::KwReturn;
    if (text == "if")     return TokenKind::KwIf;
    if (text == "else")   return TokenKind::KwElse;
    if (text == "and")    return TokenKind::KwAnd;
    if (text == "or")     return TokenKind::KwOr;
    if (text == "not")    return TokenKind::KwNot;
    if (text == "true")   return TokenKind::KwTrue;
    if (text == "false")  return TokenKind::KwFalse;
    if (text == "struct")   return TokenKind::KwStruct;
    if (text == "enum")     return TokenKind::KwEnum;
    if (text == "union")    return TokenKind::KwUnion;
    if (text == "loop")     return TokenKind::KwLoop;
    if (text == "break")    return TokenKind::KwBreak;
    if (text == "continue") return TokenKind::KwContinue;
    if (text == "as")       return TokenKind::KwAs;
    if (text == "asm")      return TokenKind::KwAsm;
    if (text == "volatile") return TokenKind::KwVolatile;
    if (text == "noreturn") return TokenKind::KwNoreturn;
    if (text == "type")     return TokenKind::KwType;
    if (text == "newtype")  return TokenKind::KwNewtype;
    if (text == "module")   return TokenKind::KwModule;
    if (text == "import")   return TokenKind::KwImport;
    if (text == "sizeof")   return TokenKind::KwSizeof;
    if (text == "alignof")  return TokenKind::KwAlignof;
    if (text == "offsetof") return TokenKind::KwOffsetof;
    if (text == "trait")    return TokenKind::KwTrait;
    if (text == "impl")     return TokenKind::KwImpl;
    if (text == "const")    return TokenKind::KwConst;
    if (text == "dyn")      return TokenKind::KwDyn;
    if (text == "with")     return TokenKind::KwWith;
    if (text == "own")      return TokenKind::KwOwn;
    if (text == "static")   return TokenKind::KwStatic;
    if (text == "pub")      return TokenKind::KwPub;
    if (text == "priv")     return TokenKind::KwPriv;
    if (text == "extern")   return TokenKind::KwExtern;
    if (text == "null")     return TokenKind::KwNull;
    if (text == "for")      return TokenKind::KwFor;
    if (text == "in")       return TokenKind::KwIn;
    if (text == "while")    return TokenKind::KwWhile;
    return TokenKind::Ident;
}

Token Lexer::scanString() {
    while (!isAtEnd()) {
        char c = peek();
        if (c == '"') {
            advance();
            return makeToken(TokenKind::StringLit);
        }
        if (c == '\\') {
            advance();
            if (isAtEnd()) break;
            char esc = peek();
            if (esc != 'n' && esc != 't' && esc != '\\' && esc != '"') {
                advance();
                return errorToken("invalid escape sequence in string");
            }
            advance();
            continue;
        }
        if (c == '\n') {
            return errorToken("unterminated string literal");
        }
        advance();
    }
    return errorToken("unterminated string literal");
}

Token Lexer::scanIdentifierOrKeyword() {
    while (!isAtEnd() && (std::isalnum(static_cast<unsigned char>(peek())) || peek() == '_')) {
        advance();
    }
    std::string_view text(start_, static_cast<size_t>(current_ - start_));
    return makeToken(identifierKind(text));
}

Token Lexer::nextToken() {
    skipWhitespaceAndComments();

    start_ = current_;
    token_start_col_ = col_;

    if (isAtEnd()) {
        return makeToken(TokenKind::Eof);
    }

    char c = advance();

    // Newline
    if (c == '\n') {
        // Skip consecutive newlines
        while (!isAtEnd() && peek() == '\n') advance();
        return makeToken(TokenKind::Newline);
    }

    // Numbers
    if (std::isdigit(static_cast<unsigned char>(c))) {
        return scanNumber();
    }

    // String literals
    if (c == '"') {
        return scanString();
    }

    // Identifiers and keywords
    if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
        return scanIdentifierOrKeyword();
    }

    // Operators and punctuation
    switch (c) {
        case '(': return makeToken(TokenKind::LParen);
        case ')': return makeToken(TokenKind::RParen);
        case '{': return makeToken(TokenKind::LBrace);
        case '}': return makeToken(TokenKind::RBrace);
        case '[': return makeToken(TokenKind::LBracket);
        case ']': return makeToken(TokenKind::RBracket);
        case ':':
            if (match(':')) return makeToken(TokenKind::ColonColon);
            return makeToken(TokenKind::Colon);
        case ',': return makeToken(TokenKind::Comma);
        case ';': return makeToken(TokenKind::Semicolon);
        case '.':
            if (match('.')) {
                if (match('=')) return makeToken(TokenKind::DotDotEq);
                return makeToken(TokenKind::DotDot);
            }
            return makeToken(TokenKind::Dot);
        case '&':
            if (match('=')) return makeToken(TokenKind::AmpEq);
            return makeToken(TokenKind::Ampersand);
        case '+':
            if (match('%')) return makeToken(TokenKind::PlusWrap);
            if (match('|')) return makeToken(TokenKind::PlusSat);
            if (match('=')) return makeToken(TokenKind::PlusEq);
            return makeToken(TokenKind::Plus);
        case '*':
            if (match('%')) return makeToken(TokenKind::StarWrap);
            if (match('=')) return makeToken(TokenKind::StarEq);
            return makeToken(TokenKind::Star);
        case '/':
            if (match('=')) return makeToken(TokenKind::SlashEq);
            return makeToken(TokenKind::Slash);

        case '-':
            if (match('>')) return makeToken(TokenKind::Arrow);
            if (match('%')) return makeToken(TokenKind::MinusWrap);
            if (match('|')) return makeToken(TokenKind::MinusSat);
            if (match('=')) return makeToken(TokenKind::MinusEq);
            return makeToken(TokenKind::Minus);

        case '=':
            if (match('=')) return makeToken(TokenKind::EqEq);
            if (match('>')) return makeToken(TokenKind::FatArrow);
            return makeToken(TokenKind::Eq);

        case '%':
            if (match('=')) return makeToken(TokenKind::PercentEq);
            return makeToken(TokenKind::Percent);
        case '^':
            if (match('=')) return makeToken(TokenKind::CaretEq);
            return makeToken(TokenKind::BitXor);
        case '~': return makeToken(TokenKind::Tilde);
        case '@': return makeToken(TokenKind::At);
        case '?': return makeToken(TokenKind::Question);

        case '\'':
            // Label: 'ident
            if (std::isalpha(static_cast<unsigned char>(peek())) || peek() == '_') {
                while (!isAtEnd() && (std::isalnum(static_cast<unsigned char>(peek())) || peek() == '_')) advance();
                return makeToken(TokenKind::Label);
            }
            return makeToken(TokenKind::Error);

        case '!':
            if (match('=')) return makeToken(TokenKind::NotEq);
            return makeToken(TokenKind::Exclaim);

        case '<':
            if (match('<')) {
                if (match('=')) return makeToken(TokenKind::ShlEq);
                return makeToken(TokenKind::Shl);
            }
            if (match('=')) return makeToken(TokenKind::LtEq);
            return makeToken(TokenKind::Lt);

        case '>':
            if (match('>')) {
                if (match('=')) return makeToken(TokenKind::ShrEq);
                return makeToken(TokenKind::Shr);
            }
            if (match('=')) return makeToken(TokenKind::GtEq);
            return makeToken(TokenKind::Gt);

        case '|':
            if (match('>')) return makeToken(TokenKind::Pipe);
            if (match('=')) return makeToken(TokenKind::PipeEq);
            return makeToken(TokenKind::BitOr);

        default:
            return errorToken("unexpected character");
    }
}

} // namespace kern
