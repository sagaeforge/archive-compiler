//
// Created by lambda on 10/31/25.
//

#include "tokenizer.h"

#include <fstream>
#include <unistd.h>

Tokenizer::Tokenizer(string_t fileName, std::ifstream &file) : m_filename(std::move(fileName)) {
    m_pos = 0;
    m_lineCnt = 0;
    m_columnCnt = 0;

    string_t line;
    while (std::getline(file, line)) {
        m_stream += line + "\n";
    }
}

Tokenizer::Tokenizer(const string_t &line) : m_filename("line") {
    m_pos = 0;
    m_lineCnt = 0;
    m_columnCnt = 0;

    m_stream = line;
}

ch_t Tokenizer::read_char() {
    if (is_end()) {
        return 0;
    }

    auto ch = m_stream.at(m_pos);
    if (ch == '\n') {
        m_lineCnt++;
        m_columnCnt = -1;
    }
    return ch;
}

void Tokenizer::next_char() {
    m_pos++;
    m_columnCnt++;
}

void Tokenizer::skip_next_line() {
    while (read_char() != '\n') next_char();
}

bool Tokenizer::is_end() const {
    return m_pos >= m_stream.size();
}

int Tokenizer::current_position() const {
    return m_pos;
}

string_t Tokenizer::slice(int start, int end) const {
    return m_stream.substr(start, end - start + 1);
}

ch_t Tokenizer::peek_char() const {
    if (m_pos + 1 >= m_stream.size()) {
        return 0;
    }

    return m_stream.at(m_pos + 1);
}

std::vector<Token> Tokenizer::tokenize() {
    std::vector<Token> tokens;
    for (; !is_end(); next_char()) {
        const auto currentCh = read_char();

        // 공백 문자열 처리
        if (std::iswspace(currentCh)) {
            continue;
        }

        // 주석 처리
        if (currentCh == '#') {
            skip_next_line();
            continue;
        }

        // 문자열 처리.
        if (currentCh == '\'' || currentCh == '\"') {
            auto pos = current_position();
            auto column = m_columnCnt;
            auto line = m_lineCnt;
            for (; !is_end() && peek_char() != currentCh; next_char());

            auto literal = slice(pos + 1, current_position());
            tokens.emplace_back(literal, TokenType::String, m_filename, line, column);
            next_char();
            continue;
        }

        // 숫자 처리.
        if (std::iswdigit(currentCh) || currentCh == '.') {
            int pos = current_position();
            auto column = m_columnCnt;
            auto line = m_lineCnt;
            for (; !is_end() && std::iswdigit(peek_char()); next_char());
            if (peek_char() != '.') {
                auto literal = slice(pos, current_position());
                tokens.emplace_back(literal, TokenType::Number, m_filename, line, column);
                continue;
            }
            next_char();
            for (; !is_end() && std::iswdigit(peek_char()); next_char());

            auto literal = slice(pos, current_position());
            tokens.emplace_back(literal, TokenType::Number, m_filename, line, column);
            continue;
        }

        // 연산자 처리.
        if (auto operatorItr = m_operators.find(string_t({currentCh}));
            operatorItr != m_operators.end()) {
            auto column = m_columnCnt;
            auto line = m_lineCnt;

            // 연산자
            const auto nextCh = peek_char();
            if (const auto twoOperatorItr = m_operators.find(string_t({currentCh, nextCh}));
                twoOperatorItr != m_operators.end()) {
                next_char();
                const auto &[literal, type] = *twoOperatorItr;
                tokens.emplace_back(literal, type, m_filename, line, column);
                continue;
            }
            const auto &[literal, tokenType] = *operatorItr;
            tokens.emplace_back(literal, tokenType, m_filename, line, column);
            continue;
        }

        // 식별자 혹은 키워드 처리.
        if (currentCh == '_' || std::iswalnum(currentCh)) {
            auto pos = current_position();
            auto column = m_columnCnt;
            auto line = m_lineCnt;
            for (; !is_end() && (peek_char() == '_' || std::iswalnum(peek_char())); next_char());

            auto literal = slice(pos, current_position());
            if (auto keywordItr = m_keywords.find(literal); keywordItr != m_keywords.end()) {
                const auto &[_, type] = *keywordItr;
                tokens.emplace_back(literal, type, m_filename, line, column);
                continue;
            }
            tokens.emplace_back(literal, TokenType::Identifier, m_filename, line, column);
            continue;
        }

        Logger::info("Unknown Charactor: {}", currentCh);
    }

    return tokens;
}

std::unordered_map<string_t, TokenType> Tokenizer::m_keywords = {
    {"var", TokenType::Variable},
    {"if", TokenType::If},
    {"fn", TokenType::Function},
    {"return", TokenType::Return}
};

std::unordered_map<string_t, TokenType> Tokenizer::m_operators = {
    {"(", TokenType::LeftParenthesis},
    {")", TokenType::RightParenthesis},
    {"{", TokenType::LeftBrace},
    {"}", TokenType::RightBrace},
    {"+", TokenType::Plus},
    {"-", TokenType::Minus},
    {"*", TokenType::Multiply},
    {"/", TokenType::Divide},
    {":", TokenType::Colon},
    {"<", TokenType::Less},
    {"<=", TokenType::LessEqual},
    {">", TokenType::Greater},
    {"<=", TokenType::GreaterEqual},
    {"==", TokenType::Equal},
    {"=", TokenType::Assign},
};
