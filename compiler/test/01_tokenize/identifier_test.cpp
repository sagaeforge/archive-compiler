#include <gtest/gtest.h>
#include <unicode/unistr.h>

#include "01_tokenize/Tokenizer.h"

#include "01_tokenize/factory/IdentifierTokenFactory.h"

TEST(IdentifierTokenFactory, canHandle) {
    icu::UnicodeString data = icu::UnicodeString::fromUTF8("test");
    auto tokenizer = nugdev::compiler::tokenize::Tokenizer({std::make_shared<nugdev::compiler::tokenize::IdentifierTokenFactory>()});
    auto token = tokenizer.tokenize(data);
    EXPECT_EQ(token.size(), 1);
    EXPECT_EQ(token[0].to_str(), icu::UnicodeString::fromUTF8("test"));
}

TEST(IdentifierTokenFactory, canNotHandle) {
    icu::UnicodeString data = icu::UnicodeString::fromUTF8("123");
    auto tokenizer = nugdev::compiler::tokenize::Tokenizer({std::make_shared<nugdev::compiler::tokenize::IdentifierTokenFactory>()});
    EXPECT_THROW(tokenizer.tokenize(data), std::runtime_error);
}

TEST(IdentifierTokenFactory, canHandleMultipleWords) {
    icu::UnicodeString data = icu::UnicodeString::fromUTF8("test test");
    auto tokenizer = nugdev::compiler::tokenize::Tokenizer({std::make_shared<nugdev::compiler::tokenize::IdentifierTokenFactory>()});
    auto token = tokenizer.tokenize(data);
    EXPECT_EQ(token.size(), 2);
    EXPECT_EQ(token[0].to_str(), icu::UnicodeString::fromUTF8("test"));
    EXPECT_EQ(token[1].to_str(), icu::UnicodeString::fromUTF8("test"));
}
