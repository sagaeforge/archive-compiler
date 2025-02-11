#include <gtest/gtest.h>

#include "01_tokenize/Tokenizer.h"
#include "01_tokenize/factory/KeywordTokenFactory.h"

TEST(KeywordTokenFactory, canHandle) {
    icu::UnicodeString data = icu::UnicodeString::fromUTF8("function");
    auto tokenizer = nugdev::compiler::tokenize::Tokenizer({std::make_shared<nugdev::compiler::tokenize::KeywordTokenFactory>()});
    auto token = tokenizer.tokenize(data);
    EXPECT_EQ(token.size(), 1);
    EXPECT_EQ(token[0].to_str(), icu::UnicodeString::fromUTF8("function"));
}

TEST(KeywordTokenFactory, canNotHandle) {
    icu::UnicodeString data = icu::UnicodeString::fromUTF8("function123");
    auto tokenizer = nugdev::compiler::tokenize::Tokenizer({std::make_shared<nugdev::compiler::tokenize::KeywordTokenFactory>()});
    EXPECT_THROW(tokenizer.tokenize(data), std::runtime_error);
}

TEST(KeywordTokenFactory, canHandleMultipleWords) {
    icu::UnicodeString data = icu::UnicodeString::fromUTF8("function let");
    auto tokenizer = nugdev::compiler::tokenize::Tokenizer({std::make_shared<nugdev::compiler::tokenize::KeywordTokenFactory>()});
    auto token = tokenizer.tokenize(data);
    EXPECT_EQ(token.size(), 2);
    EXPECT_EQ(token[0].to_str(), icu::UnicodeString::fromUTF8("function"));
    EXPECT_EQ(token[1].to_str(), icu::UnicodeString::fromUTF8("let"));
}
