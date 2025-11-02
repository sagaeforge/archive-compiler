//
// Created by lambda on 10/25/25.
//

#include "token.h"

#include "01_tokenize/token_converter.h"

Token::Token(string_t literal,
             const TokenType type,
             string_t fileName,
             const int line,
             const int column) : m_literal(std::move(literal)),
                                 m_type(type),
                                 m_fileName(std::move(fileName)),
                                 m_line(line),
                                 m_column(column) {
}

void Token::print(std::ostream &os) const {
    const auto json = TokenConverter::exportJson(*this);
    os << json;
}

std::partial_ordering Token::compare(const Token &other) const {
    const bool is_literal = m_literal == other.m_literal;
    const bool is_type = m_type == other.m_type;
    return is_literal && is_type ? std::partial_ordering::equivalent : std::partial_ordering::unordered;
}

const string_t &Token::literal() const {
    return m_literal;
}

const TokenType &Token::type() const {
    return m_type;
}

const string_t &Token::fileName() const {
    return m_fileName;
}

const int Token::line() const {
    return m_line;
}

const int Token::column() const {
    return m_column;
}

Token Token::illegal() {
    return {"", TokenType::Illegal, string_t(), 0, 0};
}
