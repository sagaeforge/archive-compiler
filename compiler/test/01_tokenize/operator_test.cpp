#include <gtest/gtest.h>

#include "01_tokenize/Tokenizer.h"
#include "01_tokenize/factory/OperatorTokenFactory.h"

TEST(OperatorTokenFactory, canHandle) {
    icu::UnicodeString data = icu::UnicodeString::fromUTF8("+");
    auto tokenizer = nugdev::compiler::tokenize::Tokenizer({std::make_shared<nugdev::compiler::tokenize::OperatorTokenFactory>()});
    auto token = tokenizer.tokenize(data);
    EXPECT_EQ(token.size(), 1);
    EXPECT_EQ(token[0].to_str(), icu::UnicodeString::fromUTF8("+"));
}

TEST(OperatorTokenFactory, canNotHandle) {
    icu::UnicodeString data = icu::UnicodeString::fromUTF8("a");
    auto tokenizer = nugdev::compiler::tokenize::Tokenizer({std::make_shared<nugdev::compiler::tokenize::OperatorTokenFactory>()});
    EXPECT_THROW(tokenizer.tokenize(data), std::runtime_error);
}

TEST(OperatorTokenFactory, canHandleMultipleOperators) {
    icu::UnicodeString data = icu::UnicodeString::fromUTF8("++");
    auto tokenizer = nugdev::compiler::tokenize::Tokenizer({std::make_shared<nugdev::compiler::tokenize::OperatorTokenFactory>()});
    auto token = tokenizer.tokenize(data);
    EXPECT_EQ(token.size(), 2);
    EXPECT_EQ(token[0].to_str(), icu::UnicodeString::fromUTF8("+"));
    EXPECT_EQ(token[1].to_str(), icu::UnicodeString::fromUTF8("+"));
}