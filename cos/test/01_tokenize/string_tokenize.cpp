//
// Created by lambda on 11/1/25.
//

#include <gtest/gtest.h>

#include "01_tokenize/tokenizer.h"

TEST(tokenizer_test, string_double_quote) {
    auto tokenizer = Tokenizer("\"안녕\"");
    auto tokens = tokenizer.tokenize();
    auto expectedToken = Token("안녕", TokenType::String, "line", 0, 0);

    ASSERT_EQ(1, tokens.size());
    ASSERT_EQ(expectedToken, tokens.front());
}

TEST(tokenizer_test, string_single_quote) {
    auto tokenizer = Tokenizer("\'안녕\'");
    auto tokens = tokenizer.tokenize();
    auto expectedToken = Token("안녕", TokenType::String, "line", 0, 0);

    ASSERT_EQ(1, tokens.size());
    ASSERT_EQ(expectedToken, tokens.front());
}

TEST(tokenizer_test, string_complex_quote) {
    auto tokenizer = Tokenizer("\"\'안녕\'\"");
    auto tokens = tokenizer.tokenize();
    auto expectedToken = Token("\'안녕\'", TokenType::String, "line", 0, 0);

    ASSERT_EQ(1, tokens.size());
    ASSERT_EQ(expectedToken, tokens.front());
}