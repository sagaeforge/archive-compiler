#include <gtest/gtest.h>
#include <unicode/unistr.h>

#include "01_tokenize/Tokenizer.h"
#include "01_tokenize/factory/StringTokenFactory.h"

TEST(StringTokenFactory, canHandle) {
    icu::UnicodeString data = icu::UnicodeString::fromUTF8("\"test\"");
    auto tokenizer = nugdev::compiler::tokenize::Tokenizer({std::make_shared<nugdev::compiler::tokenize::StringTokenFactory>()});
    auto token = tokenizer.tokenize(data);
    EXPECT_EQ(token.size(), 1);
    EXPECT_EQ(token[0].get_literal(), icu::UnicodeString::fromUTF8("test"));
}

TEST(StringTokenFactory, canNotHandle) {
    icu::UnicodeString data = icu::UnicodeString::fromUTF8("test");
    auto tokenizer = nugdev::compiler::tokenize::Tokenizer({std::make_shared<nugdev::compiler::tokenize::StringTokenFactory>()});
    EXPECT_THROW(tokenizer.tokenize(data), std::runtime_error);
}

TEST(StringTokenFactory, canHandleMultipleQuotes) {
    icu::UnicodeString data = icu::UnicodeString::fromUTF8("\"test\"\"test\"");
    auto tokenizer = nugdev::compiler::tokenize::Tokenizer({std::make_shared<nugdev::compiler::tokenize::StringTokenFactory>()});
    auto token = tokenizer.tokenize(data);
    EXPECT_EQ(token.size(), 2);
    EXPECT_EQ(token[0].get_literal(), icu::UnicodeString::fromUTF8("test"));
    EXPECT_EQ(token[1].get_literal(), icu::UnicodeString::fromUTF8("test"));
}