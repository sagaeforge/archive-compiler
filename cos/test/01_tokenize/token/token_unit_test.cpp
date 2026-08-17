//
// Created by lambda on 10/25/25.
//

#include "gtest/gtest.h"

#include "../../../src/01_tokenize/token/token.h"

class TokenUnitTest : public ::testing::Test {
};

TEST_F(TokenUnitTest, create) {
    const auto &sample_1 = Token{"", TokenType::Illegal, "", 0, 0};
    const auto &sample_2 = Token::illegal();

    ASSERT_EQ(sample_1, sample_2);
    ASSERT_EQ("", sample_1.literal());
    ASSERT_EQ(TokenType::Illegal, sample_1.type());
}