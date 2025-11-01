//
// Created by lambda on 11/1/25.
//

#include <gtest/gtest.h>

#include "01_tokenize/tokenizer.h"

TEST(tokenizer_test, number_int) {
    auto tokenizer = Tokenizer("1");
    auto tokens = tokenizer.tokenize();
    auto expectedToken = Token("1", TokenType::Number, "line", 0, 0);

    ASSERT_EQ(1, tokens.size());
    ASSERT_EQ(expectedToken, tokens.front());
}

TEST(tokenizer_test, number_int_max) {
    auto tokenizer = Tokenizer("123123123123123123 ");
    auto tokens = tokenizer.tokenize();
    auto expectedToken = Token("123123123123123123", TokenType::Number, "line", 0, 0);

    ASSERT_EQ(1, tokens.size());
    ASSERT_EQ(expectedToken, tokens.front());
}

TEST(tokenizer_test, number_float) {
    auto tokenizer = Tokenizer("123123123123123123.1");
    auto tokens = tokenizer.tokenize();
    auto expectedToken = Token("123123123123123123.1", TokenType::Number, "line", 0, 0);

    ASSERT_EQ(1, tokens.size());
    ASSERT_EQ(expectedToken, tokens.front());
}


TEST(tokenizer_test, number_float_l3) {
    auto tokenizer = Tokenizer("123123123123123123.123");
    auto tokens = tokenizer.tokenize();
    auto expectedToken = Token("123123123123123123.123", TokenType::Number, "line", 0, 0);

    ASSERT_EQ(1, tokens.size());
    ASSERT_EQ(expectedToken, tokens.front());
}