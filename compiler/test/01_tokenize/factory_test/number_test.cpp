#include <gtest/gtest.h>
#include <unicode/unistr.h>

#include "01_tokenize/Tokenizer.h"
#include "01_tokenize/factory/NumberTokenFactory.h"

TEST(NumberTokenFactory, canHandle) {
    icu::UnicodeString data = icu::UnicodeString::fromUTF8("123");
    auto tokenizer = nugdev::compiler::tokenize::Tokenizer({std::make_shared<nugdev::compiler::tokenize::NumberTokenFactory>()});
    auto token = tokenizer.tokenize(data);
    EXPECT_EQ(token.size(), 1);
    EXPECT_EQ(token[0].to_str(), icu::UnicodeString::fromUTF8("123"));
}

TEST(NumberTokenFactory, canNotHandle) {
    icu::UnicodeString data = icu::UnicodeString::fromUTF8("test");
    auto tokenizer = nugdev::compiler::tokenize::Tokenizer({std::make_shared<nugdev::compiler::tokenize::NumberTokenFactory>()});
    EXPECT_THROW(tokenizer.tokenize(data), std::runtime_error);
}

TEST(NumberTokenFactory, canHandleMultipleNumbers) {
    icu::UnicodeString data = icu::UnicodeString::fromUTF8("123 456");
    auto tokenizer = nugdev::compiler::tokenize::Tokenizer({std::make_shared<nugdev::compiler::tokenize::NumberTokenFactory>()});
    auto token = tokenizer.tokenize(data);
    EXPECT_EQ(token.size(), 2);
    EXPECT_EQ(token[0].to_str(), icu::UnicodeString::fromUTF8("123"));
    EXPECT_EQ(token[1].to_str(), icu::UnicodeString::fromUTF8("456"));
}
