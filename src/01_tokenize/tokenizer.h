//
// Created by lambda on 10/31/25.
//

#pragma once
#include <utility>

#include "common.h"
#include "token/token.h"

class Tokenizer final {
public:
    explicit Tokenizer(string_t fileName, std::ifstream &file);

    explicit Tokenizer(const string_t &line);

    std::vector<Token> tokenize();

private:
    ch_t read_char();

    ch_t peek_char() const;

    void next_char();

    void skip_next_line();

    bool is_end() const;

    int current_position() const;

    string_t slice(int start, int end) const;

private:
    string_t m_filename;
    string_t m_stream;
    int m_pos;
    int m_lineCnt = 0;
    int m_columnCnt = 0;

    static std::unordered_map<string_t, TokenType> m_keywords;
    static std::unordered_map<string_t, TokenType> m_operators;
};
