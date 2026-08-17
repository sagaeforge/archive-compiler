//
// Created by lambda on 10/25/25.
//

#pragma once

#include <utility>

#include "common.h"
#include "00_core/printable.hpp"
#include "token_type.h"
#include "00_core/comparable.h"
#include "00_core/nlohmann/json.hpp"

class Token final : public printable, comparable<Token> {
public:
    Token(string_t literal, TokenType type, string_t fileName, int line, int column);

    void print(std::ostream &os) const override;

    [[nodiscard]] std::partial_ordering compare(const Token &other) const override;

    [[nodiscard]] const string_t &literal() const;

    [[nodiscard]] const TokenType &type() const;

    [[nodiscard]] const string_t &fileName() const;

    [[nodiscard]] const int line() const;

    [[nodiscard]] const int column() const;

public:
    static Token illegal();

private:
    string_t m_literal;
    TokenType m_type;
    string_t m_fileName;
    int m_line;
    int m_column;
};
